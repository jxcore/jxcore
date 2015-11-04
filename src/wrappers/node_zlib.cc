// Copyright & License details are available under JXCORE_LICENSE file

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#include "zlib.h"
#include "node.h"
#include "node_zlib.h"
#include "node_buffer.h"

namespace node {

enum node_zlib_mode {
  NONE,
  DEFLATE,
  INFLATE,
  GZIP,
  GUNZIP,
  DEFLATERAW,
  INFLATERAW,
  UNZIP
};

/**
 * Deflate/Inflate
 */
class ZCtx : public ObjectWrap {
  friend class NodeZlib;

 public:
  explicit ZCtx(node_zlib_mode mode)
      : ObjectWrap(),
        init_done_(false),
        level_(0),
        windowBits_(0),
        memLevel_(0),
        strategy_(0),
        err_(0),
        dictionary_(NULL),
        dictionary_len_(0),
        flush_(0),
        chunk_size_(0),
        write_in_progress_(false),
        pending_close_(false),
        mode_(mode) {}

  ~ZCtx() {
    assert(!write_in_progress_ && "write in progress");
    Close();
  }

  void Close() {
    if (write_in_progress_) {
      pending_close_ = true;
      return;
    }

    pending_close_ = false;
    assert(init_done_ && "close before init");
    assert(mode_ <= UNZIP);

    JS_DEFINE_CURRENT_MARKER();

    if (mode_ == DEFLATE || mode_ == GZIP || mode_ == DEFLATERAW) {
      (void)deflateEnd(&strm_);
      JS_ADJUST_EXTERNAL_MEMORY(-kDeflateContextSize);
    } else if (mode_ == INFLATE || mode_ == GUNZIP || mode_ == INFLATERAW ||
               mode_ == UNZIP) {
      (void)inflateEnd(&strm_);
      JS_ADJUST_EXTERNAL_MEMORY(-kInflateContextSize);
    }
    mode_ = NONE;

    if (dictionary_ != NULL) {
      delete[] dictionary_;
      dictionary_ = NULL;
    }
  }

  // thread pool!
  // This function may be called multiple times on the uv_work pool
  // for a single write() call, until all of the input bytes have
  // been consumed.
  static void Process(uv_work_t *work_req) {
    ZCtx *ctx = container_of(work_req, ZCtx, work_req_);

    // If the avail_out is left at 0, then it means that it ran out
    // of room.  If there was avail_out left over, then it means
    // that all of the input was consumed.
    switch (ctx->mode_) {
      case DEFLATE:
      case GZIP:
      case DEFLATERAW:
        ctx->err_ = deflate(&ctx->strm_, ctx->flush_);
        break;
      case UNZIP:
      case INFLATE:
      case GUNZIP:
      case INFLATERAW:
        ctx->err_ = inflate(&ctx->strm_, ctx->flush_);

        // If data was encoded with dictionary
        if (ctx->err_ == Z_NEED_DICT && ctx->dictionary_ != NULL) {
          // Load it
          ctx->err_ = inflateSetDictionary(&ctx->strm_, ctx->dictionary_,
                                           ctx->dictionary_len_);
          if (ctx->err_ == Z_OK) {
            // And try to decode again
            ctx->err_ = inflate(&ctx->strm_, ctx->flush_);
          } else if (ctx->err_ == Z_DATA_ERROR) {
            // Both inflateSetDictionary() and inflate() return Z_DATA_ERROR.
            // Make it possible for After() to tell a bad dictionary from bad
            // input.
            ctx->err_ = Z_NEED_DICT;
          }
        }
        break;
      default:
        assert(0 && "wtf?");
    }

    // pass any errors back to the main thread to deal with.

    // now After will emit the output, and
    // either schedule another call to Process,
    // or shift the queue and call Process.
  }

