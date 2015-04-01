// Copyright & License details are available under JXCORE_LICENSE file

#include "node.h"
#include "node_file.h"
#include "node_buffer.h"
#include "jx/commons.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#if defined(__MINGW32__) || defined(_MSC_VER)
#include <io.h>
#endif

namespace node {

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define TYPE_ERROR(msg) THROW_TYPE_EXCEPTION(msg)

#define THROW_BAD_ARGS TYPE_ERROR("Bad argument")

JS_LOCAL_OBJECT BuildStatsObject(commons* com, const uv_statbuf_t* s);

class FSReqWrap : public ReqWrap<uv_fs_t> {
 public:
  void* operator new(size_t size, char* storage) { return storage; }

  FSReqWrap(const char* syscall, commons* como)
      : ReqWrap<uv_fs_t>(como), syscall_(syscall), dest_len_(0) {}

  inline const char* syscall() const { return syscall_; }
  inline const char* dest() const { return dest_; }
  inline unsigned int dest_len() const { return dest_len_; }
  inline void dest_len(unsigned int dest_len) { dest_len_ = dest_len; }

 private:
  const char* syscall_;
  unsigned int dest_len_;
  char dest_[1];
};

#define ASSERT_OFFSET(a)                                                       \
  if (!(a)->IsUndefined() && !(a)->IsNull() && !IsInt64((a)->NumberValue())) { \
    THROW_TYPE_EXCEPTION("Not an integer");                                    \
  }
#define ASSERT_TRUNCATE_LENGTH(a)                                              \
  if (!(a)->IsUndefined() && !(a)->IsNull() && !IsInt64((a)->NumberValue())) { \
    THROW_TYPE_EXCEPTION("Not an integer");                                    \
  }

#define GET_OFFSET(a) ((a)->IsNumber() ? (a)->IntegerValue() : -1)
#define GET_TRUNCATE_LENGTH(a) ((a)->IntegerValue())

static inline bool IsInt64(double x) {
  return x == static_cast<double>(static_cast<int64_t>(x));
}

static void After(uv_fs_t* req) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  FSReqWrap* req_wrap = (FSReqWrap*)req->data;
  assert(&req_wrap->req_ == req);

  // there is always at least one argument. "error"
  int argc = 1;

  // Allocate space for two args. We may only use one depending on the case.
  // (Feel free to increase this if you need more)
  JS_LOCAL_VALUE argv[2];

  // NOTE: This may be needed to be changed if something returns a -1
  // for a success, which is possible.
  if (req->result == -1) {
    // If the request doesn't have a path parameter set.

    if (!req->path) {
      argv[0] = UVException(req->errorno, NULL, req_wrap->syscall());
    } else if ((req->errorno == UV_EEXIST || req->errorno == UV_ENOTEMPTY ||
                req->errorno == UV_EPERM) &&
               req_wrap->dest_len() > 0) {
      argv[0] = UVException(req->errorno, NULL, req_wrap->syscall(),
                            req_wrap->dest());
    } else {
      argv[0] = UVException(req->errorno, NULL, req_wrap->syscall(),
                            static_cast<const char*>(req->path));
    }
  } else {
    // error value is empty or null for non-error.
    argv[0] = JS_NULL();

    // All have at least two args now.
    argc = 2;

    switch (req->fs_type) {
      // These all have no data to pass.
      case UV_FS_CLOSE:
      case UV_FS_RENAME:
      case UV_FS_UNLINK:
      case UV_FS_RMDIR:
      case UV_FS_MKDIR:
      case UV_FS_FTRUNCATE:
      case UV_FS_FSYNC:
      case UV_FS_FDATASYNC:
      case UV_FS_LINK:
      case UV_FS_SYMLINK:
      case UV_FS_CHMOD:
      case UV_FS_FCHMOD:
      case UV_FS_CHOWN:
      case UV_FS_FCHOWN:
        // These, however, don't.
        argc = 1;
        break;

      case UV_FS_UTIME:
      case UV_FS_FUTIME:
        argc = 0;
        break;

      case UV_FS_OPEN:
        argv[1] = STD_TO_INTEGER(req->result);
        break;

      case UV_FS_WRITE:
        argv[1] = STD_TO_INTEGER(req->result);
        break;

      case UV_FS_STAT:
      case UV_FS_LSTAT:
      case UV_FS_FSTAT:
        argv[1] = BuildStatsObject(com, static_cast<const uv_statbuf_t*>(req->ptr));
        break;

      case UV_FS_READLINK:
        argv[1] = UTF8_TO_STRING(static_cast<char*>(req->ptr));
        break;

      case UV_FS_READ:
        // Buffer interface
        argv[1] = STD_TO_INTEGER(req->result);
        break;

      case UV_FS_READDIR: {
        char* namebuf = static_cast<char*>(req->ptr);
        int nnames = req->result;

        JS_LOCAL_ARRAY names = JS_NEW_ARRAY_WITH_COUNT(nnames);

        for (int i = 0; i < nnames; i++) {
          JS_LOCAL_STRING name = STD_TO_STRING(namebuf);
          JS_INDEX_SET(names, i, name);
#ifndef NDEBUG
          namebuf += strlen(namebuf);
          assert(*namebuf == '\0');
          namebuf += 1;
#else
          namebuf += strlen(namebuf) + 1;
#endif
        }

        argv[1] = names;
      } break;

      default:
        assert(0 && "Unhandled eio response");
    }
  }

