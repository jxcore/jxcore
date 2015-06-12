### 1.0.2 Apr 26 2015
  * [[`8470a63678`](https://github.com/level/leveldown/commit/8470a63678)] - s/rvagg\/node-/level\// (Lars-Magnus Skog)
  * [[`9cbf592bea`](https://github.com/level/leveldown/commit/9cbf592bea)] - add documentation about snapshots (Max Ogden)
  * [[`b57827cd29`](https://github.com/level/leveldown/commit/b57827cd29)] - use n instead of nvm for working iojs support (Lars-Magnus Skog)
  * [[`a19927667a`](https://github.com/level/leveldown/commit/a19927667a)] - abstract-leveldown ~2.1.0 (ralphtheninja)
  * [[`95ccdf0850`](https://github.com/level/leveldown/commit/95ccdf0850)] - update logo and copyright (Lars-Magnus Skog)
  * [[`09e89d7abb`](https://github.com/level/leveldown/commit/09e89d7abb)] - updated my email (ralphtheninja)

### 1.0.1 Jan 16 2015
  * [[`6df3ecd6f5`](https://github.com/level/leveldown/commit/6df3ecd6f5)] - nan 1.5 for io.js support (Rod Vagg)
  * [[`5198231a88`](https://github.com/level/leveldown/commit/5198231a88)] - Fix LevelDB builds for modern gcc versions (Sharvil Nanavati)

### 1.0.0 Aug 26 2014
  * NAN@1.3 for Node 0.11.13+ support (@rvagg)
  * Allow writing empty values: null, undefined, '', [] and Buffer(0). Entries come out as '' or Buffer(0) (@ggreer, @juliangruber, @rvagg)
  * Fix clang build (@thlorenz)
  * Massive speed up of iterators by chunking reads (@kesla)
  * Wrap in abstract-leveldown for consistent type-checking across *DOWNs (@kesla)
  * Upgrade to LevelDB 1.17.0 (@kesla)
  * Minor memory leaks
  * Remove compile option that borked EL5 compiles
  * Switch to plain MIT license

### 0.10.2 @ Nov 30 2013

  * Apply fix by @rescrv for long-standing OSX corruption bug, https://groups.google.com/forum/#!topic/leveldb/GXhx8YvFiig (@rvagg / @rescrv)

### 0.10.1 @ Nov 21 2013

  * NAN@0.6 for Node@0.11.6 support, v8::Local<T>::New(val) rewritten to
    NanNewLocal<T>(val) (@rvagg)

### 0.10.0 @ Nov 18 2013

  * Fix array-batch memory leak, levelup/#171 (@rvagg)
  * Fix chained-batch write() segfaults, details in #73, (@rvagg and
    @mcollina)
  * Remove `Persistent` references for all `batch()` operations as
    `WriteBatch` takes an explicit copy of the data (@mcollina and
@rvagg)
  * Upgrade to Snappy 1.1.1 (@rvagg and @no9)
  * Upgrade to NAN@0.5.x (@rvagg)
  * Switch all `callback->Call()`s to `node::MakeCallback()` to properly
    support Node.js domains (@rvagg)
  * Properly enable compression by default (@Kyotoweb)
  * Enable LevelDB's BloomFilter (@Kyotoweb)
  * Upgrade to AbstractLevelDOWN@0.11.x for testing (@rvagg)
  * Add new simple batch() leak tester (@rvagg)

### 0.9.2 @ Nov 02 2013

  * Minor fixes to support Node 0.11.8 and new Linux gcc (warnings) (@rvagg)

### 0.9.1 @ Oct 03 2013

  * Include port_uv.h for Windows compile, added test to suite to make sure this happens every time LevelDB is upgraded (@rvagg)

### 0.9.0 @ Oct 01 2013

  * Upgrade from LevelDB@0.11.0 to LevelDB@0.14.0, includes change from .sst to .ldb file extension for SST files (@rvagg)

### 0.8.3 @ Sept 18 2013

  * Upgrade to nan@0.4.0, better support for latest Node master & support for installing within directory structures containing spaces in directory names (@rvagg)

### 0.8.2 @ Sept 2 2013

  * FreeBSD support (@rvagg, @kelexel)

### 0.8.1 @ Sept 1 2013

  * Fixed some minor V8-level leaks (@rvagg)

### 0.8.0 @ Aug 19 2013

  * Added `gt`, `lt`, `gte`, `lte` for iterators (@dominictarr)
  * Switch to NAN as an npm dependency (@rvagg)

### 0.7.0 @ Aug 11 2013

  * Added @pgte to contributors list
  * (very) Minor perf improvements in C++ (@mscdex)
  * Use NAN <https://github.com/rvagg/nan> for Node 0.8->0.11 compatibility

### 0.6.2 @ Jul 07 2013

  * Compatibility for Node 0.11.3, breaks compatibility with 0.11.2

### 0.6.1 @ Jun 15 2013

  * Fix broken Windows compile, apply port-uv patch to LevelDB's port.h (@rvagg)

### 0.6.0 @ Jun 14 2013

  * Upgrade to LevelDB 1.11.0, some important bugfixes: https://groups.google.com/forum/#!topic/leveldb/vS1JvmGlp4E

### 0.5.0 @ May 18 2013

  * Bumped major version for db.getProperty() addition (should have been done in 0.4.4) (@rvagg)
  * Disallow batch() operations after a write() (@rvagg)

### 0.4.4 @ May 18 2013

  * db.getProperty() implemented, see README for details (@rvagg)
  * More work on memory management, including late-creation of Persistent handles (@rvagg)

### 0.4.3 @ May 18 2013

  * Better memory leak fix (@rvagg)

### 0.2.2 @ May 17 2013

  * BACKPORT memory leak fixes (@rvagg)

### 0.4.2 @ May 17 2013

  * Same memory leak fixes as 0.4.1, properly applied to batch() operations too (@rvagg)

### 0.4.1 @ May 17 2013

  * Fix memory leak caused when passing String objects in as keys and values, conversion to Slice created new char[] but wasn't being disposed. Buffers are automatically disposed. (@rvagg, reported by @kylegetson levelup/#140)

### 0.4.0 @ May 15 2013

  * Upgrade to LevelDB 1.10.0, fairly minor changes, mostly bugfixes see https://groups.google.com/forum/#!topic/leveldb/O2Zdbi9Lrao for more info (@rvagg)

### 0.3.1 @ May 14 2013

  * Don't allow empty batch() operations through to LevelDB, on chained of array forms (@rvagg)

### 0.3.0 (& 0.2.2) @ May 14 2013

  * Pull API tests up into AbstractLevelDOWN, require it to run the tests. AbstractLevelDOWN can now be used to test LevelDOWN-compliant APIs. (@maxogden)
  * Change Iterator methods to return errors on the callbacks rather than throw (@mcollina & @rvagg)

0.2.1 @ Apr 8 2013
==================
  * Start on correct value when reverse=true, also handle end-of-store case #27 (@kesla)
  * Ignore empty string/buffer start/end options on iterators (@kesla)
  * Macro cleanup, replace some with static inline functions (@rvagg)

### 0.2.0 @ Mar 30 2013

  * Windows support--using a combination of libuv and Windows-specific code. See README for details about what's required (@rvagg)
  * leveldown.destroy(location, callback) to delete an existing LevelDB store, exposes LevelDB.DestroyDB() (@rvagg)
  * leveldown.repair(location, callback) to repair an existing LevelDB store, exposes LevelDB.RepairDB() (@rvagg)
  * advanced options: writeBufferSize, blockSize, maxOpenFiles, blockRestartInterval, exposes LevelDB options (@rvagg)
  * chained-batch operations. Argument-less db.batch() will return a new Batch object that can .put() and .del() and then .write(). API in flux so not documented yet. (@juliangruber / @rvagg)
  * auto-cleanup iterators that are left open when you close a database; any iterators left open when you close a database instance will kill your process so we now keep track of iterators and auto-close them before a db.close completes.
  * Node 0.11 support (no compile warnings)

### 0.1.4 @ Mar 11 2013

  * return error when batch ops contain null or undefined (@rvagg / @ralphtheninja / @dominictarr) (additional tests in LevelUP for this)

0.1.3 @ Mar 9 2013
==================
  * add 'standalone_static_library':1 in dependency gyp files to fix SmartOS build problems (@wolfeidau)

### 0.1.2 @ Jan 25 2013

  * upgrade to LevelDB 1.9.0, fairly minor changes since 1.7.0 (@rvagg)
  * upgrade to Snappy 1.1.0, changes block size to improve compression ~3%, slight decrease in speed (@rvagg)

### 0.1.1 @ Jan 25 2013

  * compile error on Mac OS (@kesla / @rvagg)

### 0.1.0 @ Jan 24 2013

  * change API to export single function `levelup()` (@rvagg)
  * move `createIterator()` to `levelup#iterator()` (@rvagg)
  * make all `options` arguments optional (@rvagg)
  * argument number & type checking on all methods (@rvagg)
  * stricter checking on key & value types, String/Object.toString()/Buffer, non-zero-length (@rvagg)
  * remove `use namespace` and add `namespace leveldown` everywhere (@rvagg)
  * race condition in Iterator end()/next() fix, merged from LevelUP (@ralphtheninja / @rvagg)
  * add complete, independent test suite (@rvagg)

### 0.0.1 & 0.0.2 @ Jan 2013

  * finalise rename of internal components to LevelDOWN, removing LevelUP references (@rvagg)
  * complete documentation of current API (@rvagg)

### 0.0.0 @ Jan 06 2013

  * extracted from LevelUP as stand-alone package (@rvagg)