  static void After(uv_work_t *work_req, int status) {
    assert(status == 0);

    JS_ENTER_SCOPE_COM();
    JS_DEFINE_STATE_MARKER(com);
    ZCtx *ctx = container_of(work_req, ZCtx, work_req_);

    // Acceptable error states depend on the type of zlib stream.
    switch (ctx->err_) {
      case Z_OK:
      case Z_STREAM_END:
      case Z_BUF_ERROR:
        // normal statuses, not fatal
        break;
      case Z_NEED_DICT:
        if (ctx->dictionary_ == NULL) {
          ZCtx::Error(ctx, "Missing dictionary");
        } else {
          ZCtx::Error(ctx, "Bad dictionary");
        }
        return;
      default:
        // something else.
        ZCtx::Error(ctx, "Zlib error");
        return;
    }

    JS_LOCAL_OBJECT objh = JS_OBJECT_FROM_PERSISTENT(ctx->handle_);
    JS_LOCAL_INTEGER avail_out = STD_TO_INTEGER(ctx->strm_.avail_out);
    JS_LOCAL_INTEGER avail_in = STD_TO_INTEGER(ctx->strm_.avail_in);
    ctx->write_in_progress_ = false;
    // call the write() cb
    assert(JS_IS_FUNCTION(JS_GET_NAME(objh, JS_PREDEFINED_STRING(callback))) &&
           "Invalid callback");
    JS_LOCAL_VALUE args[2] = {avail_in, avail_out};
    MakeCallback(com, objh, STD_TO_STRING("callback"), ARRAY_SIZE(args), args);

    ctx->Unref();
    if (ctx->pending_close_) ctx->Close();
  }

  static void Error(ZCtx *ctx, const char *msg_) {
    const char *msg;
    if (ctx->strm_.msg != NULL) {
      msg = ctx->strm_.msg;
    } else {
      msg = msg_;
    }
    JS_ENTER_SCOPE_COM();
    JS_DEFINE_STATE_MARKER(com);

    JS_LOCAL_OBJECT objh = JS_OBJECT_FROM_PERSISTENT(ctx->handle_);
    assert(JS_IS_FUNCTION(JS_GET_NAME(objh, JS_PREDEFINED_STRING(onerror))) &&
           "Invalid error handler");

    JS_LOCAL_VALUE args[2] = {STD_TO_STRING(msg), STD_TO_NUMBER(ctx->err_)};

    MakeCallback(com, objh, STD_TO_STRING("onerror"), ARRAY_SIZE(args), args);

    // no hope of rescue.
    if (ctx->write_in_progress_) ctx->Unref();
    ctx->write_in_progress_ = false;

    if (ctx->pending_close_) ctx->Close();
  }

  static void Init(ZCtx *ctx, int level, int windowBits, int memLevel,
                   int strategy, char *dictionary, size_t dictionary_len) {
    ctx->level_ = level;
    ctx->windowBits_ = windowBits;
    ctx->memLevel_ = memLevel;
    ctx->strategy_ = strategy;

    ctx->strm_.zalloc = Z_NULL;
    ctx->strm_.zfree = Z_NULL;
    ctx->strm_.opaque = Z_NULL;

    ctx->flush_ = Z_NO_FLUSH;

    ctx->err_ = Z_OK;

    if (ctx->mode_ == GZIP || ctx->mode_ == GUNZIP) {
      ctx->windowBits_ += 16;
    }

    if (ctx->mode_ == UNZIP) {
      ctx->windowBits_ += 32;
    }

    if (ctx->mode_ == DEFLATERAW || ctx->mode_ == INFLATERAW) {
      ctx->windowBits_ *= -1;
    }

    JS_DEFINE_CURRENT_MARKER();

    switch (ctx->mode_) {
      case DEFLATE:
      case GZIP:
      case DEFLATERAW:
        ctx->err_ =
            deflateInit2(&ctx->strm_, ctx->level_, Z_DEFLATED, ctx->windowBits_,
                         ctx->memLevel_, ctx->strategy_);
        JS_ADJUST_EXTERNAL_MEMORY(kDeflateContextSize);
        break;
      case INFLATE:
      case GUNZIP:
      case INFLATERAW:
      case UNZIP:
        ctx->err_ = inflateInit2(&ctx->strm_, ctx->windowBits_);
        JS_ADJUST_EXTERNAL_MEMORY(kInflateContextSize);
        break;
      default:
        assert(0 && "wtf?");
    }

    if (ctx->err_ != Z_OK) {
      ZCtx::Error(ctx, "Init error");
    }

    ctx->dictionary_ = reinterpret_cast<Bytef *>(dictionary);
    ctx->dictionary_len_ = dictionary_len;

    ctx->write_in_progress_ = false;
    ctx->init_done_ = true;
  }

