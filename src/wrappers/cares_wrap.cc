// Copyright & License details are available under JXCORE_LICENSE file

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define CARES_STATICLIB
#include "ares.h"
#include "wrappers/cares_wrap.h"
#include "jx/commons.h"
#include "tree.h"

#if defined(__OpenBSD__) || defined(__MINGW32__) || defined(_MSC_VER) || \
    defined(__ANDROID__) || defined(ANDROID)
#include <nameser.h>
#else
#include <arpa/nameser.h>
#endif

namespace node {

namespace cares_wrap {

// TODO(obastemur) make this memory efficient
static RB_HEAD(ares_task_list, ares_task_t) ares_tasks[MAX_JX_THREADS];

static int cmp_ares_tasks(const ares_task_t* a, const ares_task_t* b) {
  if (a->sock < b->sock) return -1;
  if (a->sock > b->sock) return 1;
  return 0;
}

RB_GENERATE_STATIC(ares_task_list, ares_task_t, node, cmp_ares_tasks)

/* This is called once per second by loop->timer. It is used to constantly */
/* call back into c-ares for possibly processing timeouts. */
static void ares_timeout(uv_timer_t* handle, int status) {
  commons* com = node::commons::getInstance();
  assert(!RB_EMPTY(&ares_tasks[com->threadId + 1]));
  ares_process_fd(com->_ares_channel, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
}

static void ares_poll_cb(uv_poll_t* watcher, int status, int events) {
  ares_task_t* task = container_of(watcher, ares_task_t, poll_watcher);

  commons* com = node::commons::getInstance();
  /* Reset the idle timer */
  uv_timer_again(com->ares_timer);

  if (status < 0) {
    /* An error happened. Just pretend that the socket is both readable and */
    /* writable. */
    ares_process_fd(com->_ares_channel, task->sock, task->sock);
    return;
  }

  /* Process DNS responses */
  ares_process_fd(com->_ares_channel,
                  events & UV_READABLE ? task->sock : ARES_SOCKET_BAD,
                  events & UV_WRITABLE ? task->sock : ARES_SOCKET_BAD);
}

static void ares_poll_close_cb(uv_handle_t* watcher) {
  ares_task_t* task = container_of(watcher, ares_task_t, poll_watcher);
  if (task != NULL) free(task);
}

/* Allocates and returns a new ares_task_t */
static ares_task_t* ares_task_create(uv_loop_t* loop, ares_socket_t sock) {
  ares_task_t* task = (ares_task_t*)malloc(sizeof *task);

  if (task == NULL) {
    /* Out of memory. */
    return NULL;
  }

  task->loop = loop;
  task->sock = sock;

  if (uv_poll_init_socket(loop, &task->poll_watcher, sock) < 0) {
    /* This should never happen. */
    if (task != NULL) free(task);
    return NULL;
  }

  return task;
}

/* Callback from ares when socket operation is started */
static void ares_sockstate_cb(void* data, ares_socket_t sock, int read,
                              int write) {
  uv_loop_t* loop = (uv_loop_t*)data;
  ares_task_t* task;

  commons* com = node::commons::getInstance();
  ares_task_t lookup_task;
  lookup_task.sock = sock;
  task = RB_FIND(ares_task_list, &ares_tasks[com->threadId + 1], &lookup_task);

  if (read || write) {
    if (!task) {
      /* New socket */

      /* If this is the first socket then start the timer. */
      if (!uv_is_active((uv_handle_t*)com->ares_timer)) {
        assert(RB_EMPTY(&ares_tasks[com->threadId + 1]));
        uv_timer_start(com->ares_timer, ares_timeout, 1000, 1000);
      }

      task = ares_task_create(loop, sock);
      if (task == NULL) {
        /* This should never happen unless we're out of memory or something */
        /* is seriously wrong. The socket won't be polled, but the the query */
        /* will eventually time out. */
        return;
      }

      RB_INSERT(ares_task_list, &ares_tasks[com->threadId + 1], task);
    }

    /* This should never fail. If it fails anyway, the query will eventually */
    /* time out. */
    uv_poll_start(&task->poll_watcher,
                  (read ? UV_READABLE : 0) | (write ? UV_WRITABLE : 0),
                  ares_poll_cb);

  } else {
    /* read == 0 and write == 0 this is c-ares's way of notifying us that */
    /* the socket is now closed. We must free the data associated with */
    /* socket. */
    assert(task &&
           "When an ares socket is closed we should have a handle for it");

    RB_REMOVE(ares_task_list, &ares_tasks[com->threadId + 1], task);
    uv_close((uv_handle_t*)&task->poll_watcher, ares_poll_close_cb);

    if (RB_EMPTY(&ares_tasks[com->threadId + 1])) {
      uv_timer_stop(com->ares_timer);
    }
  }
}

static JS_LOCAL_ARRAY HostentToAddresses(struct hostent* host) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_ARRAY addresses = JS_NEW_ARRAY();

  char ip[INET6_ADDRSTRLEN];
  for (int i = 0; host->h_addr_list[i]; ++i) {
    uv_inet_ntop(host->h_addrtype, host->h_addr_list[i], ip, sizeof(ip));

    JS_LOCAL_STRING address = STD_TO_STRING(ip);
    JS_INDEX_SET(addresses, i, address);
  }

  return JS_LEAVE_SCOPE(addresses);
}

static JS_LOCAL_ARRAY HostentToNames(struct hostent* host) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_ARRAY names = JS_NEW_ARRAY();

