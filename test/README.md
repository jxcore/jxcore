
# Test cases

Each of test cases by default can executed in three forms:

* as plain JavaScript file
* as jx package
* as native jx package (standalone binary)

# .json config file

However in some cases native packages should not be tested (e.g. when is spawning a child by `process.execPath`).
Also some of test cases are designed to run multithreaded (mt/mt-keep) and/or receive extra arguments.
Such cases may be implemented by using `.json` config file.

For example, a test-case named *test-mt-values.js* may have it's configuration stored in *test-mt-values.js.json*.
Please note, that we add ".json" to the end of full filename (including ".js" extension).

If there is no `.json` file present - a test is executed as described on top of this document.

```js
{
  "args": [
    {},
    {"execArgv": "mt-keep:4"},
    {"argv": "some parameters 123 abc"},
    {
      "execArgv": "mt",
      "argv": 555
    },
    {
      "repeat" : 5
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
  "slim" : "node_modules"
}
```

* **runtime_dependencies** - array of npm modules names (may include versions e.g. "express@4.9.5"), which should be installed prior to executing a test case
* **args** - array of argument sets. Each of set may have 0, 1 or multiple values ("execArgv", "argv"  and/or "repeat")

- `{}` - jx test-case.js
- `{ "repeat" : 5 }` - repeats 5 times: jx test-case.js
- `{"execArgv": "mt"}` - jx mt test-case.js
- `{"argv": "some parameters 123 abc"}` - jx test-case.js some parameters 123 abc

The **args** and **runtime_dependencies** are supported by *testcfg.py* file.

The other fields below are supported for now only by *test/run.js* helper launcher.

* **files** - array of files/folders to be copied to output test directory or packed into jx/native package.
If test is performed against jx/native package, then files/folders defined here will be removed after the package is created.
If you want to keep them in filesystem during test unit execution (particularly useful when combined with **slim**),
you can prefix the path with **&** sign, e.g.:

``js
  "files": [
    "&node_modules/test-require-name-conflict"
  ],
  "slim" : [ "node_modules" ]
```

* **dependencies** - array of npm modules names (may include versions e.g. "express@4.9.5"), which should be installed prior to creating a package,
	so they can be embedded
* **native** - when set to false, does not create -native package
* **package** - when set to false, does not create regular jx package
* **slim** - value of `-slim` option with will be passed to `jx package` command which creates packaged test unit.

# test/run.js

This is helper launcher, which allow for selective test run (whether only `.js`, `.jx`, `.exe` or any combination of them), e.g:
```bash
./jx test/run.js jxcore simple -p -n
```

Also it allows to run a test for a single file test-case:
```bash
./jx test/run.js - file test/jxcore/test-case.js -a
```

You can specify here number of repetitions per each test with -r switch:
```bash
./jx test/run.js - file test/jxcore/test-case.js -a -r 3
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
	-r     repeats give test/tests X times. This value overrides a value given in test.json file
	-nc    no cleanup - do not remove temporary folders (useful for further inspection)
	-file  allows to execute test just for one js file

	examples:
		run.js                 - runs .js tests from all subfolders of 'test'
		run.js jxcore          - runs .js tests for 'jxcore'
		run.js jxcore simple   - runs .js tests for 'jxcore' and 'simple'
		run.js -p jxcore       - compiles all ./test/jxcore/*.js files into .jx packages
					('jxcore_package' output folder) and runs them
		run.js -n simple       - compiles all ./test/simple/*.js files into native packages
					('simple_package' output folder) and runs them

