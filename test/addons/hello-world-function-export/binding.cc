#include <node.h>
#include <v8.h>

using namespace v8;

Handle<Value> Method(const Arguments& args) {
  HandleScope scope;
  return scope.Close(String::New("world"));
}

void init(Handle<Object> exports, Handle<Object> module) {
  JS_METHOD_SET(module, "exports", Method);
}

NODE_MODULE(binding, init);