  for (int i = 0; host->h_aliases[i]; ++i) {
    JS_LOCAL_STRING address = STD_TO_STRING(host->h_aliases[i]);
    JS_INDEX_SET(names, i, address);
  }

  return JS_LEAVE_SCOPE(names);
}

static const char* AresErrnoString(int errorno) {
  switch (errorno) {
#define ERRNO_CASE(e) \
  case ARES_##e:      \
    return #e;
    ERRNO_CASE(SUCCESS)
    ERRNO_CASE(ENODATA)
    ERRNO_CASE(EFORMERR)
    ERRNO_CASE(ESERVFAIL)
    ERRNO_CASE(ENOTFOUND)
    ERRNO_CASE(ENOTIMP)
    ERRNO_CASE(EREFUSED)
    ERRNO_CASE(EBADQUERY)
    ERRNO_CASE(EBADNAME)
    ERRNO_CASE(EBADFAMILY)
    ERRNO_CASE(EBADRESP)
    ERRNO_CASE(ECONNREFUSED)
    ERRNO_CASE(ETIMEOUT)
    ERRNO_CASE(EOF)
    ERRNO_CASE(EFILE)
    ERRNO_CASE(ENOMEM)
    ERRNO_CASE(EDESTRUCTION)
    ERRNO_CASE(EBADSTR)
    ERRNO_CASE(EBADFLAGS)
    ERRNO_CASE(ENONAME)
    ERRNO_CASE(EBADHINTS)
    ERRNO_CASE(ENOTINITIALIZED)
    ERRNO_CASE(ELOADIPHLPAPI)
    ERRNO_CASE(EADDRGETNETWORKPARAMS)
    ERRNO_CASE(ECANCELLED)
#undef ERRNO_CASE
    default:
      assert(0 && "Unhandled c-ares error");
      return "(UNKNOWN)";
  }
}

static void SetAresErrno(int errorno) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_VALUE value = STD_TO_STRING(AresErrnoString(errorno));
  JS_NAME_SET(com->getProcess(), JS_PREDEFINED_STRING(_errno), value);
}

class QueryWrap {
  node::commons* _com;

 public:
  QueryWrap() {
    JS_ENTER_SCOPE_COM();
    JS_DEFINE_STATE_MARKER(com);
    _com = com;

    object_ = JS_NEW_EMPTY_PERSISTENT_OBJECT();
  }

