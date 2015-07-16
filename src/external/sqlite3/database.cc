#include <string.h>
#include <node.h>

#include "macros.h"
#include "database.h"
#include "statement.h"

using namespace node_sqlite3;
using namespace node;

jxcore::ThreadStore<JS_PERSISTENT_FUNCTION_TEMPLATE> Database::jx_persistent;

void Database::Process() {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  if (!open && locked && !queue.empty()) {
    EXCEPTION("Database handle is closed", SQLITE_MISUSE, exception);
    JS_LOCAL_VALUE argv[] = {exception};
    bool called = false;

    // Call all callbacks with the error object.
    while (!queue.empty()) {
      Call* call = queue.front();
      if (!JS_IS_EMPTY(call->baton->callback) &&
          JS_IS_FUNCTION(call->baton->callback)) {
        TRY_CATCH_CALL(handle_, JS_TYPE_TO_LOCAL_FUNCTION(call->baton->callback), 1, argv);
        called = true;
      }
      queue.pop();
      // We don't call the actual callback, so we have to make sure that
      // the baton gets destroyed.
      delete call->baton;
      delete call;
    }

    // When we couldn't call a callback function, emit an error on the
    // Database object.
    if (!called) {
      JS_LOCAL_VALUE args[] = {STD_TO_STRING("error"), exception};
      EMIT_EVENT(handle_, 2, args);
    }
    return;
  }

  while (open && (!locked || pending == 0) && !queue.empty()) {
    Call* call = queue.front();

    if (call->exclusive && pending > 0) {
      break;
    }

    queue.pop();
    locked = call->exclusive;
    call->callback(call->baton);
    delete call;

    if (locked) break;
  }
}

void Database::Schedule(Work_Callback callback, Baton* baton, bool exclusive) {
  if (!open && locked) {
	node::commons *com = node::commons::getInstance();
	JS_DEFINE_STATE_MARKER(com);
    EXCEPTION("Database is closed", SQLITE_MISUSE, exception);
    if (!JS_IS_EMPTY(baton->callback) && JS_IS_FUNCTION(baton->callback)) {
      JS_LOCAL_VALUE argv[] = {exception};
      TRY_CATCH_CALL(handle_, JS_TYPE_TO_LOCAL_FUNCTION(baton->callback), 1, argv);
    } else {
      JS_LOCAL_VALUE argv[] = {STD_TO_STRING("error"), exception};
      EMIT_EVENT(handle_, 2, argv);
    }
    return;
  }

  if (!open || ((locked || exclusive || serialize) && pending > 0)) {
    queue.push(new Call(callback, baton, exclusive || serialize));
  } else {
    locked = exclusive;
    callback(baton);
  }
}

JS_METHOD(Database, New) {
  if (!args.IsConstructCall()) {
    THROW_TYPE_EXCEPTION("Use the new operator to create new Database objects");
  }

  JS_CLASS_NEW_INSTANCE(obj, Database);
  REQUIRE_ARGUMENT_STRING(0, filename);
  int pos = 1;

  int mode;
  if (args.Length() >= pos && args.IsInteger(pos)) {
    mode = args.GetInt32(pos++);
  } else {
    mode = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
  }

  JS_HANDLE_FUNCTION callback;
  if (args.Length() >= pos && args.IsFunction(pos)) {
    callback = args.GetAsFunction(pos++);
  }

  Database* db = new Database();
  db->Wrap(obj);

  JS_NAME_SET(obj, JS_STRING_ID("filename"), args.GetAsString(0));
  JS_NAME_SET(obj, JS_STRING_ID("mode"), STD_TO_INTEGER(mode));

  // Start opening the database.
  OpenBaton* baton = new OpenBaton(db, callback, *filename, mode);
  Work_BeginOpen(baton);

  RETURN_PARAM(obj);
}
JS_METHOD_END

void Database::Work_BeginOpen(Baton* baton) {
  int status = uv_queue_work(uv_default_loop(), &baton->request, Work_Open,
                             (uv_after_work_cb)Work_AfterOpen);
  assert(status == 0);
}

