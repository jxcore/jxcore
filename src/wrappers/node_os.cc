// Copyright & License details are available under JXCORE_LICENSE file

#include "node_os.h"
#include "jx/commons.h"

#include <errno.h>
#include <string.h>

#ifdef __MINGW32__
#include <io.h>
#endif

#ifdef __POSIX__
#include <netdb.h>      // MAXHOSTNAMELEN on Solaris.
#include <unistd.h>     // gethostname, sysconf
#include <sys/param.h>  // MAXHOSTNAMELEN on Linux and the BSDs.
#include <sys/utsname.h>
#endif

// Add Windows fallback.
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

namespace node {

JS_METHOD(OS, GetEndianness) {
  int i = 1;
  bool big = (*(char*)&i) == 0;
  RETURN_PARAM(STD_TO_STRING(big ? "BE" : "LE"));
}
JS_METHOD_END

JS_METHOD(OS, GetHostname) {
  char buf[MAXHOSTNAMELEN + 1];

  if (gethostname(buf, sizeof(buf))) {
#ifdef __POSIX__
    JS_LOCAL_VALUE exception = ErrnoException(errno, "gethostname");
#else
    JS_LOCAL_VALUE exception = ErrnoException(WSAGetLastError(), "gethostname");
#endif
    THROW_EXCEPTION_OBJECT(exception);
  }
  buf[sizeof(buf) - 1] = '\0';

  RETURN_PARAM(STD_TO_STRING(buf));
}
JS_METHOD_END

JS_METHOD(OS, GetOSType) {
#ifdef __POSIX__
  struct utsname info;
  if (uname(&info) < 0) {
    JS_LOCAL_VALUE exception = ErrnoException(errno, "uname");
    THROW_EXCEPTION_OBJECT(exception);
  }
  RETURN_PARAM(STD_TO_STRING(info.sysname));
#else  // __MINGW32__
  RETURN_PARAM(STD_TO_STRING("Windows_NT"));
#endif
}
JS_METHOD_END

JS_METHOD(OS, GetOSRelease) {
#ifdef __POSIX__
  struct utsname info;
  if (uname(&info) < 0) {
    JS_LOCAL_VALUE exception = ErrnoException(errno, "uname");
    THROW_EXCEPTION_OBJECT(exception);
  }
  RETURN_PARAM(STD_TO_STRING(info.release));
#else  // __MINGW32__
  char release[256];
  OSVERSIONINFO info;
  info.dwOSVersionInfoSize = sizeof(info);

  if (GetVersionEx(&info) == 0) {
    return JS_UNDEFINED();
  }

  snprintf(release, sizeof(release), "%d.%d.%d",
           static_cast<int>(info.dwMajorVersion),
           static_cast<int>(info.dwMinorVersion),
           static_cast<int>(info.dwBuildNumber));
  RETURN_PARAM(STD_TO_STRING(release));
#endif
}
JS_METHOD_END

JS_METHOD(OS, GetCPUInfo) {
  uv_cpu_info_t* cpu_infos;
  int count, i;

  uv_err_t err = uv_cpu_info(&cpu_infos, &count);

  if (err.code != UV_OK) {
    RETURN_PARAM(JS_UNDEFINED());
  }

  JS_LOCAL_ARRAY cpus = JS_NEW_ARRAY();

  for (i = 0; i < count; i++) {
    uv_cpu_info_t* ci = cpu_infos + i;

    JS_LOCAL_OBJECT times_info = JS_NEW_EMPTY_OBJECT();
    JS_NAME_SET(times_info, JS_STRING_ID("user"),
                STD_TO_NUMBER(ci->cpu_times.user));
    JS_NAME_SET(times_info, JS_STRING_ID("nice"),
                STD_TO_NUMBER(ci->cpu_times.nice));
    JS_NAME_SET(times_info, JS_STRING_ID("sys"),
                STD_TO_NUMBER(ci->cpu_times.sys));
    JS_NAME_SET(times_info, JS_STRING_ID("idle"),
                STD_TO_NUMBER(ci->cpu_times.idle));
    JS_NAME_SET(times_info, JS_STRING_ID("irq"),
                STD_TO_NUMBER(ci->cpu_times.irq));

    JS_LOCAL_OBJECT cpu_info = JS_NEW_EMPTY_OBJECT();
    JS_NAME_SET(cpu_info, JS_STRING_ID("model"), STD_TO_STRING(ci->model));
    JS_NAME_SET(cpu_info, JS_STRING_ID("speed"), STD_TO_NUMBER(ci->speed));
    JS_NAME_SET(cpu_info, JS_STRING_ID("times"), times_info);

    JS_INDEX_SET(cpus, i, cpu_info);
  }

  uv_free_cpu_info(cpu_infos, count);

  RETURN_POINTER(cpus);
}
JS_METHOD_END

JS_METHOD(OS, GetFreeMemory) {
  double amount = uv_get_free_memory();

  if (amount < 0) {
    RETURN_PARAM(JS_UNDEFINED());
  }

  RETURN_PARAM(STD_TO_NUMBER(amount));
}
JS_METHOD_END

JS_METHOD(OS, GetTotalMemory) {
  double amount = uv_get_total_memory();

  if (amount < 0) {
    RETURN_PARAM(JS_UNDEFINED());
  }

  RETURN_PARAM(STD_TO_NUMBER(amount));
}
JS_METHOD_END

JS_METHOD(OS, GetUptime) {
  double uptime;

  uv_err_t err = uv_uptime(&uptime);

  if (err.code != UV_OK) {
    RETURN_PARAM(JS_UNDEFINED());
  }

  RETURN_PARAM(STD_TO_NUMBER(uptime));
}
JS_METHOD_END

JS_METHOD(OS, GetLoadAvg) {
  double loadavg[3];
  uv_loadavg(loadavg);

  JS_LOCAL_ARRAY loads = JS_NEW_ARRAY_WITH_COUNT(3);
  JS_INDEX_SET(loads, 0, STD_TO_NUMBER(loadavg[0]));
  JS_INDEX_SET(loads, 1, STD_TO_NUMBER(loadavg[1]));
  JS_INDEX_SET(loads, 2, STD_TO_NUMBER(loadavg[2]));

  RETURN_POINTER(loads);
}
JS_METHOD_END

JS_METHOD(OS, GetInterfaceAddresses) {
  uv_interface_address_t* interfaces;
  int count, i;
  char ip[INET6_ADDRSTRLEN];
  JS_LOCAL_OBJECT ret, o;
  JS_LOCAL_STRING name, family;
  JS_LOCAL_ARRAY ifarr;

  uv_err_t err = uv_interface_addresses(&interfaces, &count);

  ret = JS_NEW_EMPTY_OBJECT();

  if (err.code == UV_ENOSYS) {
    error_console("UV_ENOSYS");
    RETURN_POINTER(ret);
  }

  if (err.code != UV_OK) {
    error_console("!UV_OK");
    JS_LOCAL_VALUE exception = UVException(err.code, "uv_interface_addresses");
    THROW_EXCEPTION_OBJECT(exception);
  }

  for (i = 0; i < count; i++) {
    name = STD_TO_STRING(interfaces[i].name);
    if (JS_HAS_NAME(ret, name)) {
      ifarr = JS_CAST_ARRAY(JS_GET_NAME(ret, name));
    } else {
      ifarr = JS_NEW_ARRAY();
      JS_NAME_SET(ret, name, ifarr);
    }

    if (interfaces[i].address.address4.sin_family == AF_INET) {
      uv_ip4_name(&interfaces[i].address.address4, ip, sizeof(ip));
      family = STD_TO_STRING("IPv4");
    } else if (interfaces[i].address.address4.sin_family == AF_INET6) {
      uv_ip6_name(&interfaces[i].address.address6, ip, sizeof(ip));
      family = STD_TO_STRING("IPv6");
    } else {
      strncpy(ip, "<unknown sa family>", INET6_ADDRSTRLEN);
      family = STD_TO_STRING("<unknown>");
    }

    o = JS_NEW_EMPTY_OBJECT();
    JS_NAME_SET(o, JS_STRING_ID("address"), STD_TO_STRING(ip));
    JS_NAME_SET(o, JS_STRING_ID("family"), family);

    const bool internal = interfaces[i].is_internal;

    JS_LOCAL_VALUE bval =
        internal ? STD_TO_BOOLEAN(true) : STD_TO_BOOLEAN(false);
    JS_NAME_SET(o, JS_STRING_ID("internal"), bval);

    JS_INDEX_SET(ifarr, JS_GET_ARRAY_LENGTH(ifarr), o);
  }

  uv_free_interface_addresses(interfaces, count);

  RETURN_POINTER(ret);
}
JS_METHOD_END

}  // namespace node

NODE_MODULE(node_os, node::OS::Initialize)