  virtual ~QueryWrap() {
    assert(!JS_IS_EMPTY(object_));

    JS_DEFINE_STATE_MARKER(_com);
    commons* com = _com;

    JS_NAME_DELETE(object_, JS_PREDEFINED_STRING(oncomplete));

    JS_CLEAR_PERSISTENT(object_);
  }

  JS_HANDLE_OBJECT GetObject() { return object_; }

  void SetOnComplete(JS_HANDLE_VALUE oncomplete) {
    JS_DEFINE_STATE_MARKER(_com);
    commons* com = _com;

    assert(JS_IS_FUNCTION(oncomplete));
    JS_NAME_SET(object_, JS_PREDEFINED_STRING(oncomplete), oncomplete);
  }

  // Subclasses should implement the appropriate Send method.
  virtual int Send(const char* name) {
    assert(0);
    return 0;
  }

  virtual int Send(const char* name, int family) {
    assert(0);
    return 0;
  }

 protected:
  void* GetQueryArg() { return static_cast<void*>(this); }

  static void Callback(void* arg, int status, int timeouts,
                       unsigned char* answer_buf, int answer_len) {
    QueryWrap* wrap = static_cast<QueryWrap*>(arg);

    if (status != ARES_SUCCESS) {
      wrap->ParseError(status);
    } else {
      wrap->Parse(answer_buf, answer_len);
    }

    delete wrap;
  }

  static void Callback(void* arg, int status, int timeouts,
                       struct hostent* host) {
    QueryWrap* wrap = static_cast<QueryWrap*>(arg);

    if (status != ARES_SUCCESS) {
      wrap->ParseError(status);
    } else {
      wrap->Parse(host);
    }

    delete wrap;
  }

  void CallOnComplete(JS_LOCAL_VALUE answer) {
    JS_ENTER_SCOPE();
    JS_DEFINE_STATE_MARKER(_com);
    commons* com = _com;

    JS_LOCAL_VALUE argv[2] = {STD_TO_INTEGER(0), answer};
    MakeCallback(com, object_, com->pstr_oncomplete, ARRAY_SIZE(argv), argv);
  }

  void CallOnComplete(JS_LOCAL_VALUE answer, JS_LOCAL_VALUE family) {
    JS_ENTER_SCOPE();
    JS_DEFINE_STATE_MARKER(_com);
    commons* com = _com;
    JS_LOCAL_VALUE argv[3] = {STD_TO_INTEGER(0), answer, family};
    MakeCallback(com, object_, com->pstr_oncomplete, ARRAY_SIZE(argv), argv);
  }

  void ParseError(int status) {
    assert(status != ARES_SUCCESS);
    SetAresErrno(status);

    JS_ENTER_SCOPE();
    JS_DEFINE_STATE_MARKER(_com);
    commons* com = _com;

    JS_LOCAL_VALUE argv[1] = {STD_TO_INTEGER(-1)};
    MakeCallback(com, object_, com->pstr_oncomplete, ARRAY_SIZE(argv), argv);
  }

  // Subclasses should implement the appropriate Parse method.
  virtual void Parse(unsigned char* buf, int len) { assert(0); }

  virtual void Parse(struct hostent* host) { assert(0); }

 private:
  JS_PERSISTENT_OBJECT object_;
};

class QueryAWrap : public QueryWrap {
 public:
  int Send(const char* name) {
    commons* com = node::commons::getInstance();
    ares_query(com->_ares_channel, name, ns_c_in, ns_t_a, Callback,
               GetQueryArg());
    return 0;
  }

 protected:
  void Parse(unsigned char* buf, int len) {
    JS_ENTER_SCOPE();

    struct hostent* host;

    int status = ares_parse_a_reply(buf, len, &host, NULL, NULL);
    if (status != ARES_SUCCESS) {
      this->ParseError(status);
      return;
    }

    JS_LOCAL_ARRAY addresses = HostentToAddresses(host);
    ares_free_hostent(host);

    this->CallOnComplete(addresses);
  }
};

class QueryAaaaWrap : public QueryWrap {
 public:
  int Send(const char* name) {
    commons* com = node::commons::getInstance();
    ares_query(com->_ares_channel, name, ns_c_in, ns_t_aaaa, Callback,
               GetQueryArg());
    return 0;
  }

