#include <string.h>

#include "node.h"
#include "node_buffer.h"
#include "node_version.h"

#include "macros.h"
#include "database.h"
#include "statement.h"

using namespace node_sqlite3;

jxcore::ThreadStore<JS_PERSISTENT_FUNCTION_TEMPLATE> Statement::jx_persistent;

void Statement::Process() {
  if (finalized && !queue.empty()) {
    return CleanQueue();
  }

  while (prepared && !locked && !queue.empty()) {
    Call* call = queue.front();
    queue.pop();

    call->callback(call->baton);
    delete call;
  }
}

void Statement::Schedule(Work_Callback callback, Baton* baton) {
  if (finalized) {
    queue.push(new Call(callback, baton));
    CleanQueue();
  } else if (!prepared || locked) {
    queue.push(new Call(callback, baton));
  } else {
    callback(baton);
  }
}

template <class T>
void Statement::Error(T* baton) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);
  Statement* stmt = baton->stmt;
  // Fail hard on logic errors.
  assert(stmt->status != 0);
  EXCEPTION(stmt->message.c_str(), stmt->status, exception);

  if (!JS_IS_EMPTY(baton->callback) && JS_IS_FUNCTION(baton->callback)) {
    JS_LOCAL_VALUE argv[] = {exception};
    TRY_CATCH_CALL(stmt->handle_, JS_TYPE_TO_LOCAL_FUNCTION(baton->callback), 1,
                   argv);
  } else {
    JS_LOCAL_VALUE argv[] = {STD_TO_STRING("error"), exception};
    EMIT_EVENT(stmt->handle_, 2, argv);
  }
}

// { Database db, String sql, Array params, Function callback }
JS_METHOD(Statement, New) {
  if (!args.IsConstructCall()) {
    THROW_TYPE_EXCEPTION(
        "Use the new operator to create new Statement objects");
  }

  int length = args.Length();

  JS_HANDLE_VALUE arg0 = args.GetItem(0);
  if (length <= 0 || !Database::HasInstance(com->threadId, arg0)) {
    THROW_TYPE_EXCEPTION("Database object expected");
  } else if (length <= 1 || !args.IsString(1)) {
    THROW_TYPE_EXCEPTION("SQL query expected");
  } else if (length > 2 && !args.IsUndefined(2) && !args.IsFunction(2)) {
    THROW_TYPE_EXCEPTION("Callback expected");
  }

  JS_CLASS_NEW_INSTANCE(obj, Statement);

  Database* db = ObjectWrap::Unwrap<Database>(JS_VALUE_TO_OBJECT(arg0));
  JS_HANDLE_STRING sql = args.GetAsString(1);

  JS_NAME_SET(obj, JS_STRING_ID("sql"), sql);

  Statement* stmt = new Statement(db);
  stmt->Wrap(obj);

  PrepareBaton* baton = new PrepareBaton(db, args.GetAsFunction(2), stmt);
  baton->sql = std::string(STRING_TO_STD(sql));
  db->Schedule(Work_BeginPrepare, baton);

  RETURN_PARAM(obj);
}
JS_METHOD_END

void Statement::Work_BeginPrepare(Database::Baton* baton) {
  assert(baton->db->open);
  baton->db->pending++;
  int status = uv_queue_work(uv_default_loop(), &baton->request, Work_Prepare,
                             (uv_after_work_cb)Work_AfterPrepare);
  assert(status == 0);
}

void Statement::Work_Prepare(uv_work_t* req) {
  STATEMENT_INIT(PrepareBaton);

  // In case preparing fails, we use a mutex to make sure we get the associated
  // error message.
  sqlite3_mutex* mtx = sqlite3_db_mutex(baton->db->handle);
  sqlite3_mutex_enter(mtx);

  stmt->status = sqlite3_prepare_v2(baton->db->handle, baton->sql.c_str(),
                                    baton->sql.size(), &stmt->handle, NULL);

  if (stmt->status != SQLITE_OK) {
    stmt->message = std::string(sqlite3_errmsg(baton->db->handle));
    stmt->handle = NULL;
  }

  sqlite3_mutex_leave(mtx);
}

