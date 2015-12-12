// Copyright & License details are available under JXCORE_LICENSE file

#include "jxcore.h"
#include "jx/job.h"

void run(int argc, char *argv[]) {
  jxcore::JXEngine::DefineGlobals();

  jxcore::JXEngine engine(argc, argv, true);
  engine.Initialize();

  engine.Start();

  engine.ShutDown();
}

#ifdef _WIN32
int wmain(int argc, wchar_t *wargv[]) {
  // Convert argv to to UTF8
  char **argv = new char *[argc];
  for (int i = 0; i < argc; i++) {
    // Compute the size of the required buffer
    DWORD size =
        WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, NULL, 0, NULL, NULL);
    if (size == 0) {
      // This should never happen.
      fprintf(stderr, "Could not convert arguments to utf8.");
      exit(1);
    }
    // Do the actual conversion
    argv[i] = new char[size];
    DWORD result = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, argv[i], size,
                                       NULL, NULL);
    if (result == 0) {
      // This should never happen.
      fprintf(stderr, "Could not convert arguments to utf8.");
      exit(1);
    }
  }

  run(argc, argv);
  return 0;
}
#else
// UNIX
int main(int argc, char *argv[]) {
  run(argc, argv);

  return 0;
}
#endif
