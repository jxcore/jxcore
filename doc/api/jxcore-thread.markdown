# Process

This API is a collection of JXcore methods and properties for accessing from both main thread and a subthread.

All of the methods described here are accessible from global `process` object, for example:

```js
process.keepAlive();
```

## Event: 'restart'

Attaching any callback to `restart` event, depending on the context in which it is performed, enables internal process or thread recovery.

Internal Recovery is a separate section described [here](jxcore-feature-internal-recovery.html).

## process.keepAlive(timeout)

* `timeout` {Number}

> This method is implemented only in the subthread context. When called from a main thread it does nothing.

Marks the main process to be alive. This will inform the thread pool,
that the task has an intention to continue working (probably by doing some delayed or async work).

Normally the thread pool knows, that the task is working only until it returns from the task method.
But if you would use in the task code e.g. delayed execution with `setTimeout()` or `setInterval()`, or any async call,
the task method may return faster, before those delayed/async jobs will have chance to complete.
This is a moment, when `keepAlive()` comes in handy. Later in the code, you can call `release()` to signal the main thread, that the task has completed.

The `timeout` parameter specifies, how many milliseconds the task should be kept alive, before automatic release will happen.
This parameter is optional, and if you don’t provide it, task will be alive forever, or until `process.release()` will be called.

```js
var method = function (obj) {

    setTimeout(function () {
       console.log("I'm here after 2000 ms. Can close now");
       process.release();
    }, 2000);

    console.log("waiting for 2000 ms");

    // without the line below, current task would return immediately
    // and the method delayed with setTimeout() would not have chance to complete.
    // try to comment the line below to test the behaviour, and you will see,
    // that the application closes itself, since the task is not kept alive.
    process.keepAlive();
};

jxcore.tasks.addTask(method);
```

The `keepAlive()` method even if is callable only from a subthread, does not really keep the subthread alive, but the main thread.

Internally, it increments a counter. On the other hand, every `process.release()` invocation - decrements it.
So when you want to end the application - you should call the `release()` method the same amount of times as you have called `keepAlive()`.
When that counter is zero - the main process may exit now.

## process.release()

> This method is implemented only in the subthread context. When called from a main thread it does nothing.

Unmarks the current task (from which the method was called) from being alive. You can use this method to inform the thread pool, that the task finished its work.
If you will do this for all of the tasks, the main thread could freely exit then, and the application may close itself naturally with exit code = 0.

Please refer to `process.keepAlive()` method for full example.

```js
process.release();
```

## process.sendToMain(param)

* `param` {Object}

> This method is implemented only in the subthread context. When called from a main thread it does nothing.

Sends a message to the main thread, and there it can be received by attaching to [`message`](jxcore-tasks.html#jxcore_tasks_event_message) event.
The `param` can be any value, for example string or json literal object.

```js
process.sendToMain( { obj: "something" } );

// now the main thread can receive the message like this:
jxcore.tasks.on('message', function (threadId, param) {
   console.log('Main thread received a message from subthread no ' + threadId +
      '. Message: ', param);
});
```

## process.sendToThread(threadId, param)

* `threadId` {Number}
* `param` {Object}

Sends a message to the specific thread identified by `threadId`.
The `param` can be any value, such as a string or json literal object.


## process.sendToThreads(param)

* `param` {Object}

Similar to `sendToMain()`, except that it sends a message to all of the threads from the thread pool.

## process.subThread

This property returns `true`, if the current code block runs under the subthread, or `false` otherwise.

```js
if (process.subThread) {
   console.log("we are in a subthread.");
}
```

## process.threadId

Returns the ID of the subthread. For multithreaded application it is a number between 0 and 62 (because the maximum amount of subthreads is 63).
You can also control the number of subthreads for your application, see here for more information: [Defaults](jxcore-feature-multithreading.html#defaults).

Since one of the subthreads can have its index equal to 0, we should not test it with:

```
if (process.threadId) {
    // do something
}
```

For a main thread, `threadId` always returns -1.

## process.unloadThread()

> This method is implemented only in the subthread context. When called from a main thread it does nothing.

This method marks the subthread to be removed from the thread pool and eventually removes it.
The removal itself will happen as soon as the subthread finishes execution of its latest task.

You may want to use this method to release subthread’s resources, if you don’t plan to use it soon.
Adding any tasks later will create a fresh subthread automatically on demand.
