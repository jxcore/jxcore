// Copyright & License details are available under JXCORE_LICENSE file


process.stdin.resume();
process.stdin._handle.close();
process.stdin._handle.unref();  // Should not segfault.