 protected:
  void Parse(unsigned char* buf, int len) {
    JS_ENTER_SCOPE();

    struct hostent* host;

    int status = ares_parse_aaaa_reply(buf, len, &host, NULL, NULL);
    if (status != ARES_SUCCESS) {
      this->ParseError(status);
      return;
    }

    JS_LOCAL_ARRAY addresses = HostentToAddresses(host);
    ares_free_hostent(host);

    this->CallOnComplete(addresses);
  }
};

class QueryCnameWrap : public QueryWrap {
 public:
  int Send(const char* name) {
    commons* com = node::commons::getInstance();
    ares_query(com->_ares_channel, name, ns_c_in, ns_t_cname, Callback,
               GetQueryArg());
    return 0;
  }

 protected:
  void Parse(unsigned char* buf, int len) {
    JS_ENTER_SCOPE_COM();
    JS_DEFINE_STATE_MARKER(com);
    struct hostent* host;

    int status = ares_parse_a_reply(buf, len, &host, NULL, NULL);
    if (status != ARES_SUCCESS) {
      this->ParseError(status);
      return;
    }

    // A cname lookup always returns a single record but we follow the
    // common API here.
    JS_LOCAL_ARRAY result = JS_NEW_ARRAY_WITH_COUNT(1);
    JS_INDEX_SET(result, 0, STD_TO_STRING(host->h_name));
    ares_free_hostent(host);

    this->CallOnComplete(result);
  }
};

class QueryMxWrap : public QueryWrap {
 public:
  int Send(const char* name) {
    commons* com = node::commons::getInstance();
    ares_query(com->_ares_channel, name, ns_c_in, ns_t_mx, Callback,
               GetQueryArg());
    return 0;
  }

 protected:
  void Parse(unsigned char* buf, int len) {
    JS_ENTER_SCOPE_COM();
    JS_DEFINE_STATE_MARKER(com);

    struct ares_mx_reply* mx_start;
    int status = ares_parse_mx_reply(buf, len, &mx_start);
    if (status != ARES_SUCCESS) {
      this->ParseError(status);
      return;
    }

    JS_LOCAL_ARRAY mx_records = JS_NEW_ARRAY();
    JS_LOCAL_STRING exchange_symbol = STD_TO_STRING("exchange");
    JS_LOCAL_STRING priority_symbol = STD_TO_STRING("priority");
    int i = 0;
    for (struct ares_mx_reply* mx_current = mx_start; mx_current;
         mx_current = mx_current->next) {
      JS_LOCAL_OBJECT mx_record = JS_NEW_EMPTY_OBJECT();
      JS_NAME_SET(mx_record, exchange_symbol, STD_TO_STRING(mx_current->host));
      JS_NAME_SET(mx_record, priority_symbol,
                  STD_TO_INTEGER(mx_current->priority));
      JS_INDEX_SET(mx_records, i++, mx_record);
    }

    ares_free_data(mx_start);

    this->CallOnComplete(mx_records);
  }
};

class QueryNsWrap : public QueryWrap {
 public:
  int Send(const char* name) {
    commons* com = node::commons::getInstance();
    ares_query(com->_ares_channel, name, ns_c_in, ns_t_ns, Callback,
               GetQueryArg());
    return 0;
  }

 protected:
  void Parse(unsigned char* buf, int len) {
    struct hostent* host;

    int status = ares_parse_ns_reply(buf, len, &host);
    if (status != ARES_SUCCESS) {
      this->ParseError(status);
      return;
    }

    JS_LOCAL_ARRAY names = HostentToNames(host);
    ares_free_hostent(host);

    this->CallOnComplete(names);
  }
};

class QueryTxtWrap : public QueryWrap {
 public:
  int Send(const char* name) {
    commons* com = node::commons::getInstance();
    ares_query(com->_ares_channel, name, ns_c_in, ns_t_txt, Callback,
               GetQueryArg());
    return 0;
  }

