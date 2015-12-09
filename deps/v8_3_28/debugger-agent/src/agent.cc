// Updated for JXcore [Oguz Bastemur 2015]
// See original file at github.com/nodejs/nodejs

// Copyright Fedor Indutny and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "agent.h"
#include "debugger-agent.h"

#include "node.h"
#include "node_internals.h"  // ARRAY_SIZE
#include "v8.h"
#include "v8-debug.h"
#include "queue.h"

#include <string.h>

namespace node {
namespace debugger {

using v8::Context;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::Integer;
using v8::Isolate;
using v8::Local;
using v8::Locker;
using v8::Object;
using v8::String;
using v8::Value;

Agent::Agent(node::commons* com)
    : state_(kNone),
      port_(5858),
      wait_(false),
      parent_env_(com),
      child_env_(NULL),
      dispatch_handler_(NULL) {

  uv_sem_init(&start_sem_, 0);
  uv_mutex_init(&message_mutex_);

  QUEUE_INIT(&messages_);
}

Agent::~Agent() {
  Stop();

  uv_sem_destroy(&start_sem_);
  uv_mutex_destroy(&message_mutex_);

  // Clean-up messages
  while (!QUEUE_EMPTY(&messages_)) {
    QUEUE* q = QUEUE_HEAD(&messages_);
    QUEUE_REMOVE(q);
    AgentMessage* msg = ContainerOf(&AgentMessage::member, q);
    delete msg;
  }
}

bool Agent::Start(int port, bool wait) {
  int err;

  if (state_ == kRunning) return false;

  port_ = port;
  wait_ = wait;

  err = uv_thread_create(&thread_, ThreadCb, this);
  if (err != 0) goto thread_create_failed;

  uv_sem_wait(&start_sem_);

  state_ = kRunning;

  return true;

thread_create_failed:
  uv_close(reinterpret_cast<uv_handle_t*>(&child_signal_), NULL);

async_init_failed:
  uv_loop_delete(child_loop_);

loop_init_failed:
  return false;
}

void Agent::Enable() {
  v8::Debug::SetMessageHandler(MessageHandler);

  // Assign environment to the debugger's context
  // NOTE: The debugger context is created after `SetMessageHandler()` call
  // parent_env()->AssignToContext(v8::Debug::GetDebugContext());
  v8::Debug::GetDebugContext()->SetAlignedPointerInEmbedderData(
      NODE_CONTEXT_EMBEDDER_DATA_INDEX, parent_env_);
}

void Agent::Stop() {
  if (state_ != kRunning) {
    return;
  }

  v8::Debug::SetMessageHandler(NULL);

  // Send empty message to terminate things
  EnqueueMessage(new AgentMessage(NULL, 0));

  // Signal worker thread to make it stop
  uv_async_send(&child_signal_);

  uv_thread_join(&thread_);
  // CHECK_EQ(err, 0);

  uv_close(reinterpret_cast<uv_handle_t*>(&child_signal_), NULL);

  // uv_run(child_loop_, UV_RUN_NOWAIT);
  node::commons* com = node::commons::getInstanceByThreadId(63);
  uv_run_jx(com->loop, UV_RUN_NOWAIT, node::commons::CleanPinger, 63);

  state_ = kNone;
}

void Agent::WorkerRun() {
  {
    int threadId = 63;

    node::commons* com = node::commons::newInstance(threadId);

    JS_ENGINE_LOCKER();
    JS_SET_ENGINE_DATA(isolate, &com->threadId);
    JS_DEFINE_STATE_MARKER(com);

    JS_NEW_CONTEXT(context, isolate, NULL);
    v8::Context::Scope context_scope(context);

    JS_LOCAL_OBJECT global = context->Global();

    uv_idle_t* t = com->tick_spinner;
    uv_idle_init(com->loop, t);

    uv_check_init(com->loop, com->check_immediate_watcher);
    uv_unref((uv_handle_t*)com->check_immediate_watcher);
    uv_idle_init(com->loop, com->idle_immediate_dummy);

    child_loop_ = com->loop;
    uv_async_init(child_loop_, &child_signal_, ChildSignalCb);
    uv_unref(reinterpret_cast<uv_handle_t*>(&child_signal_));

    node::SetupProcessObject(threadId, true);
    JS_HANDLE_OBJECT process_l = com->getProcess();

    com->loop->loopId = threadId;

    child_env_ = com;

    // Expose API
    InitAdaptor(com);

    node::Load(process_l);

    uv_run_jx(com->loop, UV_RUN_DEFAULT, node::commons::CleanPinger, threadId);

    node::EmitExit(process_l);
    node::RunAtExit();

    com->Dispose();
    com->node_isolate->Dispose();
  }
  node::removeCommons();
}

void Agent::InitAdaptor(node::commons* com) {
  JS_ENTER_SCOPE_WITH(com->node_isolate);
  JS_DEFINE_STATE_MARKER(com);

  // Create API adaptor
  Local<FunctionTemplate> t = FunctionTemplate::New(__contextORisolate);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(STD_TO_STRING("DebugAPI"));

  NODE_SET_PROTOTYPE_METHOD(t, "notifyListen", NotifyListen);
  NODE_SET_PROTOTYPE_METHOD(t, "notifyWait", NotifyWait);
  NODE_SET_PROTOTYPE_METHOD(t, "sendCommand", SendCommand);

  Local<Object> api = t->GetFunction()->NewInstance();
  api->SetAlignedPointerInInternalField(0, this);

  JS_NAME_SET(api, STD_TO_STRING("port"), STD_TO_INTEGER(port_));

  JS_LOCAL_OBJECT process = JS_TYPE_TO_LOCAL_OBJECT(com->getProcess());
  JS_NAME_SET(process, STD_TO_STRING("_debugAPI"), api);
  api_.Reset(com->node_isolate, api);
}

Agent* Agent::Unwrap(const v8::FunctionCallbackInfo<v8::Value>& args) {
  void* ptr = args.Holder()->GetAlignedPointerFromInternalField(0);
  return reinterpret_cast<Agent*>(ptr);
}

void Agent::NotifyListen(const FunctionCallbackInfo<Value>& args) {
  Agent* a = Unwrap(args);

  // Notify other thread that we are ready to process events
  uv_sem_post(&a->start_sem_);
}

void Agent::NotifyWait(const FunctionCallbackInfo<Value>& args) {
  Agent* a = Unwrap(args);
  a->wait_ = false;

  int err = uv_async_send(&a->child_signal_);
}

void Agent::SendCommand(const FunctionCallbackInfo<Value>& args) {
  Agent* a = Unwrap(args);
  node::commons* com = a->child_env();
  HandleScope scope(com->node_isolate);

  String::Value v(args[0]);

  v8::Debug::SendCommand(a->parent_env()->node_isolate, *v, v.length());
  if (a->dispatch_handler_ != NULL) a->dispatch_handler_(a->parent_env());
}

void Agent::ThreadCb(void* agent) { ((Agent*)agent)->WorkerRun(); }

void Agent::ChildSignalCb(uv_async_t* signal, int status) {
  Agent* a = ContainerOf(&Agent::child_signal_, signal);
  commons* com = a->child_env();
  JS_DEFINE_STATE_MARKER(com);

  HandleScope scope(__contextORisolate);
  Local<Object> api = PersistentToLocal(__contextORisolate, a->api_);

  uv_mutex_lock(&a->message_mutex_);
  while (!QUEUE_EMPTY(&a->messages_)) {
    QUEUE* q = QUEUE_HEAD(&a->messages_);
    AgentMessage* msg = ContainerOf(&AgentMessage::member, q);

    // Time to close everything
    if (msg->data() == NULL) {
      QUEUE_REMOVE(q);
      delete msg;

      MakeCallback(com, api, STD_TO_STRING("onclose"), 0, NULL);
      break;
    }

    // Waiting for client, do not send anything just yet
    // TODO(indutny): move this to js-land
    if (a->wait_) break;

    QUEUE_REMOVE(q);
    Local<Value> argv[] = {String::NewFromTwoByte(
        __contextORisolate, msg->data(), String::kNormalString, msg->length())};

    // Emit message
    MakeCallback(com, api, STD_TO_STRING("onmessage"), ARRAY_SIZE(argv), argv);
    delete msg;
  }
  uv_mutex_unlock(&a->message_mutex_);
}

void Agent::EnqueueMessage(AgentMessage* message) {
  uv_mutex_lock(&message_mutex_);
  QUEUE_INSERT_TAIL(&messages_, &message->member);
  uv_mutex_unlock(&message_mutex_);
  uv_async_send(&child_signal_);
}

void Agent::MessageHandler(const v8::Debug::Message& message) {
  Isolate* isolate = message.GetIsolate();
  commons* com = commons::getInstanceIso(isolate);
  Agent* a = (Agent*)com->agent_;

  HandleScope scope(isolate);
  Local<String> json = message.GetJSON();
  String::Value v(json);

  AgentMessage* msg = new AgentMessage(*v, v.length());
  a->EnqueueMessage(msg);
}

}  // namespace debugger
}  // namespace node
