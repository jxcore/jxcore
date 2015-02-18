// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_CARES_WRAP_H_
#define SRC_WRAPPERS_CARES_WRAP_H_

#include "node.h"

namespace node {
namespace cares_wrap {

typedef class ReqWrap<uv_getaddrinfo_t> GetAddrInfoReqWrap;

struct ares_task_t {
  UV_HANDLE_FIELDS
  ares_socket_t sock;
  uv_poll_t poll_watcher;
  RB_ENTRY(ares_task_t) node;
};

}  // namespace cares_wrap
}  // namespace node

#endif  // SRC_WRAPPERS_CARES_WRAP_H_
