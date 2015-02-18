// Copyright & License details are available under JXCORE_LICENSE file

#include "memory_wrap.h"
#include "jx/commons.h"
#include "jx/memory_store.h"
#include <stdio.h>
#include <string>
#include <iostream>

namespace node {

JS_METHOD(MemoryWrap, ReadEmbeddedSource) {
  if (!node::commons::embedded_source_) RETURN();

  RETURN_PARAM(STD_TO_STRING(commons::embedded_source_));
}
JS_METHOD_END

JS_METHOD(MemoryWrap, SetCPUCountMap) {
  if (!args.IsNumber(0)) {
    THROW_EXCEPTION("Missing parameters (setMapCount) expects (int).");
  }

  auto_lock locker_(CSLOCK_JBEND);
  if (node::commons::mapCount > 0) {
    RETURN();
  }

  const int no = args.GetInteger(0);
  node::commons::mapCount = no;
  if (node::commons::mapCount <= 1) {
    node::commons::mapCount = 3;
  }

  if (node::commons::mapCount > 65) node::commons::mapCount = 65;

  for (int i = 0; i < commons::mapCount; i++) {
    node::commons::mapData[i] = new BTStore;
  }
}
JS_METHOD_END

JS_METHOD(MemoryWrap, MapGet) {
  if (!args.IsNumber(0) || !args.IsString(1)) {
    THROW_EXCEPTION("Missing parameters (getMap) expects (int, string).");
  }

  int tid = args.GetInteger(0) + 1;

  jxcore::JXString jxs;
  args.GetString(1, &jxs);
  std::string jstr(*jxs);

  BTStore::const_iterator it = node::commons::mapData[tid]->find(jstr);

  if (it != node::commons::mapData[tid]->end()) {
    std::string backup(it->second.c_str());
    node::commons::mapData[tid]->erase(jstr);
    RETURN_PARAM(STD_TO_STRING(backup.c_str()));
  }
}
JS_METHOD_END

JS_METHOD(MemoryWrap, MapExist) {
  if (!args.IsNumber(0) || !args.IsString(1)) {
    THROW_EXCEPTION("Missing parameters (existMap) expects (int, string).");
  }

  int tid = args.GetInteger(0) + 1;
  jxcore::JXString jxs;
  args.GetString(1, &jxs);
  std::string jstr(*jxs);

  BTStore::const_iterator it = node::commons::mapData[tid]->find(jstr);

  RETURN_PARAM(STD_TO_BOOLEAN(it != node::commons::mapData[tid]->end()));
}
JS_METHOD_END

JS_METHOD(MemoryWrap, MapRead) {
  if (!args.IsNumber(0) || !args.IsString(1)) {
    THROW_EXCEPTION("Missing parameters (readMap) expects (int, string).");
  }

  int tid = args.GetInteger(0) + 1;
  jxcore::JXString jxs;
  args.GetString(1, &jxs);
  std::string jstr(*jxs);

  BTStore::const_iterator it = node::commons::mapData[tid]->find(jstr);

  if (it != node::commons::mapData[tid]->end()) {
    RETURN_PARAM(STD_TO_STRING(it->second.c_str()));
  }
}
JS_METHOD_END

JS_METHOD(MemoryWrap, MapRemove) {
  if (!args.IsNumber(0) || !args.IsString(1)) {
    THROW_EXCEPTION("Missing parameters (removeMap) expects (int, string).");
  }

  int tid = args.GetInteger(0) + 1;
  jxcore::JXString jxs;
  args.GetString(1, &jxs);
  std::string jstr(*jxs);
  node::commons::mapData[tid]->erase(jstr);
}
JS_METHOD_END

JS_METHOD(MemoryWrap, MapSet) {
  if (!args.IsNumber(0) || !args.IsString(1) || !args.IsString(2)) {
    THROW_EXCEPTION(
        "Missing parameters (setMap) expects (int, string, string).");
  }

  int tid = args.GetInteger(0) + 1;
  jxcore::JXString str_key;
  args.GetString(1, &str_key);
  std::string str_keys(*str_key);

  jxcore::JXString val;
  args.GetString(2, &val);
  std::string vals(*val);

  node::commons::mapData[tid]->erase(str_keys);
  node::commons::mapData[tid]->insert(std::make_pair(str_keys, vals));
}
JS_METHOD_END

JS_METHOD(MemoryWrap, SourceSetIfNotExists) {
  if (XSpace::Store() == NULL) RETURN();

  if (!args.IsString(0) || !args.IsString(1)) {
    THROW_EXCEPTION(
        "Missing parameters (setSourceIfNotExists) expects (string, string).");
  }

  jxcore::JXString str_key;
  args.GetString(0, &str_key);
  std::string str_keys(*str_key);

  bool set = false;
  XSpace::LOCKSTORE();
  _StringStore *store = XSpace::Store();
  if (store != NULL) {
    _StringStore::const_iterator it = store->find(str_keys);
    if (it == store->end()) {
      jxcore::JXString val;
      args.GetString(1, &val);

      externalData *data = new externalData;
      data->type = EXTERNAL_DATA_STRING;
      data->str_data = *val;

      store->insert(std::make_pair(str_keys, data));
      set = true;
    }
  }
  XSpace::UNLOCKSTORE();

  RETURN_PARAM(STD_TO_BOOLEAN(set));
}
JS_METHOD_END

JS_METHOD(MemoryWrap, SourceSetIfEq) {
  if (XSpace::Store() == NULL) RETURN();

  if (!args.IsString(0) || !args.IsString(1) || !args.IsString(2)) {
    THROW_EXCEPTION(
        "Missing parameters (setSourceIfEqualsTo) expects (string, string, "
        "string).");
  }

  jxcore::JXString str_key;
  args.GetString(0, &str_key);
  std::string str_keys(*str_key);

  bool set = false;
  XSpace::LOCKSTORE();
  _StringStore *store = XSpace::Store();
  if (store != NULL) {
    _StringStore::const_iterator it = store->find(str_keys);
    if (it != store->end()) {
      jxcore::JXString cmp;
      args.GetString(2, &cmp);
      std::string cmps(*cmp);

      if (it->second->str_data.compare(cmps) == 0) {
        _StringStore::const_iterator it = store->find(str_keys);

        if (it != store->end()) {
          externalData *old_data = it->second;
          store->erase(str_keys);
          delete old_data;
        }

        jxcore::JXString val;
        args.GetString(1, &val);

        externalData *data = new externalData;
        data->type = EXTERNAL_DATA_STRING;
        data->str_data = *val;

        store->insert(std::make_pair(str_keys, data));
        set = true;
        XSpace::ExpirationKick(*str_key);
      }
    }
  }
  XSpace::UNLOCKSTORE();

  RETURN_PARAM(STD_TO_BOOLEAN(set));
}
JS_METHOD_END

JS_METHOD(MemoryWrap, SourceSetIfEqOrNull) {
  if (XSpace::Store() == NULL) RETURN();

  if (!args.IsString(0) || !args.IsString(1) || !args.IsString(2)) {
    THROW_EXCEPTION(
        "Missing parameters (setSourceIfEqualsToOrNull) expects (string, "
        "string, string).");
  }

  jxcore::JXString str_key;
  args.GetString(0, &str_key);
  std::string str_keys(*str_key);

  bool set = false;

  XSpace::LOCKSTORE();
  _StringStore *store = XSpace::Store();
  if (store != NULL) {
    _StringStore::const_iterator it = store->find(str_keys);
    bool equal = false;
    bool null = false;

    if (it == store->end()) {
      null = true;
    } else {
      jxcore::JXString cmp;
      args.GetString(2, &cmp);
      std::string cmps(*cmp);

      if (it->second->str_data.compare(cmps) == 0) equal = true;
    }

    if (equal || null) {
      _StringStore::const_iterator it = store->find(str_keys);

      if (it != store->end()) {
        externalData *old_data = it->second;
        store->erase(str_keys);
        delete old_data;
      }

      jxcore::JXString val;
      args.GetString(1, &val);

      externalData *data = new externalData;
      data->type = EXTERNAL_DATA_STRING;
      data->str_data = *val;

      store->insert(std::make_pair(str_keys, data));
      set = true;
      XSpace::ExpirationKick(*str_key);
    }
  }
  XSpace::UNLOCKSTORE();

  RETURN_PARAM(STD_TO_BOOLEAN(set));
}
JS_METHOD_END

void MemoryWrap::SharedSet(const char *name, const char *value) {
  XSpace::LOCKSTORE();
  _StringStore *store = XSpace::Store();
  if (store != NULL) {
    std::string str_key(name);
    _StringStore::const_iterator it = store->find(str_key);

    if (it != store->end()) {
      externalData *old_data = it->second;
      store->erase(str_key);
      delete old_data;
    }

    if (value != NULL) {
      externalData *data = new externalData;
      data->type = EXTERNAL_DATA_STRING;
      data->str_data = std::string(value);

      store->insert(std::make_pair(str_key, data));
    }
  }
  XSpace::UNLOCKSTORE();
}

JS_METHOD(MemoryWrap, SourceSet) {
  if (XSpace::Store() == NULL) {
    RETURN();
  }

  if (!args.IsString(0) || !args.IsString(1)) {
    THROW_EXCEPTION("Missing parameters (setSource) expects (string, string).");
  }

  jxcore::JXString str_key;
  args.GetString(0, &str_key);

  jxcore::JXString val;
  args.GetString(1, &val);

  SharedSet(*str_key, *val);

  XSpace::ExpirationKick(*str_key);
}
JS_METHOD_END

JS_METHOD(MemoryWrap, SourceExist) {
  if (XSpace::Store() == NULL) RETURN_PARAM(STD_TO_BOOLEAN(false));

  if (!args.IsString(0)) {
    THROW_EXCEPTION("Missing parameters (existsSource) expects (string).");
  }

  jxcore::JXString str_key;
  args.GetString(0, &str_key);
  std::string jstr_key(*str_key);

  XSpace::LOCKSTORE();
  _StringStore *store = XSpace::Store();
  if (store != NULL) {
    _StringStore::const_iterator it = store->find(jstr_key);
    bool exist = it != store->end();
    XSpace::UNLOCKSTORE();
    RETURN_PARAM(STD_TO_BOOLEAN(exist));
  }
  XSpace::UNLOCKSTORE();

  RETURN_PARAM(STD_TO_BOOLEAN(false));
}
JS_METHOD_END

JS_METHOD(MemoryWrap, SourceRead) {
  if (XSpace::Store() == NULL) RETURN();

  if (!args.IsString(0)) {
    THROW_EXCEPTION("Missing parameters (readSource) expects (string).");
  }

  jxcore::JXString str_key;
  args.GetString(0, &str_key);
  std::string str_keys(*str_key);

  XSpace::LOCKSTORE();
  _StringStore *store = XSpace::Store();
  if (store != NULL) {
    _StringStore::const_iterator it = store->find(str_keys);
    if (it != store->end()) {
      externalData *data = it->second;
      std::string localSTR(data->str_data.c_str());
      XSpace::UNLOCKSTORE();

      XSpace::ExpirationKick(*str_key);
      RETURN_PARAM(STD_TO_STRING(localSTR.c_str()));
    }
  }
  XSpace::UNLOCKSTORE();
}
JS_METHOD_END

JS_METHOD(MemoryWrap, SourceRemove) {
  if (XSpace::Store() == NULL) RETURN();

  if (!args.IsString(0)) {
    THROW_EXCEPTION("Missing parameters (removeSource) expects (string).");
  }

  jxcore::JXString str_key;
  args.GetString(0, &str_key);
  std::string str_keys(*str_key);

  XSpace::LOCKSTORE();
  _StringStore *store = XSpace::Store();
  if (store != NULL) {
    _StringStore::const_iterator it = store->find(str_keys);

    if (it != store->end()) {
      externalData *data = it->second;
      store->erase(str_keys);
      delete data;
    }
  }
  XSpace::UNLOCKSTORE();

  XSpace::ExpirationRemove(*str_key);
}
JS_METHOD_END

JS_METHOD(MemoryWrap, SourceGet) {
  if (XSpace::Store() == NULL) RETURN();

  if (!args.IsString(0)) {
    THROW_EXCEPTION("Missing parameters (getSource) expects (string).");
  }

  jxcore::JXString str_key;
  args.GetString(0, &str_key);
  std::string str_keys(*str_key);

  XSpace::LOCKSTORE();
  _StringStore *store = XSpace::Store();
  if (store != NULL) {
    _StringStore::const_iterator it = store->find(str_keys);

    if (it != store->end()) {
      externalData *data = it->second;
      std::string str(data->str_data.c_str());
      store->erase(str_keys);
      XSpace::UNLOCKSTORE();
      delete data;

      XSpace::ExpirationRemove(*str_key);
      RETURN_PARAM(STD_TO_STRING(str.c_str()));
    }
  }
  XSpace::UNLOCKSTORE();
}
JS_METHOD_END

void MemoryWrap::MapClear(const bool clear_blocks) {
  auto_lock locker_(CSLOCK_JBEND);
  if (commons::mapCount == 0) {
    return;
  }

  for (int n = 0; n < commons::mapCount; n++) {
    commons::mapData[n]->clear();
    if (clear_blocks) delete commons::mapData[n];
  }
  commons::mapCount = 0;
}

}  // namespace node

NODE_MODULE(node_memory_wrap, node::MemoryWrap::Initialize)
