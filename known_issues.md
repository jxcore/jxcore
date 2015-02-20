Feel free to contribute to the project for the below urgent list of items;

##1 
Windows / FreeBSD compilation scripts for SpiderMonkey build.
 
##2
SpiderMonkey build ARM JIT stabilization update and make it primary Android target

##3
SpiderMonkey Debugger Interface

##4
REPL issues with SpiderMonkey build (see. tools/test.py simple)

##5
Embedded interface test cases & MT stability updates

##6
Improve SpiderMonkey buffer interface 

##7
packages: name conflicting, when calling `require("xxx")` - (node_modules/xxx) from inside xxx.js
./jx test/run.js -file test/jxcore/test-require-name-conflict.js -p

##9
native package:  `require.main` !== `module`
./jx test/run.js -file test/jxcore/test-others-module.js -n

##10
native packages: `./native.exe monitor run/stop/etc` should be disabled for native packages,
because it keeps respawning itself creating new processes infinitively!

##11
native packages: they do not check *.jxcore.config file:
./jx test/run.js -file test/jxcore/test-jx.config-portTCP.js -n


##12
SM: child_process.exec receives error when spawning mt/mt-keep process (which exists naturally)
./jx mt test/jxcore/test-exec-error.js

##13
callback for addTask (as object) is not invoked when waitLogic: true
./jx test/jxcore/test-tasks-waitLogic.js



