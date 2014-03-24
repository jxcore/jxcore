# Internal Recovery

In addition to recovering of crashed applications' processes by external [monitoring process](jxcore-command-monitor.markdown),
JXcore also provides automatic Internal Process Recovery as well as Internal Thread Recovery (for code running in multithreaded mode).

## Internal Process Recovery

Attaching any callback to `restart` event **inside a main thread** enables Internal Process Recovery.
When an exception occurs inside the application, the callback is invoked and there you may decide if to allow restart of the entire process or not.
Also this is a good opportunity to save some data used by the application, because after it will crash, normally they would be lost.

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
If you will not call it, the process will exit (after all it has just crashed) but not restart.

Any tasks, that you want to perform, like saving some application's data (objects, variables etc) into database, must be done before calling `restartCallback`.

When invoking this callback, you can pass an `newExitCode`, that you want the application's process to be restarted with:
the current process will exit with this code and after that the application will be relaunched as a new process.
`newExitCode` for the callback is optional. When omitted, the current application's exit code will be used.

`currentExitCode` - holds current exit code of the application, which is just about to crash.

In the example below we are throwing an exception, which causes `restart` event to be fired. Just before the restart, we're adding new parameter to `process.argv`, and this is the way to pass an argument to the new process.
We do this in order to prevent circular recoveries.

The code should be launched as single threaded (without mt/mt-keep parameter), because it handles recovery of the main process:

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


// JXcore does not autorestart application if it dies under 5000 ms
setTimeout(function () {
    throw "";
}, 5200);
```

There are two situations, when Internal Process Recovery is not performed:

1. application was alive less than 5000 milliseconds. This value will be configurable, but for now it's a constant.
2. when any listener is attached to `process.on("uncaughtException")` event, the thread recovery is not active,
because it makes sense only for uncaught exceptions, which in this case are actually caught by `uncaughtException` event.

## Internal Thread Recovery

This is analogous to [Internal Process Recovery](#internal-process-recovery), except that it concerns a **single thread** rather than entire application's process.

Attaching any callback to this event inside a code running in a subthread enables internal thread recovery.
When an exception occurs inside the subthread, the callback is invoked and there you may decide if to allow restart of the thread or not.
Also this is a good opportunity to save some data used by this thread, because after it will crash, normally they would be lost.

```js
process.on('restart', function (restartCallback) {
    // do whatever you want before thread's crash
    // and when you're done - call the callback to restart the thread
    restartCallback();
});
```

Argument for the callback:

* `restartCallback` {Function}

This is a function, which should be invoked if you want to allow for thread recovery. If you will not call it, the thread will die but not restart.
Any tasks, that you want to perform, like saving some thread's data (objects, variables etc) into database or even shared memory store, must be done before calling `restartCallback`.

In the example below we are throwing an exception, which causes `restart` event to be fired. Also we are counting how many times thread was restarted, and based on that we decide if to allow for another restart or not.

The code should be run with mt-keep parameter:

    > jx mt-keep sample.js

sample.js:

```js
var shared = jxcore.store.shared;
var sid = "threadRestartCount_" + process.threadId;

var counter = 0;

// this is just for storing thread's restart counter.
// we use jxcore.store.shared here, because it is static (unrelated to the threads)
if (shared.exists(sid)) {
    counter = parseInt(shared.get(sid)) + 1;
    console.log("Thread no %s restarted %d times.", process.threadId, counter);
}
shared.set(sid, counter);

// attaching callback to this event enables the thread recovery,
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

// this loop is used to throw an exception inside a subthread
setTimeout(function () {
    throw "Let's restart the thread!";
}, 100 * counter * process.threadId);
```

Please note, that when any listener is attached to `process.on("uncaughtException")` event, the thread recovery is not active, because it makes sense only for uncaught exceptions, which in this case are actually caught by `uncaughtException` event.

# Internal Recovery vs Process Monitor

Process Monitor and Internal Process Recovery should not be used simultaneously. Both of them perform restart of the application's process,
so they could interfere with each other leading to unexpected behaviour. For example, the application could be respawned into multiple instances, or fall into uncontrolled loop of restarting.

On the other hand, when your application is running in multithreaded mode, you can still use Internal Thread Recovery
(which allows to restart crashed threads, not the application's process) together with Process Monitor.