void Database::Work_Open(uv_work_t* req) {
  OpenBaton* baton = static_cast<OpenBaton*>(req->data);
  Database* db = baton->db;

  baton->status =
      sqlite3_open_v2(baton->filename.c_str(), &db->handle, baton->mode, NULL);

  if (baton->status != SQLITE_OK) {
    baton->message = std::string(sqlite3_errmsg(db->handle));
    sqlite3_close(db->handle);
    db->handle = NULL;
  } else {
    // Set default database handle values.
    sqlite3_busy_timeout(db->handle, 1000);
  }
}

void Database::Work_AfterOpen(uv_work_t* req) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  OpenBaton* baton = static_cast<OpenBaton*>(req->data);
  Database* db = baton->db;

  JS_LOCAL_VALUE argv[1];
  if (baton->status != SQLITE_OK) {
    EXCEPTION(baton->message.c_str(), baton->status, exception);
    argv[0] = exception;
  } else {
    db->open = true;
    argv[0] = JS_NULL();
  }

  if (!JS_IS_EMPTY(baton->callback) && JS_IS_FUNCTION(baton->callback)) {
    TRY_CATCH_CALL(db->handle_, JS_TYPE_TO_LOCAL_FUNCTION(baton->callback), 1, argv);
  } else if (!db->open) {
    JS_LOCAL_VALUE args[] = {STD_TO_STRING("error"), argv[0]};
    EMIT_EVENT(db->handle_, 2, args);
  }

  if (db->open) {
    JS_LOCAL_VALUE args[] = {STD_TO_STRING("open")};
    EMIT_EVENT(db->handle_, 1, args);
    db->Process();
  }

  delete baton;
}

JS_GETTER_CLASS_METHOD(Database, OpenGetter) {
  Database* db = ObjectWrap::Unwrap<Database>(caller);
  RETURN_GETTER_PARAM(STD_TO_BOOLEAN(db->open));
}
JS_GETTER_METHOD_END

JS_METHOD(Database, Close) {
  Database* db = ObjectWrap::Unwrap<Database>(args.This());
  OPTIONAL_ARGUMENT_FUNCTION(0, callback);

  Baton* baton = new Baton(db, callback);
  db->Schedule(Work_BeginClose, baton, true);

  RETURN_PARAM(args.This());
}
JS_METHOD_END

void Database::Work_BeginClose(Baton* baton) {
  baton->db->RemoveCallbacks();
  uv_queue_work(uv_default_loop(), &baton->request, Work_Close,
                (uv_after_work_cb)Work_AfterClose);
}

void Database::Work_Close(uv_work_t* req) {
  Baton* baton = static_cast<Baton*>(req->data);
  Database* db = baton->db;

  baton->status = sqlite3_close(db->handle);

  if (baton->status != SQLITE_OK) {
    baton->message = std::string(sqlite3_errmsg(db->handle));
  } else {
    db->handle = NULL;
  }
}

void Database::Work_AfterClose(uv_work_t* req) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  Baton* baton = static_cast<Baton*>(req->data);
  Database* db = baton->db;

  JS_LOCAL_VALUE argv[1];
  if (baton->status != SQLITE_OK) {
    EXCEPTION(baton->message.c_str(), baton->status, exception);
    argv[0] = exception;
  } else {
    db->open = false;
    // Leave db->locked to indicate that this db object has reached
    // the end of its life.
    argv[0] = JS_NULL();
  }

  // Fire callbacks.
  if (!JS_IS_EMPTY(baton->callback) && JS_IS_FUNCTION(baton->callback)) {
    TRY_CATCH_CALL(db->handle_, JS_TYPE_TO_LOCAL_FUNCTION(baton->callback), 1, argv);
  } else if (db->open) {
    JS_LOCAL_VALUE args[] = {STD_TO_STRING("error"), argv[0]};
    EMIT_EVENT(db->handle_, 2, args);
  }

  if (!db->open) {
    JS_LOCAL_VALUE args[] = {STD_TO_STRING("close"), argv[0]};
    EMIT_EVENT(db->handle_, 1, args);
    db->Process();
  }

  delete baton;
}

