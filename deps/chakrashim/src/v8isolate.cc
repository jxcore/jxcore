// Copyright Microsoft. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and / or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#include "v8.h"
#include "v8-profiler.h"
#include "jsrtutils.h"

namespace v8 {

HeapProfiler dummyHeapProfiler;
CpuProfiler dummyCpuProfiler;

Isolate* Isolate::New() {
  return jsrt::IsolateShim::New();
}

Isolate *Isolate::GetCurrent() {
  return jsrt::IsolateShim::GetCurrentAsIsolate();
}

void Isolate::Enter() {
  return jsrt::IsolateShim::FromIsolate(this)->Enter();
}

void Isolate::Exit() {
  // CONSIDER: Isolates are only used for debugging
  return jsrt::IsolateShim::FromIsolate(this)->Exit();
}

void Isolate::Dispose() {
  // CHAKRA-TODO: Handle Error?
  jsrt::IsolateShim::FromIsolate(this)->Dispose();
}

int64_t Isolate::AdjustAmountOfExternalAllocatedMemory(
    int64_t change_in_bytes) {
  // CHAKRA-TODO: We don't support adding external memory pressure at the
  // moment.
  return 0;
}

void Isolate::SetData(uint32_t slot, void* data) {
  return jsrt::IsolateShim::FromIsolate(this)->SetData(slot, data);
}

void* Isolate::GetData(uint32_t slot) {
  return jsrt::IsolateShim::FromIsolate(this)->GetData(slot);
}

uint32_t Isolate::GetNumberOfDataSlots() {
  return 0;
}

Local<Context> Isolate::GetCurrentContext() {
  return Context::GetCurrent();
}

void Isolate::RunMicrotasks() {
}

void Isolate::SetAutorunMicrotasks(bool autorun) {
}

Local<Value> Isolate::ThrowException(Local<Value> exception) {
  JsSetException(*exception);
  return Undefined(this);
}

HeapProfiler* Isolate::GetHeapProfiler() {
  return &dummyHeapProfiler;
}

CpuProfiler* Isolate::GetCpuProfiler() {
  return &dummyCpuProfiler;
}

void Isolate::AddGCPrologueCallback(
    GCPrologueCallback callback, GCType gc_type_filter) {
}

void Isolate::RemoveGCPrologueCallback(GCPrologueCallback callback) {
}

void Isolate::AddGCEpilogueCallback(
    GCEpilogueCallback callback, GCType gc_type_filter) {
}

void Isolate::RemoveGCEpilogueCallback(GCEpilogueCallback callback) {
}

void Isolate::SetCounterFunction(CounterLookupCallback) {
  CHAKRA_UNIMPLEMENTED();
}

void Isolate::SetCreateHistogramFunction(CreateHistogramCallback) {
  CHAKRA_UNIMPLEMENTED();
}

void Isolate::SetAddHistogramSampleFunction(AddHistogramSampleCallback) {
  CHAKRA_UNIMPLEMENTED();
}

bool Isolate::IdleNotification(int idle_time_in_ms) {
  // CHAKRA_UNIMPLEMENTED();
  return false;
}

void Isolate::LowMemoryNotification() {
  // TODO implement
  // CHAKRA_UNIMPLEMENTED();
}

int Isolate::ContextDisposedNotification() {
  CHAKRA_UNIMPLEMENTED();
  return 0;
}

void Isolate::GetHeapStatistics(HeapStatistics *heap_statistics) {
  size_t memoryUsage;
  if (!jsrt::IsolateShim::FromIsolate(this)->GetMemoryUsage(&memoryUsage)) {
    return;
  }
  // CONSIDER: V8 distinguishes between "total" size and "used" size
  heap_statistics->set_heap_size(memoryUsage);
}

}  // namespace v8
