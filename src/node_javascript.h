// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_NODE_JAVASCRIPT_H_
#define SRC_NODE_JAVASCRIPT_H_

#include "jx/Proxy/JSEngine.h"

namespace node {

void JXDefineJavaScript();

void DefineJavaScript(JS_HANDLE_OBJECT target);

void MainSource(node::commons *com, jxcore::JXString *str);

}  // namespace node

#endif  // SRC_NODE_JAVASCRIPT_H_
