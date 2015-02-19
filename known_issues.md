
#1

`process.on('restart')` not called for thread 0 (tasks):
./jx test/run.js -file test/jxcore/test-recovery-task-runOnce.js

#2

packages:  unicode strings breaks js code and random errors occur:
./jx test/run.js -file test/jxcore/test-unicode.js -p
./jx test/run.js -file test/jxcore/test-unicode2.js –p

#3

native packages: `__dirname` is wrong:
./jx test/run.js -file test/jxcore/test-others-__dirname.js -n

and because of this they cannot find ../file.js (it is outside the package):
./jx test/run.js -file test/jxcore/test-require-parentdir.js -n

#4

~~mt/mt-keep doesn't work. Thread is restarting with error:
restarting thread 0 due to error Error: Cannot find module 'mt'~~

#5

error for packaged js file containing shebang line:
./jx test/run.js -file test/jxcore/test-shebang.js -p

#6

packages: name conflicting, when calling `require("xxx")` - (node_modules/xxx) from inside xxx.js
./jx test/run.js -file test/jxcore/test-require-name-conflict.js -p

#7

thread exception handling behavior changed? threads restart under 5 secs now.

#8

native package:  `require.main` !== `module`
./jx test/run.js -file test/jxcore/test-others-module.js -n

#9

to be checked for uncaught exception + Abort trap:
./jx test/messageMozJS/throw_custom_error.js
./jx test/messageMozJS/throw_in_line_with_tabs.js
./jx test/messageMozJS/throw_non_error.js

./jx test/messageMozJS/eval_messages.js
./jx test/messageMozJS/stdin_messages.js
./jx test/messageMozJS/undefined_reference_in_new_context.js

#10

jx/native packages: when reading assets "asset/file.txt" should be combined with `__dirname`, not with `process.cwd()`

#11

native packages: `./native.exe monitor run/stop/etc` should be disabled for native packages,
because it keeps respawning itself creating new processes infinitively!

#12

native packages: they are not loading/seeing *.jxcore.config file:
./jx test/run.js -file test/jxcore/test-jx.config-portTCP.js -n

#13

running mt/mt-keep for non existing file:

```
./jx mt fake.js
[ 1, 0, 0 ]
[ 1, 0, 0 ]

restarting thread 0 due to error TypeError: n.charCodeAt is not a function
restarting thread 1 due to error TypeError: n.charCodeAt is not a function
```

fileexist should be probably checked before spawning a process?

#14

`process.on('exit')` does not fire for naturally exiting mt/mt-keep app.
./jx mt test/jxcore/test-process-on-exit.js


#15

child_process.exec receives error when spawning mt/mt-keep process (which exists naturally)
./jx mt test/jxcore/test-exec-error.js

#16

callback for addTask (as object) is not invoked when waitLogic: true
./jx test/jxcore/test-tasks-waitLogic.js



