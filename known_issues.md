#Known issues

Feel free to contribute to the project for the below urgent list of items:

* Windows / FreeBSD compilation scripts for SpiderMonkey build.
* SpiderMonkey build ARM JIT stabilization update and make it primary Android target
* SpiderMonkey Debugger Interface
* REPL issues with SpiderMonkey build (see. tools/test.py simple)
* Embedded interface test cases & MT stability updates
* Improve SpiderMonkey buffer interface

* packages: name conflicting, when calling `require("xxx")` - (node_modules/xxx) from inside xxx.js
```bash
./jx test/run.js -file test/jxcore/test-require-name-conflict.js -p
```

* native packages:  `require.main` !== `module`
```bash
./jx test/run.js -file test/jxcore/test-others-module.js -n
```

* native packages: they do not check *.jxcore.config file:
```bash
./jx test/run.js -file test/jxcore/test-jx.config-portTCP\.js -n
```

* callback for addTask (as object) is not invoked when waitLogic: true

This is probably not a bug. See: [#198](https://github.com/jxcore/jxcore/issues/198)

```bash
./jx test/jxcore/test-tasks-waitLogic.js
```


* once in a while there is segmentation fault inside a task (visible only after the test multiple times)
```bash
./jx test/run.js -file test/jxcore/test-process.cwd-addTask-logic.js -r 100
```
