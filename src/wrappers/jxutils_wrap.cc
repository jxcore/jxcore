// Copyright & License details are available under JXCORE_LICENSE file

#if defined(_MSC_VER)
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x) * 1000)
#endif
#include <climits>
#include "jxutils_wrap.h"
#include "thread_wrap.h"
#include "jx/memory_store.h"
#include "jx/jxp_compress.h"

namespace node {

static uint64_t unique_id = 0;
static int loops[MAX_JX_THREADS];

std::string JXUtilsWrap::exec(const char *cmd, int *ec) {
#if defined(_WIN32)
  FILE *pipe = _popen(cmd, "r");
#else
  FILE *pipe = popen(cmd, "r");
#endif
  if (!pipe) return "execSync couldn't create the pipe";
  char buffer[256];
  std::string result = "";
  while (fgets(buffer, 256, pipe)) {
    result += buffer;
  }
// need checking ferror ?
#if defined(_WIN32)
  *ec = _pclose(pipe);
#else
  *ec = pclose(pipe);
  if (*ec > 256 && *ec < 65536) {
    *ec = (*ec - 65536) / 256;
  }
#endif
  return result;
}

JS_METHOD(JXUtilsWrap, ExecSystem) {
  if (!args.IsString(0)) {
    THROW_EXCEPTION("Missing parameters (execSystem) expects (string).");
  }

  if (!commons::CanSysExec()) {
    THROW_EXCEPTION("This process is restricted for calling system commands");
  }

  int ec = -1;
  if (system(NULL)) {
    jxcore::JXString jxs;
    args.GetString(0, &jxs);
    ec = system(*jxs);
  }
  RETURN_PARAM(STD_TO_INTEGER(ec));
}
JS_METHOD_END

JS_METHOD(JXUtilsWrap, ExecSync) {
  if (!args.IsString(0)) {
    THROW_EXCEPTION("Missing parameters (execSync) expects (string).");
  }

  if (!commons::CanSysExec()) {
    THROW_EXCEPTION("This process is restricted for calling system commands");
  }

  int ec = 0;
  jxcore::JXString jxs;
  args.GetString(0, &jxs);
  std::string str = exec(*jxs, &ec);

  JS_LOCAL_ARRAY arr = JS_NEW_ARRAY();
  JS_INDEX_SET(arr, 0, UTF8_TO_STRING(str.c_str()));
  JS_INDEX_SET(arr, 1, STD_TO_INTEGER(ec));

  RETURN_POINTER(arr);
}
JS_METHOD_END

JS_METHOD(JXUtilsWrap, BeforeApplicationStart) {
  if (args.IsNull(0)) {
    THROW_EXCEPTION(
        "Warning! This method is for internal usage. Missing parameters "
        "(beforeApplicationStart) expects (object).");
  }

  JS_LOCAL_OBJECT jxconfig = JS_VALUE_TO_OBJECT(GET_ARG(0));

  int portTCP = -2, portTCPS = -2, maxMemory = -2, maxCPU = -1,
      maxCPUInterval = 2;
  bool allowSysExec = true, allowCustomSocketPort = true,
       allowLocalNativeModules = true, allowMonitoringAPI = true;
  std::string globalModulePath = ".";

  __JS_LOCAL_STRING str_ptc = JS_STRING_ID("portTCP");
  if (JS_HAS_NAME(jxconfig, str_ptc)) {
    JS_LOCAL_VALUE obj_tcp = JS_GET_NAME(jxconfig, str_ptc);
    if (!JS_IS_NULL_OR_UNDEFINED(obj_tcp)) {
      if (JS_IS_NUMBER(obj_tcp)) {
        portTCP = INTEGER_TO_STD(obj_tcp);
      }
    }
  }

  __JS_LOCAL_STRING str_ptcs = JS_STRING_ID("portTCPS");
  if (JS_HAS_NAME(jxconfig, str_ptcs)) {
    JS_LOCAL_VALUE obj_tcps = JS_GET_NAME(jxconfig, str_ptcs);
    if (!JS_IS_NULL_OR_UNDEFINED(obj_tcps)) {
      if (JS_IS_NUMBER(obj_tcps)) {
        portTCPS = INTEGER_TO_STD(obj_tcps);
      }
    }
  }

  __JS_LOCAL_STRING str_acsp = JS_STRING_ID("allowCustomSocketPort");
  if (JS_HAS_NAME(jxconfig, str_acsp)) {
    JS_LOCAL_VALUE obj_allowCustomSocketPort = JS_GET_NAME(jxconfig, str_acsp);
    if (!JS_IS_NULL_OR_UNDEFINED(obj_allowCustomSocketPort)) {
      if (JS_IS_BOOLEAN(obj_allowCustomSocketPort)) {
        allowCustomSocketPort = BOOLEAN_TO_STD(obj_allowCustomSocketPort);
      }
    }
  }

  commons::SetPortBoundaries(portTCP, portTCPS, allowCustomSocketPort);

  __JS_LOCAL_STRING str_ama = JS_STRING_ID("allowMonitoringAPI");
  if (JS_HAS_NAME(jxconfig, str_ama)) {
    JS_LOCAL_VALUE obj_allowMonitoringAPI = JS_GET_NAME(jxconfig, str_ama);
    if (!JS_IS_NULL_OR_UNDEFINED(obj_allowMonitoringAPI)) {
      if (JS_IS_BOOLEAN(obj_allowMonitoringAPI)) {
        allowMonitoringAPI = BOOLEAN_TO_STD(obj_allowMonitoringAPI);
      }
    }
  }

  commons::SetMonitoringAPI(allowMonitoringAPI);

  __JS_LOCAL_STRING str_mm = JS_STRING_ID("maxMemory");
  if (JS_HAS_NAME(jxconfig, str_mm)) {
    JS_LOCAL_VALUE obj_memory = JS_GET_NAME(jxconfig, str_mm);
    if (!JS_IS_NULL_OR_UNDEFINED(obj_memory)) {
      if (JS_IS_NUMBER(obj_memory)) {
        maxMemory = INTEGER_TO_STD(obj_memory) * 1024;  // KB
      }
    }
  }

  commons::SetMaxMemory(maxMemory);

  __JS_LOCAL_STRING str_ase = JS_STRING_ID("allowSysExec");
  if (JS_HAS_NAME(jxconfig, str_ase)) {
    JS_LOCAL_VALUE obj_sysExec = JS_GET_NAME(jxconfig, str_ase);
    if (!JS_IS_NULL_OR_UNDEFINED(obj_sysExec)) {
      if (JS_IS_BOOLEAN(obj_sysExec)) {
        allowSysExec = BOOLEAN_TO_STD(obj_sysExec);
      }
    }
  }

  commons::SetSysExec(allowSysExec ? 1 : 0);

  __JS_LOCAL_STRING str_mc = JS_STRING_ID("maxCPU");
  if (JS_HAS_NAME(jxconfig, str_mc)) {
    JS_LOCAL_VALUE obj_maxCPU = JS_GET_NAME(jxconfig, str_mc);
    if (!JS_IS_NULL_OR_UNDEFINED(obj_maxCPU)) {
      if (JS_IS_NUMBER(obj_maxCPU)) {
        maxCPU = INTEGER_TO_STD(obj_maxCPU);
      }
    }
  }

  __JS_LOCAL_STRING str_mci = JS_STRING_ID("maxCPUInterval");
  if (JS_HAS_NAME(jxconfig, str_mci)) {
    JS_LOCAL_VALUE obj_maxCPUInterval = JS_GET_NAME(jxconfig, str_mci);
    if (!JS_IS_NULL_OR_UNDEFINED(obj_maxCPUInterval)) {
      if (JS_IS_NUMBER(obj_maxCPUInterval)) {
        maxCPUInterval = INTEGER_TO_STD(obj_maxCPUInterval);
        if (maxCPUInterval <= 0) {
          error_console(
              "maxCPUInterval can not be <= 0. The default value is active now "
              "(2 secs).");
          maxCPUInterval = 2;
        }
      }
    }
  }

  commons::SetMaxCPU(maxCPU, maxCPUInterval);

  __JS_LOCAL_STRING str_alnm = JS_STRING_ID("allowLocalNativeModules");
  if (JS_HAS_NAME(jxconfig, str_alnm)) {
    JS_LOCAL_VALUE obj_allowLocalNativeModules =
        JS_GET_NAME(jxconfig, str_alnm);
    if (!JS_IS_NULL_OR_UNDEFINED(obj_allowLocalNativeModules)) {
      if (JS_IS_BOOLEAN(obj_allowLocalNativeModules)) {
        allowLocalNativeModules = BOOLEAN_TO_STD(obj_allowLocalNativeModules);
      }
    }
  }

  commons::SetAllowLocalNativeModules(allowLocalNativeModules ? 1 : 0);

  __JS_LOCAL_STRING str_gmp = JS_STRING_ID("globalModulePath");
  if (JS_HAS_NAME(jxconfig, str_gmp)) {
    JS_LOCAL_VALUE obj_globalModulePath = JS_GET_NAME(jxconfig, str_gmp);
    if (!JS_IS_NULL_OR_UNDEFINED(obj_globalModulePath)) {
      if (JS_IS_STRING(obj_globalModulePath)) {
        globalModulePath = STRING_TO_STD(obj_globalModulePath);
      }
    }
  }

  commons::SetGlobalModulePath(globalModulePath);
}
JS_METHOD_END

JS_METHOD(JXUtilsWrap, PrintLog) {
  if (!args.IsString(0)) {
    THROW_EXCEPTION("Missing parameters (print) expects (string).");
  }

  jxcore::JXString str_key;
  args.GetString(0, &str_key);
  log_console("%s\n", *str_key);
#ifndef __MOBILE_OS__
  fflush(stdout);
#endif
}
JS_METHOD_END

JS_METHOD(JXUtilsWrap, PrintError) {
  if (!args.IsString(0)) {
    THROW_EXCEPTION("Missing parameters (print_err) expects (string).");
  }

  jxcore::JXString str_key;
  args.GetString(0, &str_key);
  if (args.GetBoolean(1))
    error_console("%s\n", *str_key);
  else
    warn_console("%s\n", *str_key);
#ifndef __ANDROID__
  fflush(stdout);
#endif
}
JS_METHOD_END

JS_METHOD(JXUtilsWrap, SetMaxHeaderLength) {
  if (!args.IsNumber(0)) {
    THROW_EXCEPTION("Missing parameters (setMaxHeaderLength) expects (int).");
  }

  commons::max_header_size = args.GetInteger(0);
}
JS_METHOD_END

JS_METHOD(JXUtilsWrap, GetUniqueNext) {
  auto_lock locker_(CSLOCK_UNIQUEID);
  uint64_t val = unique_id++;
  unique_id %= ULONG_MAX;

  RETURN_PARAM(STD_TO_NUMBER(val));
}
JS_METHOD_END

JS_METHOD(JXUtilsWrap, GetCPU) {
  if (args.Length() < 2 || !args.IsNumber(0) || !args.IsInteger(1)) {
    THROW_EXCEPTION(
        "Wrong parameters. JXUtilsWrap::GetCPU expects (long, int)");
  }

  int64_t timer = args.GetInteger(0);
  int64_t diff = args.GetInteger(1);

  double usage = node::commons::GetCPUUsage(timer, diff);

  RETURN_PARAM(STD_TO_NUMBER(usage));
}
JS_METHOD_END

JS_METHOD(JXUtilsWrap, RunLoop) {
  if (args.Length() == 0) {
    loops[com->threadId] = 0;
    RETURN_PARAM(STD_TO_BOOLEAN(true));
  }
  int sleep_time = args.GetInteger(0), r = 1;
  if (loops[com->threadId] == 1) {
    RETURN_PARAM(STD_TO_BOOLEAN(false));
  }
  uv_run_mode op = UV_RUN_PAUSE;
  if (sleep_time == -1) {
    op = UV_RUN_ONCE;
  }

  loops[com->threadId] = 1;

  while (r != 0 && loops[com->threadId] != 0) {
    Sleep(sleep_time == -1 ? 1 : sleep_time);

    r = uv_run_jx(com->loop, op, node::commons::CleanPinger, com->threadId);
  }

  RETURN_PARAM(STD_TO_BOOLEAN(r == 0 ? false : true));
}
JS_METHOD_END

JS_METHOD(JXUtilsWrap, SetSourceExpiration) {
  if (XSpace::Timers() == NULL) {
    RETURN();
  }

  if (!args.IsString(0) || !args.IsNumber(1)) {
    THROW_EXCEPTION(
        "Missing parameters (expirationSource) expects (string, int).");
  }

  jxcore::JXString jxs;
  args.GetString(0, &jxs);
  std::string jxss(*jxs);

  int64_t tmo = args.GetInteger(1);

  XSpace::LOCKTIMERS();
  _timerStore *timers = XSpace::Timers();
  if (timers != NULL) {
    timers->erase(jxss);
    ttlTimer timer;
    timer.slice = tmo * 1000000;
    timer.start = uv_hrtime();
    timers->insert(std::make_pair(jxss, timer));
  }
  XSpace::SetHasKey(true);
  XSpace::UNLOCKTIMERS();
}
JS_METHOD_END

JS_METHOD(JXUtilsWrap, Compress) {
  if (!args.IsString(0)) {
    THROW_TYPE_EXCEPTION("compress methods expects a string argument");
  }

  jxcore::JXString jxs;
  int len = args.GetString(0, &jxs);

  node::Buffer *buff = jxcore::CompressString(com, *jxs, jxs.Utf8Length());

  if (buff == NULL) RETURN_PARAM(STD_TO_BOOLEAN(false));

  RETURN_PARAM(JS_VALUE_TO_OBJECT(buff->handle_));
}
JS_METHOD_END

JS_METHOD(JXUtilsWrap, Uncompress) {
  if (args.IsNull(0) || args.IsUndefined(0)) {
    jxcore::RemoveCache();
    RETURN();
  }

  JS_LOCAL_OBJECT obj = JS_VALUE_TO_OBJECT(GET_ARG(0));

  const unsigned long len = BUFFER__LENGTH(obj);
  if (args.Length() > 1) {
    if (!jxcore::RaiseCache((len * 2) + 1)) {
      THROW_EXCEPTION("Not enough memory!");
    }
  }

  node::Buffer *buff = jxcore::UncompressString(com, obj, len);

  if (buff == NULL) RETURN_PARAM(STD_TO_BOOLEAN(false));

  RETURN_PARAM(JS_VALUE_TO_OBJECT(buff->handle_));
}
JS_METHOD_END

}  // namespace node

NODE_MODULE(node_jxutils_wrap, node::JXUtilsWrap::Initialize)
