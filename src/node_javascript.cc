// Copyright & License details are available under JXCORE_LICENSE file

#include "node.h"
#include "jx/memory_store.h"
#ifdef JXCORE_SOURCES_MINIFIED
#include "jx/commons.h"
#include "jx/jxp_compress.h"
#include "jx_natives.h"
#else
#include "node_natives.h"
#endif

#include <string.h>
#if !defined(_MSC_VER)
#include <strings.h>
#endif

#include <string>
#include <stdlib.h>

namespace node {

void MainSource(node::commons *com, jxcore::JXString *str) {
  JS_DEFINE_STATE_MARKER(com);
#ifdef JXCORE_SOURCES_MINIFIED
  jxcore::mz_uint8 *_str = jxcore::UncompressNative(
      com, jxcore::node_native, sizeof(jxcore::node_native) - 1);
  const char *cstr = reinterpret_cast<const char *>(_str);
  str->SetFromSTD(cstr, strlen(cstr), JS_GET_STATE_MARKER());
  free(_str);
#else
  str->SetFromSTD(jxcore::node_native, strlen(jxcore::node_native),
                  JS_GET_STATE_MARKER());
#endif
}

void JXDefineJavaScript() {
  XSpace::LOCKSTORE();
  if (XSpace::Store() == NULL) return;

  _StringStore::const_iterator it = XSpace::Store()->find("jxcore");
  if (it != XSpace::Store()->end()) {
    XSpace::UNLOCKSTORE();
    return;
  }

  std::string name = "jxcore";
  std::string value = "throw new Error('jxcore is a global variable');";
  char *tmp = (char *)malloc(sizeof(char) * value.length() + 1);
  memcpy(tmp, value.c_str(), sizeof(char) * value.length());
  tmp[value.length()] = char(0);

  MAP_HOST_DATA jdata;
  jdata.length_ = value.length();
  jdata.data_ = tmp;

  XSpace::Store()->insert(std::make_pair(name, jdata));
  XSpace::UNLOCKSTORE();
#ifdef JXCORE_SOURCES_MINIFIED
  node::commons *com = node::commons::getInstance();
#endif

  for (int i = 0; jxcore::natives[i].name; i++) {
    if (jxcore::natives[i].source != jxcore::node_native) {
      std::string name(jxcore::natives[i].name);

      MAP_HOST_DATA data;
      char *tmp;
#ifdef JXCORE_SOURCES_MINIFIED
      if (jxcore::natives[i].source_len > 0) {
        jxcore::mz_uint8 *str = jxcore::UncompressNative(
            com, jxcore::natives[i].source, jxcore::natives[i].source_len);
        std::string bt(reinterpret_cast<const char *>(str));
        free(str);
#else
      if (strcmp(jxcore::natives[i].name, "_jx_marker") != 0) {
        std::string bt(jxcore::natives[i].source,
                       jxcore::natives[i].source_len);
#endif
        tmp = (char *)malloc(sizeof(char) * bt.length() + 1);
        memcpy(tmp, bt.c_str(), sizeof(char) * bt.length());
        tmp[bt.length()] = char(0);
        data.length_ = bt.length();
      } else {  // _jx_marker
        std::string bt(jxcore::natives[i].source);
        std::string btn = "exports.mark='";
        const char *cstr = bt.c_str();
        for (int i = 14; i < 56; i++)
          if (cstr[i] != ')')
            btn += cstr[i];
          else
            break;
        btn += "';";

        tmp = (char *)malloc(sizeof(char) * btn.length() + 1);
        memcpy(tmp, btn.c_str(), sizeof(char) * btn.length());
        tmp[btn.length()] = char(0);
        data.length_ = btn.length();
      }
      data.data_ = tmp;

      XSpace::Store()->insert(std::make_pair(name, data));
    }
  }
}

void DefineJavaScript(JS_HANDLE_OBJECT target) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  for (int i = 0; jxcore::natives[i].name; i++) {
    if (strcmp(jxcore::natives[i].name, "config") == 0) continue;
    const char *name = jxcore::natives[i].name;

    if (name[0] == '_' && name[1] == 'j' && name[2] == 'x') continue;

    if (jxcore::natives[i].source != jxcore::node_native) {
      JS_LOCAL_STRING name = STD_TO_STRING(jxcore::natives[i].name);
#ifdef JXCORE_SOURCES_MINIFIED
      jxcore::mz_uint8 *str = jxcore::UncompressNative(
          com, jxcore::natives[i].source, jxcore::natives[i].source_len);
      std::string bt(reinterpret_cast<const char *>(str));
      free(str);
#else
      std::string bt(jxcore::natives[i].source);
#endif
      JS_HANDLE_STRING source =
          STD_TO_STRING_WITH_LENGTH(bt.c_str(), bt.length());
      JS_NAME_SET(target, name, source);
    }
  }
}

}  // namespace node
