// Copyright & License details are available under JXCORE_LICENSE file

// This test currently hangs on Windows.
if (process.platform === 'win32') {
  console.error('Skipping: platform is Windows.');
  process.exit(0);
}

process.stdin.resume();
process.stdin._handle.close();
process.stdin._handle.unref();  // Should not segfault.