void Statement::Work_AfterPrepare(uv_work_t* req) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  STATEMENT_INIT(PrepareBaton);

  if (stmt->status != SQLITE_OK) {
    Error(baton);
    stmt->Finalize();
  } else {
    stmt->prepared = true;
    if (!JS_IS_EMPTY(baton->callback) && JS_IS_FUNCTION(baton->callback)) {
      JS_LOCAL_VALUE argv[] = {JS_NULL()};
      TRY_CATCH_CALL(stmt->handle_, JS_TYPE_TO_LOCAL_FUNCTION(baton->callback),
                     1, argv);
    }
  }

  STATEMENT_END();
}

template <class T>
Values::Field* Statement::BindParameter(const JS_HANDLE_VALUE source, T pos) {
  if (JS_IS_STRING(source) || JS_IS_REGEXP(source)) {
    jxcore::JXString val(source);
    return new Values::Text(pos, val.length(), *val);
  } else if (JS_IS_INT32(source)) {
    return new Values::Integer(pos, source->Int32Value());
  } else if (JS_IS_NUMBER(source)) {
    return new Values::Float(pos, source->NumberValue());
  } else if (JS_IS_BOOLEAN(source)) {
    return new Values::Integer(pos, source->BooleanValue() ? 1 : 0);
  } else if (JS_IS_NULL(source)) {
    return new Values::Null(pos);
  } else if (Buffer::HasInstance(source)) {
    JS_LOCAL_OBJECT buffer = JS_VALUE_TO_OBJECT(source);
    return new Values::Blob(pos, BUFFER__LENGTH(buffer), BUFFER__DATA(buffer));
  } else if (JS_IS_DATE(source)) {
    return new Values::Float(pos, source->NumberValue());
  } else {
    return NULL;
  }
}

template <class T>
T* Statement::Bind(node::commons* com, const jxcore::PArguments& args,
                   int start, int last) {
  JS_DEFINE_STATE_MARKER(com);

  if (last < 0) last = args.Length();
  JS_HANDLE_FUNCTION callback;
  if (last > start && args.IsFunction(last - 1)) {
    callback = args.GetAsFunction(last - 1);
    last--;
  }

  T* baton = new T(this, callback);

  JS_HANDLE_VALUE start_val = args.GetItem(start);

  if (start < last) {
    if (args.IsArray(start)) {
      JS_HANDLE_ARRAY array = args.GetAsArray(start);
      int length = JS_GET_ARRAY_LENGTH(array);
      // Note: bind parameters start with 1.
      for (int i = 0, pos = 1; i < length; i++, pos++) {
        baton->parameters.push_back(BindParameter(JS_GET_INDEX(array, i), pos));
      }
    } else if (!args.IsObject(start) || args.IsRegExp(start) ||
               args.IsDate(start) || Buffer::jxHasInstance(start_val, com)) {
      // Parameters directly in array.
      // Note: bind parameters start with 1.
      for (int i = start, pos = 1; i < last; i++, pos++) {
        baton->parameters.push_back(BindParameter(args.GetItem(i), pos));
      }
    } else if (args.IsObject(start)) {
      JS_LOCAL_OBJECT object = JS_VALUE_TO_OBJECT(start_val);
      JS_LOCAL_ARRAY array = object->GetPropertyNames();
      int length = JS_GET_ARRAY_LENGTH(array);
      for (int i = 0; i < length; i++) {
        JS_LOCAL_VALUE name = JS_GET_INDEX(array, i);

        if (JS_IS_INT32(name)) {
          int32_t idn = INT32_TO_STD(name);
          baton->parameters.push_back(
              BindParameter(JS_GET_INDEX(object, idn), INT32_TO_STD(name)));
        } else {
          JS_LOCAL_STRING str_name = JS_VALUE_TO_STRING(name);
          baton->parameters.push_back(BindParameter(
              JS_GET_NAME(object, str_name), STRING_TO_STD(str_name)));
        }
      }
    } else {
      return NULL;
    }
  }

  return baton;
}

