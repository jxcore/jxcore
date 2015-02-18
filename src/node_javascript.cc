// Copyright & License details are available under JXCORE_LICENSE file

#include "node.h"
#include "jx/memory_store.h"
#ifdef JXCORE_SOURCES_MINIFIED
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
      jxcore::node_native, sizeof(jxcore::node_native) - 1);
  str->set_std(reinterpret_cast<const char *>(_str), JS_GET_STATE_MARKER());
  free(_str);
#else
  str->set_std(jxcore::node_native, JS_GET_STATE_MARKER());
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
  std::string value = "throw 'jxcore is a global variable';";

  externalData *data = new externalData;
  data->type = EXTERNAL_DATA_STRING;
  data->str_data = value;

  XSpace::Store()->insert(std::make_pair(name, data));
  XSpace::UNLOCKSTORE();

  for (int i = 0; jxcore::natives[i].name; i++) {
    if (jxcore::natives[i].source != jxcore::node_native) {
      std::string name(jxcore::natives[i].name);
      data = new externalData;
      data->type = EXTERNAL_DATA_STRING;
#ifdef JXCORE_SOURCES_MINIFIED
      if (jxcore::natives[i].source_len > 0) {
        jxcore::mz_uint8 *str = jxcore::UncompressNative(
            jxcore::natives[i].source, jxcore::natives[i].source_len);
        std::string bt(reinterpret_cast<const char *>(str));
        free(str);
#else
      if (strcmp(jxcore::natives[i].name, "_jx_marker") != 0) {
        std::string bt(jxcore::natives[i].source,
                       jxcore::natives[i].source_len);
#endif
        data->str_data = bt;
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

        data->str_data = btn;
      }

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
          jxcore::natives[i].source, jxcore::natives[i].source_len);
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