 protected:
  void Parse(unsigned char* buf, int len) {
    struct ares_txt_reply* txt_out;

    JS_ENTER_SCOPE_COM();
    JS_DEFINE_STATE_MARKER(com);

    int status = ares_parse_txt_reply(buf, len, &txt_out);
    if (status != ARES_SUCCESS) {
      this->ParseError(status);
      return;
    }

    JS_LOCAL_ARRAY txt_records = JS_NEW_ARRAY();

    struct ares_txt_reply* current = txt_out;
    for (int i = 0; current; ++i, current = current->next) {
      JS_LOCAL_STRING txt =
          STD_TO_STRING(reinterpret_cast<char*>(current->txt));
      JS_INDEX_SET(txt_records, i, txt);
    }

    ares_free_data(txt_out);

    this->CallOnComplete(txt_records);
  }
};

class QuerySrvWrap : public QueryWrap {
 public:
  int Send(const char* name) {
    commons* com = node::commons::getInstance();
    ares_query(com->_ares_channel, name, ns_c_in, ns_t_srv, Callback,
               GetQueryArg());
    return 0;
  }

 protected:
  void Parse(unsigned char* buf, int len) {
    JS_ENTER_SCOPE_COM();
    JS_DEFINE_STATE_MARKER(com);

    struct ares_srv_reply* srv_start;
    int status = ares_parse_srv_reply(buf, len, &srv_start);
    if (status != ARES_SUCCESS) {
      this->ParseError(status);
      return;
    }

    JS_LOCAL_ARRAY srv_records = JS_NEW_ARRAY();

    __JS_LOCAL_STRING name_symbol = JS_STRING_ID("name");
    __JS_LOCAL_STRING port_symbol = JS_STRING_ID("port");
    __JS_LOCAL_STRING priority_symbol = JS_STRING_ID("priority");
    __JS_LOCAL_STRING weight_symbol = JS_STRING_ID("weight");
    int i = 0;
    for (struct ares_srv_reply* srv_current = srv_start; srv_current;
         srv_current = srv_current->next) {
      JS_LOCAL_OBJECT srv_record = JS_NEW_EMPTY_OBJECT();
      JS_NAME_SET(srv_record, name_symbol, STD_TO_STRING(srv_current->host));
      JS_NAME_SET(srv_record, port_symbol, STD_TO_INTEGER(srv_current->port));
      JS_NAME_SET(srv_record, priority_symbol,
                  STD_TO_INTEGER(srv_current->priority));
      JS_NAME_SET(srv_record, weight_symbol,
                  STD_TO_INTEGER(srv_current->weight));

      JS_INDEX_SET(srv_records, i++, srv_record);
    }

    ares_free_data(srv_start);

    this->CallOnComplete(srv_records);
  }
};

class QueryNaptrWrap : public QueryWrap {
 public:
  int Send(const char* name) {
    commons* com = node::commons::getInstance();
    ares_query(com->_ares_channel, name, ns_c_in, ns_t_naptr, Callback,
               GetQueryArg());
    return 0;
  }