bool Statement::Bind(const Parameters& parameters) {
  if (parameters.size() == 0) {
    return true;
  }

  sqlite3_reset(handle);
  sqlite3_clear_bindings(handle);

  Parameters::const_iterator it = parameters.begin();
  Parameters::const_iterator end = parameters.end();

  for (; it < end; ++it) {
    Values::Field* field = *it;

    if (field != NULL) {
      int pos;
      if (field->index > 0) {
        pos = field->index;
      } else {
        pos = sqlite3_bind_parameter_index(handle, field->name.c_str());
      }

      switch (field->type) {
        case SQLITE_INTEGER: {
          status =
              sqlite3_bind_int(handle, pos, ((Values::Integer*)field)->value);
        } break;
        case SQLITE_FLOAT: {
          status =
              sqlite3_bind_double(handle, pos, ((Values::Float*)field)->value);
        } break;
        case SQLITE_TEXT: {
          status = sqlite3_bind_text(
              handle, pos, ((Values::Text*)field)->value.c_str(),
              ((Values::Text*)field)->value.size(), SQLITE_TRANSIENT);
        } break;
        case SQLITE_BLOB: {
          status = sqlite3_bind_blob(handle, pos, ((Values::Blob*)field)->value,
                                     ((Values::Blob*)field)->length,
                                     SQLITE_TRANSIENT);
        } break;
        case SQLITE_NULL: {
          status = sqlite3_bind_null(handle, pos);
        } break;
      }
    }

    if (status != SQLITE_OK) {
      message = std::string(sqlite3_errmsg(db->handle));
      return false;
    }
  }

  return true;
}

JS_METHOD(Statement, Bind) {
  Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());

  Baton* baton = stmt->Bind<Baton>(com, args);
  if (baton == NULL) {
    THROW_EXCEPTION("Data type is not supported");
  } else {
    stmt->Schedule(Work_BeginBind, baton);
    RETURN_PARAM(args.This());
  }
}
JS_METHOD_END

void Statement::Work_BeginBind(Baton* baton) { STATEMENT_BEGIN(Bind); }

void Statement::Work_Bind(uv_work_t* req) {
  STATEMENT_INIT(Baton);

  sqlite3_mutex* mtx = sqlite3_db_mutex(stmt->db->handle);
  sqlite3_mutex_enter(mtx);
  stmt->Bind(baton->parameters);
  sqlite3_mutex_leave(mtx);
}

void Statement::Work_AfterBind(uv_work_t* req) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);
  STATEMENT_INIT(Baton);

  if (stmt->status != SQLITE_OK) {
    Error(baton);
  } else {
    // Fire callbacks.
    if (!JS_IS_EMPTY(baton->callback) && JS_IS_FUNCTION(baton->callback)) {
      JS_LOCAL_VALUE argv[] = {JS_NULL()};
      TRY_CATCH_CALL(stmt->handle_, JS_TYPE_TO_LOCAL_FUNCTION(baton->callback),
                     1, argv);
    }
  }

  STATEMENT_END();
}

JS_METHOD(Statement, Get) {
  Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());

  Baton* baton = stmt->Bind<RowBaton>(com, args);
  if (baton == NULL) {
    THROW_EXCEPTION("Data type is not supported");
  } else {
    stmt->Schedule(Work_BeginGet, baton);
    RETURN_PARAM(args.This());
  }
}
JS_METHOD_END

void Statement::Work_BeginGet(Baton* baton) { STATEMENT_BEGIN(Get); }

void Statement::Work_Get(uv_work_t* req) {
  STATEMENT_INIT(RowBaton);

  if (stmt->status != SQLITE_DONE || baton->parameters.size()) {
    sqlite3_mutex* mtx = sqlite3_db_mutex(stmt->db->handle);
    sqlite3_mutex_enter(mtx);

    if (stmt->Bind(baton->parameters)) {
      stmt->status = sqlite3_step(stmt->handle);

      if (!(stmt->status == SQLITE_ROW || stmt->status == SQLITE_DONE)) {
        stmt->message = std::string(sqlite3_errmsg(stmt->db->handle));
      }
    }

    sqlite3_mutex_leave(mtx);

    if (stmt->status == SQLITE_ROW) {
      // Acquire one result row before returning.
      GetRow(&baton->row, stmt->handle);
    }
  }
}