JS_METHOD(Database, Serialize) {
  Database* db = ObjectWrap::Unwrap<Database>(args.This());
  OPTIONAL_ARGUMENT_FUNCTION(0, callback);

  bool before = db->serialize;
  db->serialize = true;

  if (!JS_IS_EMPTY(callback) && JS_IS_FUNCTION(callback)) {
	JS_HANDLE_OBJECT _this = args.This();
	TRY_CATCH_CALL_NO_PARAM(_this, JS_TYPE_TO_LOCAL_FUNCTION(callback));
    db->serialize = before;
  }

  db->Process();

  RETURN_PARAM(args.This());
}
JS_METHOD_END

JS_METHOD(Database, Parallelize) {
  Database* db = ObjectWrap::Unwrap<Database>(args.This());
  OPTIONAL_ARGUMENT_FUNCTION(0, callback);

  bool before = db->serialize;
  db->serialize = false;

  if (!JS_IS_EMPTY(callback) && JS_IS_FUNCTION(callback)) {
	JS_HANDLE_OBJECT _this = args.This();
	TRY_CATCH_CALL_NO_PARAM(_this, JS_TYPE_TO_LOCAL_FUNCTION(callback));
    db->serialize = before;
  }

  db->Process();

  RETURN_PARAM(args.This());
}
JS_METHOD_END

JS_METHOD(Database, Configure) {
  Database* db = ObjectWrap::Unwrap<Database>(args.This());

  REQUIRE_ARGUMENTS(2);

  jxcore::JXString jxs;
  args.GetString(0, &jxs);

  if (strcmp(*jxs, "trace") == 0) {
    JS_LOCAL_FUNCTION handle;
    Baton* baton = new Baton(db, handle);
    db->Schedule(RegisterTraceCallback, baton);
  } else if (strcmp(*jxs, "profile") == 0) {
    JS_LOCAL_FUNCTION handle;
    Baton* baton = new Baton(db, handle);
    db->Schedule(RegisterProfileCallback, baton);
  } else if (strcmp(*jxs, "busyTimeout") == 0) {
    if (!args.IsInteger(1)) {
      THROW_TYPE_EXCEPTION("Value must be an integer");
    }
    JS_LOCAL_FUNCTION handle;
    Baton* baton = new Baton(db, handle);
    baton->status = args.GetInt32(1);
    db->Schedule(SetBusyTimeout, baton);
  } else {
    std::string str = *jxs;
    str += " is not a valid configuration option";
    THROW_EXCEPTION(str.c_str());
  }

  db->Process();

  RETURN_PARAM(args.This());
}
JS_METHOD_END

void Database::SetBusyTimeout(Baton* baton) {
  assert(baton->db->open);
  assert(baton->db->handle);

  // Abuse the status field for passing the timeout.
  sqlite3_busy_timeout(baton->db->handle, baton->status);

  delete baton;
}

void Database::RegisterTraceCallback(Baton* baton) {
  assert(baton->db->open);
  assert(baton->db->handle);
  Database* db = baton->db;

  if (db->debug_trace == NULL) {
    // Add it.
    db->debug_trace = new AsyncTrace(db, TraceCallback);
    sqlite3_trace(db->handle, TraceCallback, db);
  } else {
    // Remove it.
    sqlite3_trace(db->handle, NULL, NULL);
    db->debug_trace->finish();
    db->debug_trace = NULL;
  }

  delete baton;
}

void Database::TraceCallback(void* db, const char* sql) {
  // Note: This function is called in the thread pool.
  // Note: Some queries, such as "EXPLAIN" queries, are not sent through this.
  static_cast<Database*>(db)->debug_trace->send(new std::string(sql));
}

void Database::TraceCallback(Database* db, std::string* sql) {
  // Note: This function is called in the main V8 thread.
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_VALUE argv[] = {STD_TO_STRING("trace"), STD_TO_STRING(sql->c_str())};
  EMIT_EVENT(db->handle_, 2, argv);
  delete sql;
}

void Database::RegisterProfileCallback(Baton* baton) {
  assert(baton->db->open);
  assert(baton->db->handle);
  Database* db = baton->db;

  if (db->debug_profile == NULL) {
    // Add it.
    db->debug_profile = new AsyncProfile(db, ProfileCallback);
    sqlite3_profile(db->handle, ProfileCallback, db);
  } else {
    // Remove it.
    sqlite3_profile(db->handle, NULL, NULL);
    db->debug_profile->finish();
    db->debug_profile = NULL;
  }

  delete baton;
}

