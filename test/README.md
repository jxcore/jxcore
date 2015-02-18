
# Test cases

Each of test cases by default is executed in three forms:

* as plain JavaScript file
* as jx package
* as native jx package

# .json config file

However in some cases native package should not be tested (e.g. when is spawning a child by `process.execPath`).
Also some of test cases are designed to run multithreaded (mt/mt-keep) and/or receive extra arguments.
Such cases may be implemented by using .json config file.

For example, a test-case named *test-mt-values.js* may have it's configuration stored in *test-mt-values.js.json*.
Please note, that we add ".json" to the end of full filename (including ".js" extension).

If there is no .json file present - a test is executed as described on top of this document.

```js
{
  "args": [
    {},
    {"execArgv": "mt-keep:4"},
    {"argv": "some parameters 123 abc"},
    {
      "execArgv": "mt",
      "argv": 555
    }
  ],
  "files": [
    "node_modules/test-require-name-conflict"
  ],
  "dependencies : [
  	"express@4.9.5", "passport"
  ],
  "runtime_dependencies" : [
  	"express"
  ],
  "native": false,
  "package": false,
}
```

* **runtime_dependencies** - array of npm modules names (may include versions e.g. "express@4.9.5"), which should be installed prior to executing a test case
* **args** - array of argument sets. Each of set may have 0, 1 or 2 values ("execArgv" and/or "argv")

- `{}` - jx test-case.js
- `{"execArgv": "mt"}` - jx mt test-case.js
- `{"argv": "some parameters 123 abc"}` - jx test-case.js some parameters 123 abc

The **args** and **runtime_dependencies** are supported by *testcfg.py* file.

The other fields below are supported for now only by *test/run.js* helper launcher.

* **files** - array of files/folder to be copied to output test directory or packed into jx/native package
* **dependencies** - array of npm modules names (may include versions e.g. "express@4.9.5"), which should be installed prior to creating a package,
	so they can be embedded
* **native** - when set to false, does not create -native package
* **package** - when set to false, does not create regular jx package

# test/run.js

This is helper launcher, which allow for selective test run (whether only .js, .jx, .exe or any combination of them), e.g:
```bash
./node test/run.js jxcore simple -p -n
```

Also it allows to run a test for a single file test-case:
```bash
./node test/run.js - file test/jxcore/test-case.js -a
```

--help

Usage:

	run.js [-j | -p | -n | -a | -v | -f] [test_folder1 test_folder2 ...] -file test_folder/test_file.js

	-j    tests only .js test cases. Default when omitted
	-p     tests only packaged test cases
	-n     tests only native packaged test cases
	-a     tests all: -j, -p, -n
	-s     silent: hides extra messages
	-f     packages are created once per jx version. Use -f to force refresh them
	-file  allows to execute test just for one js file

	examples:
		run.js                 - runs .js tests from all subfolders of 'test'
		run.js jxcore          - runs .js tests for 'jxcore'
		run.js jxcore simple   - runs .js tests for 'jxcore' and 'simple'
		run.js -p jxcore       - compiles all ./test/jxcore/*.js files into .jx packages
					('jxcore_package' output folder) and runs them
		run.js -n simple       - compiles all ./test/simple/*.js files into native packages
					('simple_package' output folder) and runs them