void Statement::Work_AfterGet(uv_work_t* req) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);
  STATEMENT_INIT(RowBaton);

  if (stmt->status != SQLITE_ROW && stmt->status != SQLITE_DONE) {
    Error(baton);
  } else {
    // Fire callbacks.
    if (!JS_IS_EMPTY(baton->callback) && JS_IS_FUNCTION(baton->callback)) {
      if (stmt->status == SQLITE_ROW) {
        // Create the result array from the data we acquired.
        JS_LOCAL_VALUE argv[] = {JS_NULL(), RowToJS(&baton->row)};
        TRY_CATCH_CALL(stmt->handle_,
                       JS_TYPE_TO_LOCAL_FUNCTION(baton->callback), 2, argv);
      } else {
        JS_LOCAL_VALUE argv[] = {JS_NULL()};
        TRY_CATCH_CALL(stmt->handle_,
                       JS_TYPE_TO_LOCAL_FUNCTION(baton->callback), 1, argv);
      }
    }
  }

  STATEMENT_END();
}

JS_METHOD(Statement, Run) {
  Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());

  Baton* baton = stmt->Bind<RunBaton>(com, args);
  if (baton == NULL) {
    THROW_EXCEPTION("Data type is not supported");
  } else {
    stmt->Schedule(Work_BeginRun, baton);
    RETURN_PARAM(args.This());
  }
}
JS_METHOD_END

void Statement::Work_BeginRun(Baton* baton) { STATEMENT_BEGIN(Run); }

void Statement::Work_Run(uv_work_t* req) {
  STATEMENT_INIT(RunBaton);

  sqlite3_mutex* mtx = sqlite3_db_mutex(stmt->db->handle);
  sqlite3_mutex_enter(mtx);

  // Make sure that we also reset when there are no parameters.
  if (!baton->parameters.size()) {
    sqlite3_reset(stmt->handle);
  }

  if (stmt->Bind(baton->parameters)) {
    stmt->status = sqlite3_step(stmt->handle);

    if (!(stmt->status == SQLITE_ROW || stmt->status == SQLITE_DONE)) {
      stmt->message = std::string(sqlite3_errmsg(stmt->db->handle));
    } else {
      baton->inserted_id = sqlite3_last_insert_rowid(stmt->db->handle);
      baton->changes = sqlite3_changes(stmt->db->handle);
    }
  }

  sqlite3_mutex_leave(mtx);
}

void Statement::Work_AfterRun(uv_work_t* req) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);
  STATEMENT_INIT(RunBaton);

  if (stmt->status != SQLITE_ROW && stmt->status != SQLITE_DONE) {
    Error(baton);
  } else {
    // Fire callbacks.
    if (!JS_IS_EMPTY(baton->callback) && JS_IS_FUNCTION(baton->callback)) {
      JS_NAME_SET(stmt->handle_, JS_STRING_ID("lastID"),
                  STD_TO_INTEGER(baton->inserted_id));
      JS_NAME_SET(stmt->handle_, JS_STRING_ID("changes"),
                  STD_TO_INTEGER(baton->changes));

      JS_LOCAL_VALUE argv[] = {JS_NULL()};
      TRY_CATCH_CALL(stmt->handle_, JS_TYPE_TO_LOCAL_FUNCTION(baton->callback),
                     1, argv);
    }
  }

  STATEMENT_END();
}

JS_METHOD(Statement, All) {
  Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());

  Baton* baton = stmt->Bind<RowsBaton>(com, args);
  if (baton == NULL) {
    THROW_EXCEPTION("Data type is not supported");
  } else {
    stmt->Schedule(Work_BeginAll, baton);
    RETURN_PARAM(args.This());
  }
}
JS_METHOD_END

void Statement::Work_BeginAll(Baton* baton) { STATEMENT_BEGIN(All); }

void Statement::Work_All(uv_work_t* req) {
  STATEMENT_INIT(RowsBaton);

  sqlite3_mutex* mtx = sqlite3_db_mutex(stmt->db->handle);
  sqlite3_mutex_enter(mtx);

  // Make sure that we also reset when there are no parameters.
  if (!baton->parameters.size()) {
    sqlite3_reset(stmt->handle);
  }

  if (stmt->Bind(baton->parameters)) {
    while ((stmt->status = sqlite3_step(stmt->handle)) == SQLITE_ROW) {
      Row* row = new Row();
      GetRow(row, stmt->handle);
      baton->rows.push_back(row);
    }

    if (stmt->status != SQLITE_DONE) {
      stmt->message = std::string(sqlite3_errmsg(stmt->db->handle));
    }
  }

  sqlite3_mutex_leave(mtx);
}