void Database::ProfileCallback(void* db, const char* sql,
                               sqlite3_uint64 nsecs) {
  // Note: This function is called in the thread pool.
  // Note: Some queries, such as "EXPLAIN" queries, are not sent through this.
  ProfileInfo* info = new ProfileInfo();
  info->sql = std::string(sql);
  info->nsecs = nsecs;
  static_cast<Database*>(db)->debug_profile->send(info);
}

void Database::ProfileCallback(Database* db, ProfileInfo* info) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_VALUE argv[] = {STD_TO_STRING("profile"),
                           STD_TO_STRING(info->sql.c_str()),
                           STD_TO_NUMBER((double)info->nsecs / 1000000.0)};
  EMIT_EVENT(db->handle_, 3, argv);
  delete info;
}

void Database::RegisterUpdateCallback(Baton* baton) {
  assert(baton->db->open);
  assert(baton->db->handle);
  Database* db = baton->db;

  if (db->update_event == NULL) {
    // Add it.
    db->update_event = new AsyncUpdate(db, UpdateCallback);
    sqlite3_update_hook(db->handle, UpdateCallback, db);
  } else {
    // Remove it.
    sqlite3_update_hook(db->handle, NULL, NULL);
    db->update_event->finish();
    db->update_event = NULL;
  }

  delete baton;
}

void Database::UpdateCallback(void* db, int type, const char* database,
                              const char* table, sqlite3_int64 rowid) {
  // Note: This function is called in the thread pool.
  // Note: Some queries, such as "EXPLAIN" queries, are not sent through this.
  UpdateInfo* info = new UpdateInfo();
  info->type = type;
  info->database = std::string(database);
  info->table = std::string(table);
  info->rowid = rowid;
  static_cast<Database*>(db)->update_event->send(info);
}

void Database::UpdateCallback(Database* db, UpdateInfo* info) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_VALUE argv[] = {STD_TO_STRING(sqlite_authorizer_string(info->type)),
                           STD_TO_STRING(info->database.c_str()),
                           STD_TO_STRING(info->table.c_str()),
                           STD_TO_INTEGER(info->rowid), };
  EMIT_EVENT(db->handle_, 4, argv);
  delete info;
}

JS_METHOD(Database, Exec) {
  Database* db = ObjectWrap::Unwrap<Database>(args.This());

  REQUIRE_ARGUMENT_STRING(0, sql);
  OPTIONAL_ARGUMENT_FUNCTION(1, callback);

  Baton* baton = new ExecBaton(db, callback, *sql);
  db->Schedule(Work_BeginExec, baton, true);

  RETURN_PARAM(args.This());
}
JS_METHOD_END

void Database::Work_BeginExec(Baton* baton) {
  assert(baton->db->locked);
  assert(baton->db->open);
  assert(baton->db->handle);
  assert(baton->db->pending == 0);
  int status = uv_queue_work(uv_default_loop(), &baton->request, Work_Exec,
                             (uv_after_work_cb)Work_AfterExec);
  assert(status == 0);
}

void Database::Work_Exec(uv_work_t* req) {
  ExecBaton* baton = static_cast<ExecBaton*>(req->data);

  char* message = NULL;
  baton->status =
      sqlite3_exec(baton->db->handle, baton->sql.c_str(), NULL, NULL, &message);

  if (baton->status != SQLITE_OK && message != NULL) {
    baton->message = std::string(message);
    sqlite3_free(message);
  }
}

void Database::Work_AfterExec(uv_work_t* req) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  ExecBaton* baton = static_cast<ExecBaton*>(req->data);
  Database* db = baton->db;

  if (baton->status != SQLITE_OK) {
    EXCEPTION(baton->message.c_str(), baton->status, exception);

    if (!JS_IS_EMPTY(baton->callback) && JS_IS_FUNCTION(baton->callback)) {
      JS_LOCAL_VALUE argv[] = {exception};
      TRY_CATCH_CALL(db->handle_, JS_TYPE_TO_LOCAL_FUNCTION(baton->callback), 1, argv);
    } else {
      JS_LOCAL_VALUE args[] = {STD_TO_STRING("error"), exception};
      EMIT_EVENT(db->handle_, 2, args);
    }
  } else if (!JS_IS_EMPTY(baton->callback) &&
             JS_IS_FUNCTION(baton->callback)) {
    JS_LOCAL_VALUE argv[] = {JS_NULL()};
    TRY_CATCH_CALL(db->handle_, JS_TYPE_TO_LOCAL_FUNCTION(baton->callback), 1, argv);
  }

  db->Process();

  delete baton;
}