  MakeCallback(com, req_wrap->object_, com->pstr_oncomplete, argc, argv);

  uv_fs_req_cleanup(&req_wrap->req_);
  delete req_wrap;
}

// This struct is only used on sync fs calls.
// For async calls FSReqWrap is used.
struct fs_req_wrap {
  fs_req_wrap() {}
  ~fs_req_wrap() { uv_fs_req_cleanup(&req); }
  // Ensure that copy ctor and assignment operator are not used.
  fs_req_wrap(const fs_req_wrap& req);
  fs_req_wrap& operator=(const fs_req_wrap& req);
  uv_fs_t req;
};

#define ASYNC_DEST_CALL(func, callback, dest_path, ...)                       \
  FSReqWrap* req_wrap;                                                        \
  char* dest_str = (dest_path);                                               \
  int dest_len = dest_str == NULL ? 0 : strlen(dest_str);                     \
  char* storage = new char[sizeof(*req_wrap) + dest_len];                     \
  req_wrap = new (storage) FSReqWrap(#func, com);                             \
  req_wrap->dest_len(dest_len);                                               \
  if (dest_str != NULL) {                                                     \
    memcpy(const_cast<char*>(req_wrap->dest()), dest_str, dest_len + 1);      \
  }                                                                           \
  int r = uv_fs_##func(com->loop, &req_wrap->req_, __VA_ARGS__, After);       \
  JS_NAME_SET(req_wrap->object_, JS_PREDEFINED_STRING(oncomplete), callback); \
  req_wrap->Dispatched();                                                     \
  if (r < 0) {                                                                \
    uv_fs_t* req = &req_wrap->req_;                                           \
    req->result = r;                                                          \
    req->path = NULL;                                                         \
    req->errorno = uv_last_error(com->loop).code;                             \
    After(req);                                                               \
  }                                                                           \
  RETURN_PARAM(req_wrap->object_);

#define ASYNC_CALL(func, callback, ...) \
  ASYNC_DEST_CALL(func, callback, NULL, __VA_ARGS__)

#define SYNC_DEST_CALL(func, path, dest, ...)                              \
  fs_req_wrap req_wrap;                                                    \
  int result = uv_fs_##func(com->loop, &req_wrap.req, __VA_ARGS__, NULL);  \
  if (result < 0) {                                                        \
    int code = uv_last_error(com->loop).code;                              \
    if (dest != NULL &&                                                    \
        (code == UV_EEXIST || code == UV_ENOTEMPTY || code == UV_EPERM)) { \
      JS_LOCAL_VALUE __err = UVException(code, #func, "", dest);           \
      THROW_EXCEPTION_OBJECT(__err);                                       \
    } else {                                                               \
      JS_LOCAL_VALUE __err = UVException(code, #func, "", path);           \
      THROW_EXCEPTION_OBJECT(__err);                                       \
    }                                                                      \
  }

#define SYNC_CALL(func, path, ...) SYNC_DEST_CALL(func, path, NULL, __VA_ARGS__)

#define SYNC_REQ req_wrap.req

#define SYNC_RESULT result

JS_METHOD(File, Close) {
  if (args.Length() < 1 || !args.IsInteger(0)) {
    THROW_BAD_ARGS;
  }

  int fd = args.GetInt32(0);

  if (args.IsFunction(1)) {
    JS_LOCAL_VALUE arg1 = GET_ARG(1);
    ASYNC_CALL(close, arg1, fd)
  } else {
    SYNC_CALL(close, 0, fd)
    RETURN();
  }
}
JS_METHOD_END

JS_LOCAL_OBJECT BuildStatsObject(commons* com, const uv_statbuf_t* s) {
  JS_ENTER_SCOPE();

  JS_DEFINE_STATE_MARKER(com);

#ifdef JS_ENGINE_MOZJS
  // create non Callable Stat. V8 alternative creates the new instance non-callable
  JS::RootedValue rt_stat(__contextORisolate);
  com->CreateNewNonCallableInstance(&com->nf_stats_constructor_template, &rt_stat);
  JS_LOCAL_OBJECT stats(rt_stat.get(), __contextORisolate);
#elif defined(JS_ENGINE_V8)
  JS_LOCAL_OBJECT stats = JS_NEW_DEFAULT_INSTANCE(
      JS_GET_FUNCTION(com->nf_stats_constructor_template));

  if (JS_IS_EMPTY(stats)) return JS_LOCAL_OBJECT();
#endif

#define X(name, v)                                     \
  {                                                    \
    JS_LOCAL_VALUE val = STD_TO_INTEGER(s->st_##name); \
    if (JS_IS_EMPTY(val)) return JS_LOCAL_OBJECT();    \
    JS_NAME_SET(stats, v, val);                        \
  }
  X(dev, JS_STRING_ID("dev"))
  X(mode, JS_STRING_ID("mode"))
  X(nlink, JS_STRING_ID("nlink"))
  X(uid, JS_STRING_ID("uid"))
  X(gid, JS_STRING_ID("gid"))
  X(rdev, JS_STRING_ID("rdev"))
#if defined(__POSIX__)
  X(blksize, JS_STRING_ID("blksize"))
#endif
#undef X

#define X(name, v)                                                         \
  {                                                                        \
    JS_LOCAL_VALUE val = STD_TO_NUMBER(static_cast<double>(s->st_##name)); \
    if (JS_IS_EMPTY(val)) return JS_LOCAL_OBJECT();                        \
    JS_NAME_SET(stats, v, val);                                            \
  }
  X(ino, JS_STRING_ID("ino"))
  X(size, JS_STRING_ID("size"))
#if defined(__POSIX__)
  X(blocks, JS_STRING_ID("blocks"))
#endif
#undef X

#define X(name, v)                                       \
  {                                                      \
    JS_LOCAL_VALUE val = NODE_UNIXTIME_V8(s->st_##name); \
    if (JS_IS_EMPTY(val)) return JS_LOCAL_OBJECT();      \
    JS_NAME_SET(stats, v, val);                          \
  }
  X(atime, JS_STRING_ID("atime"))
  X(mtime, JS_STRING_ID("mtime"))
  X(ctime, JS_STRING_ID("ctime"))
#undef X

  return JS_LEAVE_SCOPE(stats);
}

JS_LOCAL_OBJECT BuildStatsObject(const uv_statbuf_t* s) {
  JS_ENTER_SCOPE_COM();
  if (com == NULL) return JS_LOCAL_OBJECT();

  return JS_LEAVE_SCOPE(BuildStatsObject(com, s));
}

JS_METHOD(File, Stat) {
  if (!args.IsString(0)) THROW_TYPE_EXCEPTION("path must be a string");

  jxcore::JXString path;
  args.GetString(0, &path);

  if (args.IsFunction(1)) {
    ASYNC_CALL(stat, GET_ARG(1), *path)
  } else {
    SYNC_CALL(stat, *path, *path)
    RETURN_PARAM(
        BuildStatsObject(com, static_cast<const uv_statbuf_t*>(SYNC_REQ.ptr)));
  }
}
JS_METHOD_END

JS_METHOD(File, LStat) {
  if (!args.IsString(0)) THROW_TYPE_EXCEPTION("path must be a string");

  jxcore::JXString path;
  args.GetString(0, &path);

  if (args.IsFunction(1)) {
    ASYNC_CALL(lstat, GET_ARG(1), *path)
  } else {
    SYNC_CALL(lstat, *path, *path)
    RETURN_PARAM(
        BuildStatsObject(com, static_cast<const uv_statbuf_t*>(SYNC_REQ.ptr)));
  }
}
JS_METHOD_END

JS_METHOD(File, FStat) {
  if (!args.IsInteger(0)) {
    THROW_TYPE_EXCEPTION("Bad Argument, expects integer");
  }

  int fd = args.GetInteger(0);

  if (args.IsFunction(1)) {
    ASYNC_CALL(fstat, GET_ARG(1), fd)
  } else {
    SYNC_CALL(fstat, 0, fd)
    RETURN_PARAM(
        BuildStatsObject(com, static_cast<const uv_statbuf_t*>(SYNC_REQ.ptr)));
  }
}
JS_METHOD_END

JS_METHOD(File, Symlink) {
  if (!args.IsString(0)) THROW_TYPE_EXCEPTION("dest path must be a string");
  if (!args.IsString(1)) THROW_TYPE_EXCEPTION("src path must be a string");

  jxcore::JXString dest;
  args.GetString(0, &dest);
  jxcore::JXString path;
  args.GetString(1, &path);
  int flags = 0;

  if (args.IsString(2)) {
    jxcore::JXString mode;
    args.GetString(2, &mode);
    if (strcmp(*mode, "dir") == 0) {
      flags |= UV_FS_SYMLINK_DIR;
    } else if (strcmp(*mode, "junction") == 0) {
      flags |= UV_FS_SYMLINK_JUNCTION;
    } else if (strcmp(*mode, "file") != 0) {
      THROW_EXCEPTION("Unknown symlink type");
    }
  }

  if (args.IsFunction(3)) {
    ASYNC_DEST_CALL(symlink, GET_ARG(3), *dest, *dest, *path, flags)
  } else {
    SYNC_DEST_CALL(symlink, *path, *dest, *dest, *path, flags)
    RETURN();
  }
}
JS_METHOD_END

JS_METHOD(File, Link) {
  int len = args.Length();
  if (len < 1) THROW_TYPE_EXCEPTION("dest path required");
  if (len < 2) THROW_TYPE_EXCEPTION("src path required");
  if (!args.IsString(0)) THROW_TYPE_EXCEPTION("dest path must be a string");
  if (!args.IsString(1)) THROW_TYPE_EXCEPTION("src path must be a string");

  JS_LOCAL_VALUE arg0 = GET_ARG(0);
  JS_LOCAL_VALUE arg1 = GET_ARG(1);

  jxcore::JXString orig_path(arg0);
  jxcore::JXString new_path(arg1);

  if (args.IsFunction(2)) {
    JS_LOCAL_VALUE arg2 = GET_ARG(2);
    ASYNC_DEST_CALL(link, arg2, *new_path, *orig_path, *new_path)
  } else {
    SYNC_DEST_CALL(link, *orig_path, *new_path, *orig_path, *new_path)
    RETURN();
  }
}
JS_METHOD_END

JS_METHOD(File, ReadLink) {
  if (!args.IsString(0)) THROW_TYPE_EXCEPTION("path must be a string");

  jxcore::JXString path;
  args.GetString(0, &path);

  if (args.IsFunction(1)) {
    ASYNC_CALL(readlink, GET_ARG(1), *path)
  } else {
    SYNC_CALL(readlink, *path, *path)
    RETURN_PARAM(UTF8_TO_STRING((char*)SYNC_REQ.ptr));
  }
}
JS_METHOD_END

JS_METHOD(File, Rename) {
  if (!args.IsString(0)) THROW_TYPE_EXCEPTION("old path must be a string");
  if (!args.IsString(1)) THROW_TYPE_EXCEPTION("new path must be a string");

  jxcore::JXString old_path;
  jxcore::JXString new_path;
  args.GetString(0, &old_path);
  args.GetString(1, &new_path);

  if (args.IsFunction(2)) {
    ASYNC_DEST_CALL(rename, GET_ARG(2), *new_path, *old_path, *new_path)
  } else {
    SYNC_DEST_CALL(rename, *old_path, *new_path, *old_path, *new_path)
  }
}
JS_METHOD_END

JS_METHOD(File, FTruncate) {
  if (args.Length() < 2 || !args.IsInteger(0)) {
    THROW_BAD_ARGS;
  }

  int fd = args.GetInt32(0);

  JS_LOCAL_VALUE arg1 = GET_ARG(1);
  JS_LOCAL_VALUE arg2 = GET_ARG(2);

  ASSERT_TRUNCATE_LENGTH(arg1);
  int64_t len = GET_TRUNCATE_LENGTH(arg1);

  if (arg2->IsFunction()) {
    ASYNC_CALL(ftruncate, arg2, fd, len)
  } else {
    SYNC_CALL(ftruncate, 0, fd, len)
    RETURN();
  }
}
JS_METHOD_END

JS_METHOD(File, Fdatasync) {
  if (args.Length() < 1 || !args.IsInteger(0)) {
    THROW_BAD_ARGS;
  }

  int fd = args.GetInt32(0);

  if (args.IsFunction(1)) {
    JS_LOCAL_VALUE arg1 = GET_ARG(1);
    ASYNC_CALL(fdatasync, arg1, fd)
  } else {
    SYNC_CALL(fdatasync, 0, fd)
    RETURN();
  }
}
JS_METHOD_END

JS_METHOD(File, Fsync) {
  if (args.Length() < 1 || !args.IsInteger(0)) {
    THROW_BAD_ARGS;
  }

  int fd = args.GetInt32(0);

  if (args.IsFunction(1)) {
    JS_LOCAL_VALUE arg1 = GET_ARG(1);
    ASYNC_CALL(fsync, arg1, fd)
  } else {
    SYNC_CALL(fsync, 0, fd)
    RETURN();
  }
}
JS_METHOD_END

JS_METHOD(File, Unlink) {
  if (args.Length() < 1) TYPE_ERROR("path required");
  if (!args.IsString(0)) TYPE_ERROR("path must be a string");

  JS_LOCAL_VALUE arg0 = GET_ARG(0);
  jxcore::JXString path(arg0);

  if (args.IsFunction(1)) {
    JS_LOCAL_VALUE arg1 = GET_ARG(1);
    ASYNC_CALL(unlink, arg1, *path)
  } else {
    SYNC_CALL(unlink, *path, *path)
    RETURN();
  }
}
JS_METHOD_END

JS_METHOD(File, RMDir) {
  if (args.Length() < 1) TYPE_ERROR("path required");
  if (!args.IsString(0)) TYPE_ERROR("path must be a string");

  JS_LOCAL_VALUE arg0 = GET_ARG(0);
  jxcore::JXString path(arg0);

  if (args.IsFunction(1)) {
    JS_LOCAL_VALUE arg1 = GET_ARG(1);
    ASYNC_CALL(rmdir, arg1, *path)
  } else {
    SYNC_CALL(rmdir, *path, *path)
    RETURN();
  }
}
JS_METHOD_END

JS_METHOD(File, MKDir) {
  if (args.Length() < 2 || !args.IsString(0) || !args.IsInteger(1)) {
    THROW_BAD_ARGS;
  }

  JS_LOCAL_VALUE arg0 = GET_ARG(0);
  jxcore::JXString path(arg0);
  int mode = static_cast<int>(args.GetInt32(1));

  if (args.IsFunction(2)) {
    JS_LOCAL_VALUE arg2 = GET_ARG(2);
    ASYNC_CALL(mkdir, arg2, *path, mode)
  } else {
    SYNC_CALL(mkdir, *path, *path, mode)
    RETURN();
  }
}
JS_METHOD_END

JS_METHOD(File, ReadDir) {
  if (args.Length() < 1) TYPE_ERROR("path required");
  if (!args.IsString(0)) TYPE_ERROR("path must be a string");

  JS_LOCAL_VALUE arg0 = GET_ARG(0);
  jxcore::JXString path(arg0);

  if (args.IsFunction(1)) {
    JS_LOCAL_VALUE arg1 = GET_ARG(1);
    ASYNC_CALL(readdir, arg1, *path, 0 /*flags*/)
  } else {
    SYNC_CALL(readdir, *path, *path, 0 /*flags*/)

    char* namebuf = static_cast<char*>(SYNC_REQ.ptr);
    int nnames = req_wrap.req.result;
    JS_LOCAL_ARRAY names = JS_NEW_ARRAY_WITH_COUNT(nnames);

    for (int i = 0; i < nnames; i++) {
      JS_LOCAL_STRING name = UTF8_TO_STRING(namebuf);
      JS_INDEX_SET(names, i, name);
#ifndef NDEBUG
      namebuf += strlen(namebuf);
      assert(*namebuf == '\0');
      namebuf += 1;
#else
      namebuf += strlen(namebuf) + 1;
#endif
    }

    RETURN_POINTER(names);
  }
}
JS_METHOD_END

JS_METHOD(File, Open) {
  int len = args.Length();
  if (len < 1) TYPE_ERROR("path required");
  if (len < 2) TYPE_ERROR("flags required");
  if (len < 3) TYPE_ERROR("mode required");
  if (!args.IsString(0)) TYPE_ERROR("path must be a string");
  if (!args.IsInteger(1)) TYPE_ERROR("flags must be an int");
  if (!args.IsInteger(2)) TYPE_ERROR("mode must be an int");

  JS_LOCAL_VALUE arg0 = GET_ARG(0);
  jxcore::JXString path(arg0);
  int flags = args.GetInt32(1);
  int mode = static_cast<int>(args.GetInt32(2));

  if (args.IsFunction(3)) {
    JS_LOCAL_VALUE arg3 = GET_ARG(3);
    ASYNC_CALL(open, arg3, *path, flags, mode)
  } else {
    SYNC_CALL(open, *path, *path, flags, mode)
    int fd = SYNC_RESULT;
    RETURN_PARAM(STD_TO_INTEGER(fd));
  }
}
JS_METHOD_END

// bytesWritten = write(fd, data, position, enc, callback)
// Wrapper for write(2).
//
// 0 fd        integer. file descriptor
// 1 buffer    the data to write
// 2 offset    where in the buffer to start from
// 3 length    how much to write
// 4 position  if integer, position to write at in the file.
//             if null, write from the current position
JS_METHOD(File, Write) {
  if (args.Length() < 5) {
    THROW_EXCEPTION(
        "expects (fd integer, data buffer, offset integer, length integer, "
        "position integer)");
  }

  if (!args.IsInteger(0)) {
    THROW_TYPE_EXCEPTION("Bad argument, expects integer");
  }

  int fd = args.GetInteger(0);

  if (!Buffer::jxHasInstance(GET_ARG(1), com)) {
    THROW_EXCEPTION("Second argument needs to be a buffer");
  }

  JS_LOCAL_OBJECT buffer_obj = JS_VALUE_TO_OBJECT(GET_ARG(1));
  char* buffer_data = BUFFER__DATA(buffer_obj);
  size_t buffer_length = BUFFER__LENGTH(buffer_obj);

  size_t off = args.GetInteger(2);
  if (off >= buffer_length) {
    THROW_RANGE_EXCEPTION("Offset is out of bounds");
  }

  ssize_t len = args.GetInteger(3);
  if (!Buffer::IsWithinBounds(off, len, buffer_length)) {
    THROW_RANGE_EXCEPTION("off + len > buffer.length");
  }

  JS_LOCAL_VALUE _item4 = GET_ARG(4);
  ASSERT_OFFSET(_item4);
  int64_t pos = GET_OFFSET(_item4);

  char* buf = (char*)buffer_data + off;
  JS_LOCAL_VALUE cb = GET_ARG(5);

  if (JS_IS_FUNCTION(cb)) {
    ASYNC_CALL(write, cb, fd, buf, len, pos)
  } else {
    SYNC_CALL(write, 0, fd, buf, len, pos)
    RETURN_PARAM(STD_TO_INTEGER(SYNC_RESULT));
  }
}
JS_METHOD_END

/*
 * Wrapper for read(2).
 *
 * bytesRead = fs.read(fd, buffer, offset, length, position)
 *
 * 0 fd        integer. file descriptor
 * 1 buffer    instance of Buffer
 * 2 offset    integer. offset to start reading into inside buffer
 * 3 length    integer. length to read
 * 4 position  file position - null for current position
 *
 */
JS_METHOD(File, Read) {
  if (args.Length() < 2 || !args.IsInteger(0)) {
    THROW_BAD_ARGS;
  }

  int fd = args.GetInt32(0);

  JS_LOCAL_VALUE cb;

  size_t len;
  int64_t pos;

  char* buf = NULL;

  if (!Buffer::jxHasInstance(args.GetItem(1), com)) {
    THROW_EXCEPTION("Second argument needs to be a buffer");
  }

  JS_LOCAL_OBJECT buffer_obj = JS_VALUE_TO_OBJECT(args.GetItem(1));
  char* buffer_data = BUFFER__DATA(buffer_obj);
  size_t buffer_length = BUFFER__LENGTH(buffer_obj);

  size_t off = args.GetInt32(2);
  if (off >= buffer_length) {
    THROW_EXCEPTION("Offset is out of bounds");
  }

  len = args.GetInt32(3);
  if (!Buffer::IsWithinBounds(off, len, buffer_length)) {
    THROW_EXCEPTION("Length extends beyond buffer");
  }

  JS_HANDLE_VALUE arg4 = args.GetItem(4);
  pos = GET_OFFSET(arg4);

  buf = buffer_data + off;

  cb = GET_ARG(5);

  if (JS_IS_FUNCTION(cb)) {
    ASYNC_CALL(read, cb, fd, buf, len, pos);
  } else {
    SYNC_CALL(read, 0, fd, buf, len, pos)
    JS_LOCAL_INTEGER bytesRead = STD_TO_INTEGER(SYNC_RESULT);
    RETURN_POINTER(bytesRead);
  }
}
JS_METHOD_END

/* fs.chmod(path, mode);
 * Wrapper for chmod(1) / EIO_CHMOD
 */
JS_METHOD(File, Chmod) {
  if (args.Length() < 2 || !args.IsString(0) || !args.IsInteger(1)) {
    THROW_BAD_ARGS;
  }

  JS_LOCAL_VALUE arg0 = GET_ARG(0);
  jxcore::JXString path(arg0);
  int mode = static_cast<int>(args.GetInt32(1));

  if (args.IsFunction(2)) {
    JS_LOCAL_VALUE arg2 = GET_ARG(2);
    ASYNC_CALL(chmod, arg2, *path, mode);
  } else {
    SYNC_CALL(chmod, *path, *path, mode);
    RETURN();
  }
}
JS_METHOD_END

/* fs.fchmod(fd, mode);
 * Wrapper for fchmod(1) / EIO_FCHMOD
 */
JS_METHOD(File, FChmod) {
  if (args.Length() < 2 || !args.IsInteger(0) || !args.IsInteger(1)) {
    THROW_BAD_ARGS;
  }
  int fd = args.GetInt32(0);
  int mode = static_cast<int>(args.GetInt32(1));

  if (args.IsFunction(2)) {
    JS_LOCAL_VALUE arg2 = GET_ARG(2);
    ASYNC_CALL(fchmod, arg2, fd, mode);
  } else {
    SYNC_CALL(fchmod, 0, fd, mode);
    RETURN();
  }
}
JS_METHOD_END

/* fs.chown(path, uid, gid);
 * Wrapper for chown(1) / EIO_CHOWN
 */
JS_METHOD(File, Chown) {
  int len = args.Length();
  if (len < 1) TYPE_ERROR("path required");
  if (len < 2) TYPE_ERROR("uid required");
  if (len < 3) TYPE_ERROR("gid required");
  if (!args.IsString(0)) TYPE_ERROR("path must be a string");
  if (!args.IsUnsigned(1)) TYPE_ERROR("uid must be an unsigned int");
  if (!args.IsUnsigned(2)) TYPE_ERROR("gid must be an unsigned int");

  JS_LOCAL_VALUE arg0 = GET_ARG(0);
  jxcore::JXString path(arg0);
  uv_uid_t uid = static_cast<uv_uid_t>(args.GetUInteger(1));
  uv_gid_t gid = static_cast<uv_gid_t>(args.GetUInteger(2));

  if (args.IsFunction(3)) {
    JS_LOCAL_VALUE arg3 = GET_ARG(3);
    ASYNC_CALL(chown, arg3, *path, uid, gid);
  } else {
    SYNC_CALL(chown, *path, *path, uid, gid);
    RETURN();
  }
}
JS_METHOD_END

/* fs.fchown(fd, uid, gid);
 * Wrapper for fchown(1) / EIO_FCHOWN
 */
JS_METHOD(File, FChown) {
  int len = args.Length();
  if (len < 1) TYPE_ERROR("fd required");
  if (len < 2) TYPE_ERROR("uid required");
  if (len < 3) TYPE_ERROR("gid required");
  if (!args.IsInteger(0)) TYPE_ERROR("fd must be an int");
  if (!args.IsUnsigned(1)) TYPE_ERROR("uid must be an unsigned int");
  if (!args.IsUnsigned(2)) TYPE_ERROR("gid must be an unsigned int");

  int fd = args.GetInt32(0);
  uv_uid_t uid = static_cast<uv_uid_t>(args.GetUInteger(1));
  uv_gid_t gid = static_cast<uv_gid_t>(args.GetUInteger(2));

  if (args.IsFunction(3)) {
    JS_LOCAL_VALUE arg3 = GET_ARG(3);
    ASYNC_CALL(fchown, arg3, fd, uid, gid);
  } else {
    SYNC_CALL(fchown, 0, fd, uid, gid);
    RETURN();
  }
}
JS_METHOD_END

JS_METHOD(File, UTimes) {
  int len = args.Length();
  if (len < 1) TYPE_ERROR("path required");
  if (len < 2) TYPE_ERROR("atime required");
  if (len < 3) TYPE_ERROR("mtime required");
  if (!args.IsString(0)) TYPE_ERROR("path must be a string");
  if (!args.IsNumber(1)) TYPE_ERROR("atime must be a number");
  if (!args.IsNumber(2)) TYPE_ERROR("mtime must be a number");

  JS_LOCAL_VALUE arg0 = GET_ARG(0);
  const jxcore::JXString path(arg0);
  const double atime = static_cast<double>(args.GetNumber(1));
  const double mtime = static_cast<double>(args.GetNumber(2));

  if (args.IsFunction(3)) {
    JS_LOCAL_VALUE arg3 = GET_ARG(3);
    ASYNC_CALL(utime, arg3, *path, atime, mtime);
  } else {
    SYNC_CALL(utime, *path, *path, atime, mtime);
    RETURN();
  }
}
JS_METHOD_END

JS_METHOD(File, FUTimes) {
  int len = args.Length();
  if (len < 1) TYPE_ERROR("fd required");
  if (len < 2) TYPE_ERROR("atime required");
  if (len < 3) TYPE_ERROR("mtime required");
  if (!args.IsInteger(0)) TYPE_ERROR("fd must be an int");
  if (!args.IsNumber(1)) TYPE_ERROR("atime must be a number");
  if (!args.IsNumber(2)) TYPE_ERROR("mtime must be a number");

  const int fd = args.GetInt32(0);
  const double atime = static_cast<double>(args.GetNumber(1));
  const double mtime = static_cast<double>(args.GetNumber(2));

  if (args.IsFunction(3)) {
    JS_LOCAL_VALUE arg3 = GET_ARG(3);
    ASYNC_CALL(futime, arg3, fd, atime, mtime);
  } else {
    SYNC_CALL(futime, 0, fd, atime, mtime);
    RETURN();
  }
}
JS_METHOD_END

}  // end namespace node

NODE_MODULE(node_fs, node::File::Initialize)