void Statement::Work_AfterAll(uv_work_t* req) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);
  STATEMENT_INIT(RowsBaton);

  if (stmt->status != SQLITE_DONE) {
    Error(baton);
  } else {
    // Fire callbacks.
    if (!JS_IS_EMPTY(baton->callback) && JS_IS_FUNCTION(baton->callback)) {
      if (baton->rows.size()) {
        // Create the result array from the data we acquired.
        JS_LOCAL_ARRAY result = JS_NEW_ARRAY_WITH_COUNT(baton->rows.size());
        Rows::const_iterator it = baton->rows.begin();
        Rows::const_iterator end = baton->rows.end();
        for (int i = 0; it < end; ++it, i++) {
          JS_INDEX_SET(result, i, RowToJS(*it));
          delete *it;
        }

        JS_LOCAL_VALUE argv[] = {JS_NULL(), result};
        TRY_CATCH_CALL(stmt->handle_,
                       JS_TYPE_TO_LOCAL_FUNCTION(baton->callback), 2, argv);
      } else {
        // There were no result rows.
        JS_LOCAL_VALUE argv[] = {JS_NULL(), JS_NEW_ARRAY()};
        TRY_CATCH_CALL(stmt->handle_,
                       JS_TYPE_TO_LOCAL_FUNCTION(baton->callback), 2, argv);
      }
    }
  }

  STATEMENT_END();
}

JS_METHOD(Statement, Each) {
  Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());

  int last = args.Length();

  JS_HANDLE_FUNCTION completed;
  if (last >= 2 && args.IsFunction(last - 1) && args.IsFunction(last - 2)) {
    completed = args.GetAsFunction(--last);
  }

  EachBaton* baton = stmt->Bind<EachBaton>(com, args, 0, last);
  if (baton == NULL) {
    THROW_EXCEPTION("Data type is not supported");
  } else {
    baton->completed = JS_NEW_PERSISTENT_FUNCTION(completed);
    stmt->Schedule(Work_BeginEach, baton);
    RETURN_PARAM(args.This());
  }
}
JS_METHOD_END

void Statement::Work_BeginEach(Baton* baton) {
  // Only create the Async object when we're actually going into
  // the event loop. This prevents dangling events.
  EachBaton* each_baton = static_cast<EachBaton*>(baton);
  each_baton->async = new Async(each_baton->stmt, AsyncEach);
  each_baton->async->item_cb = JS_NEW_PERSISTENT_FUNCTION(each_baton->callback);
  each_baton->async->completed_cb =
      JS_NEW_PERSISTENT_FUNCTION(each_baton->completed);

  STATEMENT_BEGIN(Each);
}

void Statement::Work_Each(uv_work_t* req) {
  STATEMENT_INIT(EachBaton);

  Async* async = baton->async;

  sqlite3_mutex* mtx = sqlite3_db_mutex(stmt->db->handle);

  int retrieved = 0;

  // Make sure that we also reset when there are no parameters.
  if (!baton->parameters.size()) {
    sqlite3_reset(stmt->handle);
  }

  if (stmt->Bind(baton->parameters)) {
    while (true) {
      sqlite3_mutex_enter(mtx);
      stmt->status = sqlite3_step(stmt->handle);
      if (stmt->status == SQLITE_ROW) {
        sqlite3_mutex_leave(mtx);
        Row* row = new Row();
        GetRow(row, stmt->handle);
        NODE_SQLITE3_MUTEX_LOCK(&async->mutex)
        async->data.push_back(row);
        retrieved++;
        NODE_SQLITE3_MUTEX_UNLOCK(&async->mutex)

        uv_async_send(&async->watcher);
      } else {
        if (stmt->status != SQLITE_DONE) {
          stmt->message = std::string(sqlite3_errmsg(stmt->db->handle));
        }
        sqlite3_mutex_leave(mtx);
        break;
      }
    }
  }

  async->completed = true;
  uv_async_send(&async->watcher);
}

