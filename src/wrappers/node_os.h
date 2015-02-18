// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_NODE_OS_H_
#define SRC_WRAPPERS_NODE_OS_H_

#include "node.h"

namespace node {

class OS {
  static DEFINE_JS_METHOD(GetEndianness);
  static DEFINE_JS_METHOD(GetHostname);
  static DEFINE_JS_METHOD(GetLoadAvg);
  static DEFINE_JS_METHOD(GetUptime);
  static DEFINE_JS_METHOD(GetTotalMemory);
  static DEFINE_JS_METHOD(GetFreeMemory);
  static DEFINE_JS_METHOD(GetCPUInfo);
  static DEFINE_JS_METHOD(GetOSType);
  static DEFINE_JS_METHOD(GetOSRelease);
  static DEFINE_JS_METHOD(GetInterfaceAddresses);

  INIT_CLASS_MEMBERS() {
    SET_CLASS_METHOD("getEndianness", GetEndianness, 0);
    SET_CLASS_METHOD("getHostname", GetHostname, 0);
    SET_CLASS_METHOD("getLoadAvg", GetLoadAvg, 0);
    SET_CLASS_METHOD("getUptime", GetUptime, 0);
    SET_CLASS_METHOD("getTotalMem", GetTotalMemory, 0);
    SET_CLASS_METHOD("getFreeMem", GetFreeMemory, 0);
    SET_CLASS_METHOD("getCPUs", GetCPUInfo, 0);
    SET_CLASS_METHOD("getOSType", GetOSType, 0);
    SET_CLASS_METHOD("getOSRelease", GetOSRelease, 0);
    SET_CLASS_METHOD("getInterfaceAddresses", GetInterfaceAddresses, 0);
  }
  END_INIT_MEMBERS
};

}  // namespace node

#endif  // SRC_WRAPPERS_NODE_OS_H_
