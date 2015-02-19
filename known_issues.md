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
`process.on('restart')` not called for thread 0 (tasks):
./jx test/run.js -file test/jxcore/test-recovery-task-runOnce.js

##8
packages:  unicode strings breaks js code and random errors occur:sudo
./jx test/run.js -file test/jxcore/test-unicode.js -p
./jx test/run.js -file test/jxcore/test-unicode2.js –p

##9
native packages: `__dirname` is wrong:
./jx test/run.js -file test/jxcore/test-others-__dirname.js -n

Because of this it cannot find ../file.js (it is outside the package):
./jx test/run.js -file test/jxcore/test-require-parentdir.js -n

##10
error for packaged js file containing shebang line:
./jx test/run.js -file test/jxcore/test-shebang.js -p

##11
packages: name conflicting, when calling `require("xxx")` - (node_modules/xxx) from inside xxx.js
./jx test/run.js -file test/jxcore/test-require-name-conflict.js -p

##12
thread exception handling behavior changed? threads restart under 5 secs now.

##13
native package:  `require.main` !== `module`
./jx test/run.js -file test/jxcore/test-others-module.js -n

##14
to be checked for uncaught exception + Abort trap:
./jx test/messageMozJS/throw_custom_error.js
./jx test/messageMozJS/throw_in_line_with_tabs.js
./jx test/messageMozJS/throw_non_error.js

./jx test/messageMozJS/eval_messages.js
./jx test/messageMozJS/stdin_messages.js
./jx test/messageMozJS/undefined_reference_in_new_context.js

##15
jx/native packages: when reading assets "asset/file.txt" should be combined with `__dirname`, not with `process.cwd()`

##16
`jx compile test.jxp -native` creates a native package with output filename = "-native" (instead of e.g "test").
However it works fine if jxp contains "native" : true.
Also `jx package -native` works

##17
native packages: `./native.exe monitor run/stop/etc` should be disabled for native packages,
because it keeps respawning itself creating new processes infinitively!

##18
native packages: they are not loading/seeing *.jxcore.config file:
./jx test/run.js -file test/jxcore/test-jx.config-portTCP.js -n

##19
running mt/mt-keep for non existing file:

```
./jx mt fake.js
[ 1, 0, 0 ]
[ 1, 0, 0 ]

restarting thread 0 due to error TypeError: n.charCodeAt is not a function
restarting thread 1 due to error TypeError: n.charCodeAt is not a function
```

fileexist should be probably checked before spawning a process?

##20
`process.on('exit')` does not fire for naturally exiting mt/mt-keep app.
./jx mt test/jxcore/test-process-on-exit.js


##21
child_process.exec receives error when spawning mt/mt-keep process (which exists naturally)
./jx mt test/jxcore/test-exec-error.js

##22
callback for addTask (as object) is not invoked when waitLogic: true
./jx test/jxcore/test-tasks-waitLogic.js