void Statement::CloseCallback(uv_handle_t* handle) {
  assert(handle != NULL);
  assert(handle->data != NULL);
  Async* async = static_cast<Async*>(handle->data);
  delete async;
}

void Statement::AsyncEach(uv_async_t* handle, int status) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  Async* async = static_cast<Async*>(handle->data);

  while (true) {
    // Get the contents out of the data cache for us to process in the JS
    // callback.
    Rows rows;
    NODE_SQLITE3_MUTEX_LOCK(&async->mutex)
    rows.swap(async->data);
    NODE_SQLITE3_MUTEX_UNLOCK(&async->mutex)

    if (rows.empty()) {
      break;
    }

    if (!JS_IS_EMPTY(async->item_cb) && JS_IS_FUNCTION(async->item_cb)) {
      JS_LOCAL_VALUE argv[2];
      argv[0] = JS_NULL();

      Rows::const_iterator it = rows.begin();
      Rows::const_iterator end = rows.end();
      for (int i = 0; it < end; ++it, i++) {
        argv[1] = RowToJS(*it);
        async->retrieved++;
        TRY_CATCH_CALL(async->stmt->handle_,
                       JS_TYPE_TO_LOCAL_FUNCTION(async->item_cb), 2, argv);
        delete *it;
      }
    }
  }

  if (async->completed) {
    if (!JS_IS_EMPTY(async->completed_cb) &&
        JS_IS_FUNCTION(async->completed_cb)) {
      JS_LOCAL_VALUE argv[] = {JS_NULL(), STD_TO_INTEGER(async->retrieved)};
      TRY_CATCH_CALL(async->stmt->handle_,
                     JS_TYPE_TO_LOCAL_FUNCTION(async->completed_cb), 2, argv);
    }
    uv_close((uv_handle_t*)handle, CloseCallback);
  }
}

void Statement::Work_AfterEach(uv_work_t* req) {
  JS_ENTER_SCOPE();
  STATEMENT_INIT(EachBaton);

  if (stmt->status != SQLITE_DONE) {
    Error(baton);
  }

  STATEMENT_END();
}

JS_METHOD(Statement, Reset) {
  Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());

  OPTIONAL_ARGUMENT_FUNCTION(0, callback);

  Baton* baton = new Baton(stmt, callback);
  stmt->Schedule(Work_BeginReset, baton);

  RETURN_PARAM(args.This());
}
JS_METHOD_END

void Statement::Work_BeginReset(Baton* baton) { STATEMENT_BEGIN(Reset); }

void Statement::Work_Reset(uv_work_t* req) {
  STATEMENT_INIT(Baton);

  sqlite3_reset(stmt->handle);
  stmt->status = SQLITE_OK;
}

void Statement::Work_AfterReset(uv_work_t* req) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);
  STATEMENT_INIT(Baton);

  // Fire callbacks.
  if (!JS_IS_EMPTY(baton->callback) && JS_IS_FUNCTION(baton->callback)) {
    JS_LOCAL_VALUE argv[] = {JS_NULL()};
    TRY_CATCH_CALL(stmt->handle_, JS_TYPE_TO_LOCAL_FUNCTION(baton->callback), 1,
                   argv);
  }

  STATEMENT_END();
}

JS_LOCAL_OBJECT Statement::RowToJS(Row* row) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);
  JS_LOCAL_OBJECT result = JS_NEW_EMPTY_OBJECT();

  Row::const_iterator it = row->begin();
  Row::const_iterator end = row->end();
  for (int i = 0; it < end; ++it, i++) {
    Values::Field* field = *it;

    JS_LOCAL_VALUE value;

    switch (field->type) {
      case SQLITE_INTEGER: {
        value = STD_TO_NUMBER(((Values::Integer*)field)->value);
      } break;
      case SQLITE_FLOAT: {
        value = STD_TO_NUMBER(((Values::Float*)field)->value);
      } break;
      case SQLITE_TEXT: {
        value = JS_TYPE_TO_LOCAL_VALUE(
            STD_TO_STRING_WITH_LENGTH(((Values::Text*)field)->value.c_str(),
                                      ((Values::Text*)field)->value.size()));
      } break;
      case SQLITE_BLOB: {
        value = JS_TYPE_TO_LOCAL_VALUE(
            Buffer::New(((Values::Blob*)field)->value,
                        ((Values::Blob*)field)->length)->handle_);
      } break;
      case SQLITE_NULL: {
        value = JS_NULL();
      } break;
    }

    JS_NAME_SET(result, JS_STRING_ID(field->name.c_str()), value);

    DELETE_FIELD(field);
  }

  return result;
}