 protected:
  void Parse(unsigned char* buf, int len) {
    JS_ENTER_SCOPE_COM();
    JS_DEFINE_STATE_MARKER(com);

    ares_naptr_reply* naptr_start;
    int status = ares_parse_naptr_reply(buf, len, &naptr_start);

    if (status != ARES_SUCCESS) {
      this->ParseError(status);
      return;
    }

    JS_LOCAL_ARRAY naptr_records = JS_NEW_ARRAY();
    __JS_LOCAL_STRING flags_symbol = JS_STRING_ID("flags");
    __JS_LOCAL_STRING service_symbol = JS_STRING_ID("service");
    __JS_LOCAL_STRING regexp_symbol = JS_STRING_ID("regexp");
    __JS_LOCAL_STRING replacement_symbol = JS_STRING_ID("replacement");
    __JS_LOCAL_STRING order_symbol = JS_STRING_ID("order");
    __JS_LOCAL_STRING preference_symbol = JS_STRING_ID("preference");

    int i = 0;
    for (ares_naptr_reply* naptr_current = naptr_start; naptr_current;
         naptr_current = naptr_current->next) {
      JS_LOCAL_OBJECT naptr_record = JS_NEW_EMPTY_OBJECT();

      JS_NAME_SET(naptr_record, flags_symbol,
                  STD_TO_STRING(reinterpret_cast<char*>(naptr_current->flags)));
      JS_NAME_SET(
          naptr_record, service_symbol,
          STD_TO_STRING(reinterpret_cast<char*>(naptr_current->service)));
      JS_NAME_SET(
          naptr_record, regexp_symbol,
          STD_TO_STRING(reinterpret_cast<char*>(naptr_current->regexp)));
      JS_NAME_SET(naptr_record, replacement_symbol,
                  STD_TO_STRING(naptr_current->replacement));
      JS_NAME_SET(naptr_record, order_symbol,
                  STD_TO_INTEGER(naptr_current->order));
      JS_NAME_SET(naptr_record, preference_symbol,
                  STD_TO_INTEGER(naptr_current->preference));

      JS_INDEX_SET(naptr_records, i++, naptr_record);
    }

    ares_free_data(naptr_start);

    this->CallOnComplete(naptr_records);
  }
};

class GetHostByAddrWrap : public QueryWrap {
 public:
  int Send(const char* name) {
    int length, family;
    char address_buffer[sizeof(struct in6_addr)];

    commons* com = node::commons::getInstance();

    if (uv_inet_pton(AF_INET, name, &address_buffer).code == UV_OK) {
      length = sizeof(struct in_addr);
      family = AF_INET;
    } else if (uv_inet_pton(AF_INET6, name, &address_buffer).code == UV_OK) {
      length = sizeof(struct in6_addr);
      family = AF_INET6;
    } else {
      return ARES_ENOTIMP;
    }

    ares_gethostbyaddr(com->_ares_channel, address_buffer, length, family,
                       Callback, GetQueryArg());
    return 0;
  }

 protected:
  void Parse(struct hostent* host) {
    JS_ENTER_SCOPE();

    this->CallOnComplete(HostentToNames(host));
  }
};

class GetHostByNameWrap : public QueryWrap {
 public:
  int Send(const char* name, int family) {
    commons* com = node::commons::getInstance();
    ares_gethostbyname(com->_ares_channel, name, family, Callback,
                       GetQueryArg());
    return 0;
  }

 protected:
  void Parse(struct hostent* host) {
    JS_ENTER_SCOPE_COM();
    JS_DEFINE_STATE_MARKER(com);

    JS_LOCAL_ARRAY addresses = HostentToAddresses(host);
    JS_LOCAL_INTEGER family = STD_TO_INTEGER(host->h_addrtype);

    this->CallOnComplete(addresses, family);
  }
};

template <class Wrap>
static JS_LOCAL_METHOD(Query) {
  if (args.Length() < 2 || !args.IsFunction(1)) {
    THROW_TYPE_EXCEPTION(
        "This method expects at least 2 arguments.(String, Function..)");
  }

  Wrap* wrap = new Wrap();
  wrap->SetOnComplete(GET_ARG(1));

  // We must cache the wrap's js object here, because cares might make the
  // callback from the wrap->Send stack. This will destroy the wrap's internal
  // object reference, causing wrap->GetObject() to return undefined.
  JS_LOCAL_OBJECT object = JS_TYPE_TO_LOCAL_OBJECT(wrap->GetObject());

  jxcore::JXString jxs;
  args.GetString(0, &jxs);
  int r = wrap->Send(*jxs);
  if (r) {
    SetAresErrno(r);
    delete wrap;
    RETURN_PARAM(JS_NULL());
  } else {
    RETURN_PARAM(object);
  }
}
JS_METHOD_END

