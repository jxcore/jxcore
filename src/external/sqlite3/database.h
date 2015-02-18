#ifndef NODE_SQLITE3_SRC_DATABASE_H
#define NODE_SQLITE3_SRC_DATABASE_H

#include "node.h"

#include <string>
#include <queue>

#include "sqlite3.h"
#include "async.h"

#include "../jx_persistent_store.h"

using namespace ENGINE_NS;
using namespace node;

namespace node_sqlite3 {

class Database : public ObjectWrap {
 public:
  static jxcore::ThreadStore<JS_PERSISTENT_FUNCTION_TEMPLATE> jx_persistent;

  static JS_DEFINE_GETTER_METHOD(OpenGetter);

  INIT_NAMED_CLASS_MEMBERS(Database, Database) {
    int id = com->threadId;
    jx_persistent.templates[id] =
        JS_NEW_PERSISTENT_FUNCTION_TEMPLATE(constructor);

    SET_INSTANCE_METHOD("close", Close, 0);
    SET_INSTANCE_METHOD("exec", Exec, 0);
    SET_INSTANCE_METHOD("wait", Wait, 0);
    SET_INSTANCE_METHOD("loadExtension", LoadExtension, 0);
    SET_INSTANCE_METHOD("serialize", Serialize, 0);
    SET_INSTANCE_METHOD("parallelize", Parallelize, 0);
    SET_INSTANCE_METHOD("configure", Configure, 0);

#ifdef JS_ENGINE_V8
    enum v8::PropertyAttribute attributes =
        static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete);
    constructor->InstanceTemplate()->SetAccessor(
        STD_TO_STRING("open"), OpenGetter, NULL, JS_HANDLE_VALUE(),
        v8::DEFAULT, attributes);
#elif defined(JS_ENGINE_MOZJS)
    JS_ACCESSOR_SET(constructor, STD_TO_STRING("open"), OpenGetter, NULL);
#endif
  }
  END_INIT_NAMED_MEMBERS(Database)

  static inline bool HasInstance(const int threadId, JS_HANDLE_VALUE_REF val) {
    if (!JS_IS_OBJECT(val)) return false;
    return JS_HAS_INSTANCE(jx_persistent.templates[threadId], JS_VALUE_TO_OBJECT(val));
  }

  struct Baton {
    uv_work_t request;
    Database* db;
    JS_PERSISTENT_FUNCTION callback;
    int status;
    std::string message;

    Baton(Database* db_, JS_HANDLE_FUNCTION cb_) : db(db_), status(SQLITE_OK) {
      db->Ref();
      request.data = this;
      callback = JS_NEW_PERSISTENT_FUNCTION(cb_);
    }

    virtual ~Baton() {
      db->Unref();
      callback.Dispose();
    }
  };

  struct OpenBaton : Baton {
    std::string filename;
    int mode;
    OpenBaton(Database* db_, JS_HANDLE_FUNCTION cb_, const char* filename_,
              int mode_)
        : Baton(db_, cb_), filename(filename_), mode(mode_) {}
  };

  struct ExecBaton : Baton {
    std::string sql;
    ExecBaton(Database* db_, JS_HANDLE_FUNCTION cb_, const char* sql_)
        : Baton(db_, cb_), sql(sql_) {}
  };

  struct LoadExtensionBaton : Baton {
    std::string filename;
    LoadExtensionBaton(Database* db_, JS_HANDLE_FUNCTION cb_,
                       const char* filename_)
        : Baton(db_, cb_), filename(filename_) {}
  };

  typedef void (*Work_Callback)(Baton* baton);

  struct Call {
    Call(Work_Callback cb_, Baton* baton_, bool exclusive_ = false)
        : callback(cb_), exclusive(exclusive_), baton(baton_) {};
    Work_Callback callback;
    bool exclusive;
    Baton* baton;
  };

  struct ProfileInfo {
    std::string sql;
    sqlite3_int64 nsecs;
  };

  struct UpdateInfo {
    int type;
    std::string database;
    std::string table;
    sqlite3_int64 rowid;
  };

  bool IsOpen() { return open; }
  bool IsLocked() { return locked; }

  typedef Async<std::string, Database> AsyncTrace;
  typedef Async<ProfileInfo, Database> AsyncProfile;
  typedef Async<UpdateInfo, Database> AsyncUpdate;

  friend class Statement;

 protected:
  Database()
      : ObjectWrap(),
        handle(NULL),
        open(false),
        locked(false),
        pending(0),
        serialize(false),
        debug_trace(NULL),
        debug_profile(NULL),
        update_event(NULL) {}

  ~Database() {
    RemoveCallbacks();
    sqlite3_close(handle);
    handle = NULL;
    open = false;
  }

  static DEFINE_JS_METHOD(New);
  static void Work_BeginOpen(Baton* baton);
  static void Work_Open(uv_work_t* req);
  static void Work_AfterOpen(uv_work_t* req);

  void Schedule(Work_Callback callback, Baton* baton, bool exclusive = false);
  void Process();

  static DEFINE_JS_METHOD(Exec);
  static void Work_BeginExec(Baton* baton);
  static void Work_Exec(uv_work_t* req);
  static void Work_AfterExec(uv_work_t* req);

  static DEFINE_JS_METHOD(Wait);
  static void Work_Wait(Baton* baton);

  static DEFINE_JS_METHOD(Close);
  static void Work_BeginClose(Baton* baton);
  static void Work_Close(uv_work_t* req);
  static void Work_AfterClose(uv_work_t* req);

  static DEFINE_JS_METHOD(LoadExtension);
  static void Work_BeginLoadExtension(Baton* baton);
  static void Work_LoadExtension(uv_work_t* req);
  static void Work_AfterLoadExtension(uv_work_t* req);

  static DEFINE_JS_METHOD(Serialize);
  static DEFINE_JS_METHOD(Parallelize);

  static DEFINE_JS_METHOD(Configure);

  static void SetBusyTimeout(Baton* baton);

  static void RegisterTraceCallback(Baton* baton);
  static void TraceCallback(void* db, const char* sql);
  static void TraceCallback(Database* db, std::string* sql);

  static void RegisterProfileCallback(Baton* baton);
  static void ProfileCallback(void* db, const char* sql, sqlite3_uint64 nsecs);
  static void ProfileCallback(Database* db, ProfileInfo* info);

  static void RegisterUpdateCallback(Baton* baton);
  static void UpdateCallback(void* db, int type, const char* database,
                             const char* table, sqlite3_int64 rowid);
  static void UpdateCallback(Database* db, UpdateInfo* info);

  void RemoveCallbacks();

 protected:
  sqlite3* handle;

  bool open;
  bool locked;
  unsigned int pending;

  bool serialize;

  std::queue<Call*> queue;

  AsyncTrace* debug_trace;
  AsyncProfile* debug_profile;
  AsyncUpdate* update_event;
};
}

#endif