  static void SetDictionary(ZCtx *ctx) {
    if (ctx->dictionary_ == NULL) return;

    ctx->err_ = Z_OK;

    switch (ctx->mode_) {
      case DEFLATE:
      case DEFLATERAW:
        ctx->err_ = deflateSetDictionary(&ctx->strm_, ctx->dictionary_,
                                         ctx->dictionary_len_);
        break;
      default:
        break;
    }

    if (ctx->err_ != Z_OK) {
      ZCtx::Error(ctx, "Failed to set dictionary");
    }
  }

  static void Reset(ZCtx *ctx) {
    ctx->err_ = Z_OK;

    switch (ctx->mode_) {
      case DEFLATE:
      case DEFLATERAW:
        ctx->err_ = deflateReset(&ctx->strm_);
        break;
      case INFLATE:
      case INFLATERAW:
        ctx->err_ = inflateReset(&ctx->strm_);
        break;
      default:
        break;
    }

    if (ctx->err_ != Z_OK) {
      ZCtx::Error(ctx, "Failed to reset stream");
    }
  }

 private:
  static const int kDeflateContextSize = 16384;  // approximate
  static const int kInflateContextSize = 10240;  // approximate

  bool init_done_;

  z_stream strm_;
  int level_;
  int windowBits_;
  int memLevel_;
  int strategy_;

  int err_;

  Bytef *dictionary_;
  size_t dictionary_len_;

  int flush_;

  int chunk_size_;

  bool write_in_progress_;
  bool pending_close_;

