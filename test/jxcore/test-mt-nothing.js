// Copyright & License details are available under JXCORE_LICENSE file

// This unit is testing running mt/mt-keep process which basically does nothing.
// mt/mt-keep args are defined in .json. file

// do nothing

if (process.threadId !== -1)
  process.release();