template <class Wrap>
static JS_LOCAL_METHOD(QueryWithFamily) {
  if (args.Length() < 3 || !args.IsFunction(2)) {
    THROW_TYPE_EXCEPTION(
        "This method expects at least 3 arguments.(String, Int, Function..)");
  }

  Wrap* wrap = new Wrap();
  wrap->SetOnComplete(GET_ARG(2));

  // We must cache the wrap's js object here, because cares might make the
  // callback from the wrap->Send stack. This will destroy the wrap's internal
  // object reference, causing wrap->GetObject() to return undefined.
  JS_LOCAL_OBJECT object = JS_TYPE_TO_LOCAL_OBJECT(wrap->GetObject());

  int family = args.GetInteger(1);

  jxcore::JXString jxs;
  args.GetString(0, &jxs);
  int r = wrap->Send(*jxs, family);
  if (r) {
    SetAresErrno(r);
    delete wrap;
    RETURN_PARAM(JS_NULL());
  } else {
    RETURN_PARAM(object);
  }
}
JS_METHOD_END

void AfterGetAddrInfo(uv_getaddrinfo_t* req, int status, struct addrinfo* res) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  GetAddrInfoReqWrap* req_wrap = (GetAddrInfoReqWrap*)req->data;

  JS_LOCAL_VALUE argv[1];

  if (status) {
    // Error
    SetErrno(uv_last_error(com->loop));
    argv[0] = JS_TYPE_TO_LOCAL_VALUE(JS_NULL());
  } else {
    // Success
    struct addrinfo* address;
    int n = 0;

    // Count the number of responses.
    for (address = res; address; address = address->ai_next) {
      n++;
    }

    // Create the response array.
    JS_LOCAL_ARRAY results = JS_NEW_ARRAY_WITH_COUNT(n);

    char ip[INET6_ADDRSTRLEN];
    const char* addr;

    n = 0;

    // Iterate over the IPv4 responses again this time creating javascript
    // strings for each IP and filling the results array.
    address = res;
    while (address) {
      assert(address->ai_socktype == SOCK_STREAM);

      // Ignore random ai_family types.
      if (address->ai_family == AF_INET) {
        // Juggle pointers
        addr = (char*)&((struct sockaddr_in*)address->ai_addr)->sin_addr;
        uv_err_t err =
            uv_inet_ntop(address->ai_family, addr, ip, INET6_ADDRSTRLEN);
        if (err.code != UV_OK) continue;

        // Create JavaScript string
        JS_LOCAL_STRING s = STD_TO_STRING(ip);
        JS_INDEX_SET(results, n, s);
        n++;
      }

      // Increment
      address = address->ai_next;
    }

    // Iterate over the IPv6 responses putting them in the array.
    address = res;
    while (address) {
      assert(address->ai_socktype == SOCK_STREAM);

      // Ignore random ai_family types.
      if (address->ai_family == AF_INET6) {
        // Juggle pointers
        addr = (char*)&((struct sockaddr_in6*)address->ai_addr)->sin6_addr;
        uv_err_t err =
            uv_inet_ntop(address->ai_family, addr, ip, INET6_ADDRSTRLEN);
        if (err.code != UV_OK) continue;

        // Create JavaScript string
        JS_LOCAL_STRING s = STD_TO_STRING(ip);
        JS_INDEX_SET(results, n, s);
        n++;
      }

      // Increment
      address = address->ai_next;
    }

    argv[0] = results;
  }

  uv_freeaddrinfo(res);

  // Make the callback into JavaScript
  MakeCallback(req_wrap->object_, com->pstr_oncomplete, ARRAY_SIZE(argv), argv);

  delete req_wrap;
}

static JS_LOCAL_METHOD(IsIP) {
  jxcore::JXString ip;
  args.GetString(0, &ip);

  char address_buffer[sizeof(struct in6_addr)];

  if (uv_inet_pton(AF_INET, *ip, &address_buffer).code == UV_OK) {
    RETURN_PARAM(STD_TO_INTEGER(4));
  }

  if (uv_inet_pton(AF_INET6, *ip, &address_buffer).code == UV_OK) {
    RETURN_PARAM(STD_TO_INTEGER(6));
  }

  RETURN_PARAM(STD_TO_INTEGER(0));
}
JS_METHOD_END

