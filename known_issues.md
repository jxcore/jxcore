#Known issues

Feel free to contribute to the project for the below urgent list of items:

##1
Windows / FreeBSD compilation scripts for SpiderMonkey build.

##2
Make SpiderMonkey as primary Android engine

##3
SpiderMonkey Debugger Interface

##4
REPL issues with SpiderMonkey build (see. tools/test.py simple)

##5
Embedded interface test cases & MT stability updates

##6
packages: name conflicting, when calling `require("xxx")` - (node_modules/xxx) from inside xxx.js
```bash
./jx test/run.js -file test/jxcore/test-require-name-conflict.js -p
```

##7
native packages:  `require.main` !== `module`
```bash
./jx test/run.js -file test/jxcore/test-others-module.js -n
```

##8
native packages: they do not check *.jxcore.config file:
```bash
./jx test/run.js -file test/jxcore/test-jx.config-portTCP\.js -n
```

##9
callback for addTask (as object) is not invoked when waitLogic: true

This is probably not a bug. See: [#198](https://github.com/jxcore/jxcore/issues/198)

```bash
./jx test/jxcore/test-tasks-waitLogic.js
```
