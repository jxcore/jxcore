# Internal Recovery

In addition to recovering the process of a crashed applications by external [monitoring process](jxcore-command-monitor.markdown),
JXcore also provides automatic Internal Process Recovery as well as Internal Instance Recovery (for code running in multitasking mode).

## Internal Process Recovery

Internal Process Recovery  can be enabled by attaching any callback to `restart` event **inside a main instance**.
When an exception occurs inside the application, the callback is invoked and you can decide if to allow restart of the entire process or not.
Also, this is a good opportunity to save some data used by the application, which would otherwise be lost after the crash.

```js
process.on('restart', function (restartCallback, newExitCode) {
    // do whatever you want before application's crash
    // and when you're done - call the callback to restart the process
    restartCallback();
});
```

Arguments for the callback:

* `restartCallback` {Function}
    * `newExitCode` {Number}
* `currentExitCode` {Number}

The `restartCallback` is a function, which should be invoked if you want to allow for application restart/recovery.
Unless it's used, the process will exit (after all it has just crashed) without restart.

Any tasks like saving application's data (objects, variables etc) into database, must be done before calling `restartCallback`.

When invoking this callback, you can pass an `newExitCode`, that you want the application's process to be restarted with.
Current process will exit with this code and the application will be relaunched as a new process.
`newExitCode` for the callback is optional. When omitted, the current application's exit code will be used.

`currentExitCode` - holds current exit code of the application, which is just about to crash.

In the example below we are throwing an exception, which causes `restart` event to be fired. Just before the restart, we're adding new parameter to `process.argv`, and this is the way to pass an argument to the new process.
We do this in order to prevent circular recoveries.

The code should be launched as single-instanced (without `mt`/`mt-keep` parameter), because it handles the recovery of the main process:

    > jx sample.js

sample.js:

```js
process.on('restart', function (restartCallback, newExitCode) {

    //in order to prevent circular recoveries only recover once!
    if (process.argv[process.argv.length - 1] != "111") {
        process.argv[process.argv.length] = 111;

        console.log("Restarting");
        //calling restartCB
        restartCallback(123);
    }
    else {
        // we're not calling restartCallback(), so the application will exit
        // without restarting
        console.log("Skipping restart.");
    }
});


// JXcore does not auto-restart application if it dies under 5000 ms
setTimeout(function () {
    throw "";
}, 5200);
```

There are two situations, when Internal Process Recovery is not performed:

1. Application was alive for less than 5000 milliseconds. This value will be configurable in the future, but for now it's a constant.
2. When any listener is attached to `process.on("uncaughtException")` event, the process recovery is not active.
It makes sense only for uncaught exceptions, which in this case are actually caught by `uncaughtException` event.

## Internal Instance Recovery

This is analogous to [Internal Process Recovery](#internal-process-recovery), except that it concerns a **sub-instance** rather than main application process.

Attaching any callback to this event inside a code running in a sub-instance enables internal sub-instance recovery.
When an exception occurs inside the sub-instance, the callback is invoked. You can then decide to allow restart of the instance or not.
Also this is a good opportunity to save some data used by this particular sub-instance which would otherwise be lost after the crash.

```js
process.on('restart', function (restartCallback) {
    // do whatever you want before instance will crash
    // and when you're done - call the callback to restart the instance
    restartCallback();
});
```

Argument for the callback:

* `restartCallback` {Function}

This function should be invoked to allow for sub-instance recovery. Unless it is called, the sub-instance will die without restart.
Any tasks, like saving some data (objects, variables etc) into database or even shared memory store, must be done before calling `restartCallback`.

In the example below we are throwing an exception, which causes restart event to be fired.
Also we are counting how many times the sub-instance was restarted. Based on that value we decide when to stop.

The code should be run with mt-keep parameter:

    > jx mt-keep sample.js

sample.js:

```js
var shared = jxcore.store.shared;
var sid = "threadRestartCount_" + process.threadId;

var counter = 0;

// this is just for storing restart counter of the instance.
// we use jxcore.store.shared here, because it is static (unrelated to the instances)
if (shared.exists(sid)) {
    counter = parseInt(shared.get(sid)) + 1;
    console.log("Thread no %s restarted %d times.", process.threadId, counter);
}
shared.set(sid, counter);

// attaching callback to this event enables the sub-instance recovery,
// but still you need to call restartCallback() explicitly to make a restart
process.on('restart', function (restartCallback) {
    if (counter < 3) {
        // we don't want to allow for infinitive restarts. 3 is enough.
        console.log("Restarting thread no " + process.threadId);
        restartCallback();
    } else {
        console.log("Thread no %s was restarted %d times. We'll not restart any more.",
            process.threadId, counter);
        // releasing the thread allows main process to exit,
        // when all the threads are released.
        process.release();
    }
});

// this loop is used to throw an exception inside a sub-instance
setTimeout(function () {
    throw "Let's restart the sub-instance!";
}, 100 * counter * process.threadId);
```

Please note, that when any listener is attached to `process.on("uncaughtException")` event, the sub-instance recovery is not active, because it makes sense only for uncaught exceptions, which in this case are actually caught by `uncaughtException` event.

# Internal Recovery vs Process Monitor

Process Monitor and Internal Process Recovery should not be used simultaneously. Both of them perform restart of the application's process,
so they could interfere with each other leading to an unexpected behavior.
For example, the application could be respawned into multiple instances, or fall into uncontrolled loop of restarting.

On the other hand, when your application is running in multi-tasked mode, you can still use Internal Instance Recovery
(which allows to restart crashed sub-instances, not the main application's process) together with Process Monitor.