void Statement::GetRow(Row* row, sqlite3_stmt* stmt) {
  int rows = sqlite3_column_count(stmt);

  for (int i = 0; i < rows; i++) {
    int type = sqlite3_column_type(stmt, i);
    const char* name = sqlite3_column_name(stmt, i);
    switch (type) {
      case SQLITE_INTEGER: {
        row->push_back(
            new Values::Integer(name, sqlite3_column_int64(stmt, i)));
      } break;
      case SQLITE_FLOAT: {
        row->push_back(new Values::Float(name, sqlite3_column_double(stmt, i)));
      } break;
      case SQLITE_TEXT: {
        const char* text = (const char*)sqlite3_column_text(stmt, i);
        int length = sqlite3_column_bytes(stmt, i);
        row->push_back(new Values::Text(name, length, text));
      } break;
      case SQLITE_BLOB: {
        const void* blob = sqlite3_column_blob(stmt, i);
        int length = sqlite3_column_bytes(stmt, i);
        row->push_back(new Values::Blob(name, length, blob));
      } break;
      case SQLITE_NULL: {
        row->push_back(new Values::Null(name));
      } break;
      default:
        assert(false);
    }
  }
}

JS_METHOD(Statement, Finalize) {
  Statement* stmt = ObjectWrap::Unwrap<Statement>(args.This());
  OPTIONAL_ARGUMENT_FUNCTION(0, callback);

  Baton* baton = new Baton(stmt, callback);
  stmt->Schedule(Finalize, baton);

  RETURN_PARAM(stmt->db->handle_);
}
JS_METHOD_END

void Statement::Finalize(Baton* baton) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);
  baton->stmt->Finalize();

  // Fire callback in case there was one.
  if (!JS_IS_EMPTY(baton->callback) && JS_IS_FUNCTION(baton->callback)) {
    TRY_CATCH_CALL_NO_PARAM(baton->stmt->handle_,
                            JS_TYPE_TO_LOCAL_FUNCTION(baton->callback));
  }

  delete baton;
}

void Statement::Finalize() {
  assert(!finalized);
  finalized = true;
  CleanQueue();
  // Finalize returns the status code of the last operation. We already fired
  // error events in case those failed.
  sqlite3_finalize(handle);
  handle = NULL;
  db->Unref();
}

void Statement::CleanQueue() {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  if (prepared && !queue.empty()) {
    // This statement has already been prepared and is now finalized.
    // Fire error for all remaining items in the queue.
    EXCEPTION("Statement is already finalized", SQLITE_MISUSE, exception);
    JS_LOCAL_VALUE argv[] = {exception};
    bool called = false;

    // Clear out the queue so that this object can get GC'ed.
    while (!queue.empty()) {
      Call* call = queue.front();
      queue.pop();

      if (prepared && !JS_IS_EMPTY(call->baton->callback) &&
          JS_IS_FUNCTION(call->baton->callback)) {
        TRY_CATCH_CALL(
            handle_, JS_TYPE_TO_LOCAL_FUNCTION(call->baton->callback), 1, argv);
        called = true;
      }

      // We don't call the actual callback, so we have to make sure that
      // the baton gets destroyed.
      delete call->baton;
      delete call;
    }

    // When we couldn't call a callback function, emit an error on the
    // Statement object.
    if (!called) {
      JS_LOCAL_VALUE args[] = {STD_TO_STRING("error"), exception};
      EMIT_EVENT(handle_, 2, args);
    }
  } else
    while (!queue.empty()) {
      // Just delete all items in the queue; we already fired an event when
      // preparing the statement failed.
      Call* call = queue.front();
      queue.pop();

      // We don't call the actual callback, so we have to make sure that
      // the baton gets destroyed.
      delete call->baton;
      delete call;
    }
}
