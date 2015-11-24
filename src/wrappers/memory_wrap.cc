// Copyright & License details are available under JXCORE_LICENSE file

#include "memory_wrap.h"
#include "node_buffer.h"
#include "jx/extend.h"
#include "jx/memory_store.h"
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
    THROW_EXCEPTION("Missing parameters (getMap) expects (int, string, bool).");
  }

  int tid = args.GetInteger(0) + 1;

  jxcore::JXString jxs;
  args.GetString(1, &jxs);
  std::string jstr(*jxs);

  BTStore::const_iterator it = node::commons::mapData[tid]->find(jstr);

  if (it != node::commons::mapData[tid]->end()) {
    if (!args.GetBoolean(2)) {  // is_buffer
      JS_LOCAL_STRING str =
          UTF8_TO_STRING_WITH_LENGTH(it->second.data_, it->second.length_);
      free(it->second.data_);
      node::commons::mapData[tid]->erase(jstr);
      RETURN_PARAM(str);
    } else {
      node::Buffer *buff =
          node::Buffer::New(it->second.data_, it->second.length_, com);
      free(it->second.data_);
      node::commons::mapData[tid]->erase(jstr);
      RETURN_PARAM(JS_TYPE_TO_LOCAL_OBJECT(buff->handle_));
    }
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
    THROW_EXCEPTION(
        "Missing parameters (readMap) expects (int, string, bool).");
  }

  int tid = args.GetInteger(0) + 1;
  jxcore::JXString jxs;
  args.GetString(1, &jxs);
  std::string jstr(*jxs);

  BTStore::const_iterator it = node::commons::mapData[tid]->find(jstr);

  if (it != node::commons::mapData[tid]->end()) {
    if (!args.GetBoolean(2)) {  // is_buffer
      JS_LOCAL_STRING str =
          UTF8_TO_STRING_WITH_LENGTH(it->second.data_, it->second.length_);
      RETURN_PARAM(str);
    } else {
      node::Buffer *buff =
          node::Buffer::New(it->second.data_, it->second.length_, com);
      RETURN_PARAM(JS_TYPE_TO_LOCAL_OBJECT(buff->handle_));
    }
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

  BTStore::const_iterator it = node::commons::mapData[tid]->find(jstr);

  if (it != node::commons::mapData[tid]->end()) {
    free(it->second.data_);
    node::commons::mapData[tid]->erase(jstr);
  }
}
JS_METHOD_END

