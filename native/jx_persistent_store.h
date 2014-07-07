/*
 * jx_persistent_store.h
 *
 *  Copyright Nubisa Inc. 2014 All rights reserved.
 *
 *  License (MIT)
 *
 *  This header file is prepared for (and free to use and redistribute with) JXcore and Node.JS modules
 */

/*
 * USAGE:
 *
 * Simply replace
 *
 * static JS_PERSISTENT_FUNCTION_TEMPLATE SOMECLASS::constructor_template;
 *
 * To
 *
 * static ThreadStore<JS_PERSISTENT_FUNCTION_TEMPLATE > SOMECLASS::c_constructor_template;
 *
 * Whenever the constructor_template is needed;
 *
 * const int tid = c_constructor_template.getThreadId();
 *
 * c_constructor_template.templates[tid] // instead constructor_template
 */

#ifndef JX_PERSISTENT_STORE_H_
#define JX_PERSISTENT_STORE_H_

#if (NODE_MODULE_VERSION > 0x000B)
#define NODE12
#endif

#include "node.h"
#include <string.h>

namespace JX{
#ifndef JS_ENGINE_NO_V8
  #define JX_ISOLATE v8::Isolate*
  #define JX_CURRENT_ENGINE() v8::Isolate::GetCurrent()
  #ifdef NODE12
    #define JX_GET_ENGINE_DATA(x) x->GetData(0)
  #else
    #define JX_GET_ENGINE_DATA(x) x->GetData();
  #endif
#else
  #define JX_ISOLATE JS_ENGINE_MARKER
  #define JX_CURRENT_ENGINE() JS_CURRENT_ENGINE()
  #define JX_GET_ENGINE_DATA(x) JS_CURRENT_ENGINE_DATA(x)
#endif

#define MAX_JX_THREADS 64 //JXcore support max 64 v8 threads per process

template <class T>
class ThreadStore{
  static bool mted;

  void initStore(){
#ifndef USES_JXCORE_ENGINE
    templates = new T[1];
    mted = false;
#else
    mted = true;
    templates = new T[MAX_JX_THREADS];
#endif
  }


  int _threadId() const{
    if(!mted) return 0;

    JX_ISOLATE iso = JX_CURRENT_ENGINE();

    if(iso == NULL)
      return -2;

    void *id = JX_GET_ENGINE_DATA(iso);

    if(id == NULL) //it is NULL for Node.JS
    {
      return -1;
    }
    else{
      int tid = * ((int*)id);
      return tid;
    }
  }

public:
    T *templates;

    ThreadStore()
    {
      initStore();
    }


    ThreadStore(T val)
    {
      initStore();

      const int tc = mted ? MAX_JX_THREADS:1;
      
      for(int i=0;i<tc;i++){
        templates[i] = val;
      }
    }


    //JXcore keeps the thread id inside the isolate data slot
    int getThreadId(){
      const int tid = _threadId();
      if(tid<0)
        return 0;
      else
        return tid;
    }
  };

  template<class T>
  bool ThreadStore<T>::mted;
}

#endif /* JX_PERSISTENT_STORE_H_ */