static JS_LOCAL_METHOD(GetAddrInfo) {
  if (!args.IsString(0)) {
    THROW_EXCEPTION("cares.GetAddrInfo requires first parameter string");
  }

  jxcore::JXString hostname;
  args.GetString(0, &hostname);

  int fam = AF_UNSPEC;
  if (args.IsNumber(1)) {
    switch (args.GetInteger(1)) {
      case 6:
        fam = AF_INET6;
        break;

      case 4:
        fam = AF_INET;
        break;
    }
  }

  GetAddrInfoReqWrap* req_wrap = new GetAddrInfoReqWrap(com);

  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = fam;
  hints.ai_socktype = SOCK_STREAM;

  int r = uv_getaddrinfo(com->loop, &req_wrap->req_, AfterGetAddrInfo,
                         *hostname, NULL, &hints);
  req_wrap->Dispatched();

  if (r) {
    SetErrno(uv_last_error(com->loop));
    delete req_wrap;
    RETURN_PARAM(JS_NULL());
  } else {
    RETURN_PARAM(req_wrap->object_);
  }
}
JS_METHOD_END

static DECLARE_CLASS_INITIALIZER(Initialize) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);
  bool ___NO_NEW_INSTANCE = true;
  int r;

  r = ares_library_init(ARES_LIB_INIT_ALL);
  assert(r == ARES_SUCCESS);

  struct ares_options options;
  memset(&options, 0, sizeof(options));
  options.flags = ARES_FLAG_NOCHECKRESP;
  options.sock_state_cb = ares_sockstate_cb;
  options.sock_state_cb_data = com->loop;

  /* We do the call to ares_init_option for caller. */
  r = ares_init_options(&com->_ares_channel, &options,
                        ARES_OPT_FLAGS | ARES_OPT_SOCK_STATE_CB);
  assert(r == ARES_SUCCESS);

  /* Initialize the timeout timer. The timer won't be started until the */
  /* first socket is opened. */
  uv_timer_init(com->loop, com->ares_timer);

  JS_SET_NAMED_METHOD(target, "queryA", Query<QueryAWrap>,
                      2);  // check parameter count later!!
  JS_SET_NAMED_METHOD(target, "queryAaaa", Query<QueryAaaaWrap>, 2);
  JS_SET_NAMED_METHOD(target, "queryCname", Query<QueryCnameWrap>, 2);
  JS_SET_NAMED_METHOD(target, "queryMx", Query<QueryMxWrap>, 2);
  JS_SET_NAMED_METHOD(target, "queryNs", Query<QueryNsWrap>, 2);
  JS_SET_NAMED_METHOD(target, "queryTxt", Query<QueryTxtWrap>, 2);
  JS_SET_NAMED_METHOD(target, "querySrv", Query<QuerySrvWrap>, 2);
  JS_SET_NAMED_METHOD(target, "queryNaptr", Query<QueryNaptrWrap>, 2);
  JS_SET_NAMED_METHOD(target, "getHostByAddr", Query<GetHostByAddrWrap>, 2);
  JS_SET_NAMED_METHOD(target, "getHostByName",
                      QueryWithFamily<GetHostByNameWrap>, 3);

  JS_SET_NAMED_METHOD(target, "getaddrinfo", GetAddrInfo, 2);
  JS_SET_NAMED_METHOD(target, "isIP", IsIP, 1);

  JS_NAME_SET(target, JS_STRING_ID("AF_INET"), STD_TO_INTEGER(AF_INET));
  JS_NAME_SET(target, JS_STRING_ID("AF_INET6"), STD_TO_INTEGER(AF_INET6));
  JS_NAME_SET(target, JS_STRING_ID("AF_UNSPEC"), STD_TO_INTEGER(AF_UNSPEC));
}

}  // namespace cares_wrap

}  // namespace node

NODE_MODULE(node_cares_wrap, node::cares_wrap::Initialize)