JS_METHOD(Database, Wait) {
  Database* db = ObjectWrap::Unwrap<Database>(args.This());

  OPTIONAL_ARGUMENT_FUNCTION(0, callback);

  Baton* baton = new Baton(db, callback);
  db->Schedule(Work_Wait, baton, true);

  RETURN_PARAM(args.This());
}
JS_METHOD_END

void Database::Work_Wait(Baton* baton) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  assert(baton->db->locked);
  assert(baton->db->open);
  assert(baton->db->handle);
  assert(baton->db->pending == 0);

  if (!JS_IS_EMPTY(baton->callback) && JS_IS_FUNCTION(baton->callback)) {
    JS_LOCAL_VALUE argv[] = {JS_NULL()};
    TRY_CATCH_CALL(baton->db->handle_, JS_TYPE_TO_LOCAL_FUNCTION(baton->callback), 1, argv);
  }

  baton->db->Process();

  delete baton;
}

JS_METHOD(Database, LoadExtension) {
  Database* db = ObjectWrap::Unwrap<Database>(args.This());

  REQUIRE_ARGUMENT_STRING(0, filename);
  OPTIONAL_ARGUMENT_FUNCTION(1, callback);

  Baton* baton = new LoadExtensionBaton(db, callback, *filename);
  db->Schedule(Work_BeginLoadExtension, baton, true);

  RETURN_PARAM(args.This());
}
JS_METHOD_END

void Database::Work_BeginLoadExtension(Baton* baton) {
  assert(baton->db->locked);
  assert(baton->db->open);
  assert(baton->db->handle);
  assert(baton->db->pending == 0);
  int status =
      uv_queue_work(uv_default_loop(), &baton->request, Work_LoadExtension,
                    (uv_after_work_cb)Work_AfterLoadExtension);
  assert(status == 0);
}

void Database::Work_LoadExtension(uv_work_t* req) {
  LoadExtensionBaton* baton = static_cast<LoadExtensionBaton*>(req->data);

  sqlite3_enable_load_extension(baton->db->handle, 1);

  char* message = NULL;
  baton->status = sqlite3_load_extension(baton->db->handle,
                                         baton->filename.c_str(), 0, &message);

  sqlite3_enable_load_extension(baton->db->handle, 0);

  if (baton->status != SQLITE_OK && message != NULL) {
    baton->message = std::string(message);
    sqlite3_free(message);
  }
}

void Database::Work_AfterLoadExtension(uv_work_t* req) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  LoadExtensionBaton* baton = static_cast<LoadExtensionBaton*>(req->data);
  Database* db = baton->db;

  if (baton->status != SQLITE_OK) {
    EXCEPTION(baton->message.c_str(), baton->status, exception);

    if (!JS_IS_EMPTY(baton->callback) && JS_IS_FUNCTION(baton->callback)) {
      JS_LOCAL_VALUE argv[] = {exception};
      TRY_CATCH_CALL(db->handle_, JS_TYPE_TO_LOCAL_FUNCTION(baton->callback), 1, argv);
    } else {
      JS_LOCAL_VALUE args[] = {STD_TO_STRING("error"), exception};
      EMIT_EVENT(db->handle_, 2, args);
    }
  } else if (!JS_IS_EMPTY(baton->callback) &&
             JS_IS_FUNCTION(baton->callback)) {
    JS_LOCAL_VALUE argv[] = {JS_NULL()};
    TRY_CATCH_CALL(db->handle_, JS_TYPE_TO_LOCAL_FUNCTION(baton->callback), 1, argv);
  }

  db->Process();

  delete baton;
}

void Database::RemoveCallbacks() {
  if (debug_trace) {
    debug_trace->finish();
    debug_trace = NULL;
  }
  if (debug_profile) {
    debug_profile->finish();
    debug_profile = NULL;
  }
}