  uv_work_t work_req_;
  node_zlib_mode mode_;
};

JS_METHOD(NodeZlib, New) {
  if (args.Length() < 1 || !args.IsInteger(0)) {
    THROW_TYPE_EXCEPTION("Bad argument");
  }
  node_zlib_mode mode = (node_zlib_mode)args.GetInt32(0);

  if (mode < DEFLATE || mode > UNZIP) {
    THROW_TYPE_EXCEPTION("Bad argument");
  }

  JS_CLASS_NEW_INSTANCE(obj, Zlib);
  ZCtx *ctx = new ZCtx(mode);
  ctx->Wrap(obj);
  RETURN_POINTER(obj);
}
JS_METHOD_END

JS_METHOD(NodeZlib, Close) {
  ZCtx *ctx = ObjectWrap::Unwrap<ZCtx>(args.This());
  ctx->Close();
}
JS_METHOD_END

JS_METHOD(NodeZlib, Write) {
  assert(args.Length() == 7);

  ZCtx *ctx = ObjectWrap::Unwrap<ZCtx>(args.This());
  assert(ctx->init_done_ && "write before init");
  assert(ctx->mode_ != NONE && "already finalized");

  assert(!ctx->write_in_progress_ && "write already in progress");
  assert(!ctx->pending_close_ && "close is pending");
  ctx->write_in_progress_ = true;
  ctx->Ref();

  assert(!args.IsUndefined(0) && "must provide flush value");

  unsigned int flush = args.GetUInteger(0);

  if (flush != Z_NO_FLUSH && flush != Z_PARTIAL_FLUSH &&
      flush != Z_SYNC_FLUSH && flush != Z_FULL_FLUSH && flush != Z_FINISH &&
      flush != Z_BLOCK) {
    assert(0 && "Invalid flush value");
  }

  Bytef *in;
  Bytef *out;
  size_t in_off, in_len, out_off, out_len;

  if (args.IsNull(1)) {
    // just a flush
    Bytef nada[1] = {0};
    in = nada;
    in_len = 0;
    in_off = 0;
  } else {
    JS_LOCAL_OBJECT in_buf;
    in_buf = JS_VALUE_TO_OBJECT(args.GetItem(1));
    in_off = args.GetUInteger(2);
    in_len = args.GetUInteger(3);

    assert(Buffer::IsWithinBounds(in_off, in_len, BUFFER__LENGTH(in_buf)));
    in = reinterpret_cast<Bytef *>(BUFFER__DATA(in_buf) + in_off);
  }

  JS_LOCAL_OBJECT out_buf = JS_VALUE_TO_OBJECT(args.GetItem(4));
  out_off = args.GetUInteger(5);
  out_len = args.GetUInteger(6);
  assert(Buffer::IsWithinBounds(out_off, out_len, BUFFER__LENGTH(out_buf)));
  out = reinterpret_cast<Bytef *>(BUFFER__DATA(out_buf) + out_off);

  // build up the work request
  uv_work_t *work_req = &(ctx->work_req_);

  ctx->strm_.avail_in = in_len;
  ctx->strm_.next_in = in;
  ctx->strm_.avail_out = out_len;
  ctx->strm_.next_out = out;
  ctx->flush_ = flush;

  // set this so that later on, I can easily tell how much was written.
  ctx->chunk_size_ = out_len;

  uv_queue_work(com->loop, work_req, ZCtx::Process, ZCtx::After);

  JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(ctx->handle_);
  RETURN_POINTER(objl);
}
JS_METHOD_END

// just pull the ints out of the args and call the other Init
JS_METHOD(NodeZlib, Init) {
  assert((args.Length() == 4 || args.Length() == 5) &&
         "init(windowBits, level, memLevel, strategy, [dictionary])");

  ZCtx *ctx = ObjectWrap::Unwrap<ZCtx>(args.This());

  int windowBits = args.GetUInteger(0);
  assert((windowBits >= 8 && windowBits <= 15) && "invalid windowBits");

  int level = args.GetInt32(1);
  assert((level >= -1 && level <= 9) && "invalid compression level");

  int memLevel = args.GetUInteger(2);
  assert((memLevel >= 1 && memLevel <= 9) && "invalid memlevel");

  int strategy = args.GetUInteger(3);
  assert((strategy == Z_FILTERED || strategy == Z_HUFFMAN_ONLY ||
          strategy == Z_RLE || strategy == Z_FIXED ||
          strategy == Z_DEFAULT_STRATEGY) &&
         "invalid strategy");

  char *dictionary = NULL;
  size_t dictionary_len = 0;
  if (args.Length() >= 5 && Buffer::jxHasInstance(args.GetItem(4), com)) {
    JS_LOCAL_OBJECT dictionary_ = JS_VALUE_TO_OBJECT(args.GetItem(4));

    dictionary_len = BUFFER__LENGTH(dictionary_);
    dictionary = new char[dictionary_len];

    memcpy(dictionary, BUFFER__DATA(dictionary_), dictionary_len);
  }

  ZCtx::Init(ctx, level, windowBits, memLevel, strategy, dictionary,
             dictionary_len);
  ZCtx::SetDictionary(ctx);
}
JS_METHOD_END

JS_METHOD(NodeZlib, Reset) {
  ZCtx *ctx = ObjectWrap::Unwrap<ZCtx>(args.This());

  ZCtx::Reset(ctx);
  ZCtx::SetDictionary(ctx);
}
JS_METHOD_END

void NodeZlib::DefinePrivateConstants(commons *com, JS_HANDLE_OBJECT target) {
  JS_DEFINE_STATE_MARKER(com);

  NODE_DEFINE_CONSTANT(target, DEFLATE);
  NODE_DEFINE_CONSTANT(target, INFLATE);
  NODE_DEFINE_CONSTANT(target, GZIP);
  NODE_DEFINE_CONSTANT(target, GUNZIP);
  NODE_DEFINE_CONSTANT(target, DEFLATERAW);
  NODE_DEFINE_CONSTANT(target, INFLATERAW);
  NODE_DEFINE_CONSTANT(target, UNZIP);
}

}  // namespace node

NODE_MODULE(node_zlib, node::NodeZlib::Initialize)