#define COPY_BUFFER(buf, to)                                \
  size_t ln_##buf = BUFFER__LENGTH(buf);                    \
  char *to = (char *)malloc(sizeof(char) * (ln_##buf + 1)); \
  memcpy(to, BUFFER__DATA(buf), sizeof(char) * (ln_##buf)); \
  to[ln_##buf] = char(0)

JS_METHOD(MemoryWrap, MapSet) {
  if (!args.IsNumber(0) || !args.IsString(1) ||
      (!Buffer::jxHasInstance(args.GetItem(2), com) && !args.IsString(2))) {
    THROW_EXCEPTION(
        "Missing parameters (setMap) expects (int, string, string/buffer).");
  }

  int tid = args.GetInteger(0) + 1;
  jxcore::JXString str_key;
  args.GetString(1, &str_key);
  std::string str_keys(*str_key);

  MAP_HOST_DATA data;
  if (!args.IsString(2)) {
    JS_LOCAL_OBJECT obj = JS_VALUE_TO_OBJECT(args.GetItem(2));
    COPY_BUFFER(obj, val);
    data.data_ = val;
    data.length_ = ln_obj;
  } else {
    jxcore::JXString val;
    args.GetString(2, &val);
    val.DisableAutoGC();
    data.data_ = *val;
    data.length_ = val.length();
  }

  node::commons::mapData[tid]->erase(str_keys);
  node::commons::mapData[tid]->insert(std::make_pair(str_keys, data));
}
JS_METHOD_END

void MemoryWrap::MapClear(const bool clear_blocks) {
  auto_lock locker_(CSLOCK_JBEND);
  if (commons::mapCount == 0) {
    return;
  }

  for (int n = 0; n < commons::mapCount; n++) {
    BTStore::const_iterator it = commons::mapData[n]->begin();
    for (; it != commons::mapData[n]->end(); it++) {
      free(it->second.data_);
    }
    commons::mapData[n]->clear();
    if (clear_blocks) delete commons::mapData[n];
  }
  commons::mapCount = 0;
}

JS_METHOD(MemoryWrap, SourceSetIfNotExists) {
  if (XSpace::Store() == NULL) RETURN();

  if (!args.IsString(0) ||
      (!args.IsString(1) && !Buffer::jxHasInstance(args.GetItem(1), com))) {
    THROW_EXCEPTION(
        "Missing parameters (setSourceIfNotExists) expects (string, "
        "string/buffer).");
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
      MAP_HOST_DATA data;
      if (!args.IsString(1)) {
        JS_LOCAL_OBJECT obj = JS_VALUE_TO_OBJECT(args.GetItem(1));
        COPY_BUFFER(obj, val);
        data.data_ = val;
        data.length_ = ln_obj;
      } else {
        jxcore::JXString val;
        args.GetString(1, &val);
        val.DisableAutoGC();
        data.data_ = *val;
        data.length_ = val.length();
      }

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

  if (!args.IsString(0) ||
      (!args.IsString(1) && !Buffer::jxHasInstance(args.GetItem(1), com)) ||
      (!args.IsString(2) && !Buffer::jxHasInstance(args.GetItem(2), com))) {
    THROW_EXCEPTION(
        "Missing parameters (setSourceIfEqualsTo) expects (string, "
        "buffer/string, "
        "buffer/string).");
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
      std::string cmps;
      if (args.IsString(2)) {
        jxcore::JXString cmp;
        args.GetString(2, &cmp);
        cmps = *cmp;
      } else {
        JS_LOCAL_OBJECT obj = JS_VALUE_TO_OBJECT(args.GetItem(2));
        cmps = BUFFER__DATA(obj);
      }
      std::string org = it->second.data_;

      if (org.compare(cmps) == 0) {
        _StringStore::const_iterator it = store->find(str_keys);

        if (it != store->end()) {
          MAP_HOST_DATA old_data = it->second;
          free(old_data.data_);
          store->erase(str_keys);
        }

        MAP_HOST_DATA data;
        if (!args.IsString(1)) {
          JS_LOCAL_OBJECT obj = JS_VALUE_TO_OBJECT(args.GetItem(1));
          COPY_BUFFER(obj, val);
          data.data_ = val;
          data.length_ = ln_obj;
        } else {
          jxcore::JXString val;
          args.GetString(1, &val);
          val.DisableAutoGC();
          data.data_ = *val;
          data.length_ = val.length();
        }

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

  if (!args.IsString(0) ||
      (!args.IsString(1) && !Buffer::jxHasInstance(args.GetItem(1), com)) ||
      (!args.IsString(2) && !Buffer::jxHasInstance(args.GetItem(2), com))) {
    THROW_EXCEPTION(
        "Missing parameters (setSourceIfEqualsToOrNull) expects (string, "
        "buffer/string, buffer/string).");
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
      if (!args.IsString(2)) {
        JS_LOCAL_OBJECT obj = JS_VALUE_TO_OBJECT(args.GetItem(2));
        std::string cmps(BUFFER__DATA(obj));
        std::string org = it->second.data_;

        if (org.compare(cmps) == 0) equal = true;
      } else {
        jxcore::JXString val;
        args.GetString(2, &val);
        std::string cmps(*val);
        std::string org = it->second.data_;

        if (org.compare(cmps) == 0) equal = true;
      }
    }

    if (equal || null) {
      _StringStore::const_iterator it = store->find(str_keys);

      if (it != store->end()) {
        MAP_HOST_DATA old_data = it->second;
        free(old_data.data_);
        store->erase(str_keys);
      }

      MAP_HOST_DATA data;
      if (!args.IsString(1)) {
        JS_LOCAL_OBJECT obj = JS_VALUE_TO_OBJECT(args.GetItem(1));
        COPY_BUFFER(obj, val);
        data.data_ = val;
        data.length_ = ln_obj;
      } else {
        jxcore::JXString val;
        args.GetString(1, &val);
        val.DisableAutoGC();
        data.data_ = *val;
        data.length_ = val.length();
      }

      store->insert(std::make_pair(str_keys, data));
      set = true;
      XSpace::ExpirationKick(*str_key);
    }
  }
  XSpace::UNLOCKSTORE();

  RETURN_PARAM(STD_TO_BOOLEAN(set));
}
JS_METHOD_END

void MemoryWrap::SharedSet(const char *name, const char *value,
                           const size_t len) {
  XSpace::LOCKSTORE();
  _StringStore *store = XSpace::Store();
  if (store != NULL) {
    std::string str_key(name);
    _StringStore::const_iterator it = store->find(str_key);

    if (it != store->end()) {
      MAP_HOST_DATA old_data = it->second;
      free(old_data.data_);
      store->erase(str_key);
    }

    if (value != NULL) {
      MAP_HOST_DATA data;
      char *tmp = (char *)malloc(sizeof(char) * len + 1);
      memcpy(tmp, value, sizeof(char) * len);
      tmp[len] = char(0);
      data.length_ = len;
      data.data_ = tmp;

      store->insert(std::make_pair(str_key, data));
    }
  }
  XSpace::UNLOCKSTORE();
}

JS_METHOD(MemoryWrap, SourceSet) {
  if (XSpace::Store() == NULL) {
    RETURN();
  }

  if (!args.IsString(0) ||
      (!args.IsString(1) && !Buffer::jxHasInstance(args.GetItem(1), com))) {
    THROW_EXCEPTION(
        "Missing parameters (setSource) expects (string, buffer/string).");
  }

  jxcore::JXString str_key;
  args.GetString(0, &str_key);

  MAP_HOST_DATA data;
  if (!args.IsString(1)) {
    JS_LOCAL_OBJECT obj = JS_VALUE_TO_OBJECT(args.GetItem(1));
    SharedSet(*str_key, BUFFER__DATA(obj), BUFFER__LENGTH(obj));
  } else {
    jxcore::JXString val;
    args.GetString(1, &val);
    SharedSet(*str_key, *val, val.length());
  }

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
    THROW_EXCEPTION("Missing parameters (readSource) expects (string, bool).");
  }

  jxcore::JXString str_key;
  args.GetString(0, &str_key);
  std::string str_keys(*str_key);

  XSpace::LOCKSTORE();
  _StringStore *store = XSpace::Store();
  if (store != NULL) {
    _StringStore::const_iterator it = store->find(str_keys);
    if (it != store->end()) {
      MAP_HOST_DATA data = it->second;
      if (!args.GetBoolean(1)) {  // is_buffer
        JS_LOCAL_STRING str =
            UTF8_TO_STRING_WITH_LENGTH(data.data_, data.length_);
        XSpace::UNLOCKSTORE();

        XSpace::ExpirationKick(*str_key);
        RETURN_PARAM(str);
      } else {
        node::Buffer *buff =
            node::Buffer::New(it->second.data_, it->second.length_, com);
        XSpace::UNLOCKSTORE();

        XSpace::ExpirationKick(*str_key);
        RETURN_PARAM(JS_TYPE_TO_LOCAL_OBJECT(buff->handle_));
      }
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
      MAP_HOST_DATA data = it->second;
      free(data.data_);
      store->erase(str_keys);
    }
  }
  XSpace::UNLOCKSTORE();

  XSpace::ExpirationRemove(*str_key);
}
JS_METHOD_END

JS_METHOD(MemoryWrap, SourceGet) {
  if (XSpace::Store() == NULL) RETURN();

  if (!args.IsString(0)) {
    THROW_EXCEPTION("Missing parameters (getSource) expects (string, bool).");
  }

  jxcore::JXString str_key;
  args.GetString(0, &str_key);
  std::string str_keys(*str_key);

  XSpace::LOCKSTORE();
  _StringStore *store = XSpace::Store();
  if (store != NULL) {
    _StringStore::const_iterator it = store->find(str_keys);

    if (it != store->end()) {
      if (!args.GetBoolean(1)) {  // is_buffer
        MAP_HOST_DATA data = it->second;
        JS_LOCAL_STRING str =
            UTF8_TO_STRING_WITH_LENGTH(data.data_, data.length_);
        free(data.data_);
        store->erase(str_keys);
        XSpace::UNLOCKSTORE();

        XSpace::ExpirationRemove(*str_key);
        RETURN_PARAM(str);

      } else {
        node::Buffer *buff =
            node::Buffer::New(it->second.data_, it->second.length_, com);
        free(it->second.data_);
        store->erase(str_keys);
        XSpace::UNLOCKSTORE();

        XSpace::ExpirationRemove(*str_key);
        RETURN_PARAM(JS_TYPE_TO_LOCAL_OBJECT(buff->handle_));
      }
    }
  }
  XSpace::UNLOCKSTORE();
}
JS_METHOD_END

}  // namespace node

NODE_MODULE(node_memory_wrap, node::MemoryWrap::Initialize)
