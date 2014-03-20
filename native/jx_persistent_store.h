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
 * static Persistent<FunctionTemplate> SOMECLASS::constructor_template;
 *
 * To
 *
 * static ThreadStore<Persistent<FunctionTemplate> > SOMECLASS::c_constructor_template;
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

#include "v8.h"
#include "node.h"
#include <string.h>

namespace JX{

using v8::Isolate;

#define MAX_JX_THREADS 64 //JXcore support max 64 v8 threads per process

template <class T>
class ThreadStore{
  static bool mted, checked;


  void initStore(){
    if(!ThreadStore::checked){
	  checked = true;
      ThreadStore::mted = node::get_builtin_module("JX") != NULL; // referenced from Node.H
	}

	if(ThreadStore::mted){
	  templates = new T[MAX_JX_THREADS];
	}
	else{
      templates = new T[1];
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
    if(!ThreadStore::mted)
      return 0;

    Isolate *iso = Isolate::GetCurrent(); //reintroduced back to 3.24.x / 3.25

    if(iso == NULL)
      return 0;
#ifdef NODE12
	  void *id = iso->GetData(0);
#else
	  void *id = iso->GetData();
#endif

	  if(id == NULL) //it is NULL for Node.JS
	    return 0; // main thread
	  else
	    return *((int*)id);
    }
  };

  template<class T>
  bool ThreadStore<T>::mted;

  template<class T>
  bool ThreadStore<T>::checked;
}

#endif /* JX_PERSISTENT_STORE_H_ */ 
