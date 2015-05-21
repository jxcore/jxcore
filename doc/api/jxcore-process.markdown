# Process

This API is a collection of JXcore methods and properties for accessing from both main instance and a sub-instance.

All of the methods described here are accessible from global `process` object, for example:

```js
process.keepAlive();
```

## Event: 'restart'

Attaching any callback to `restart` event, depending on the context in which it is performed, enables internal process or sub-instance recovery.

Internal Recovery is a separate section described [here](jxcore-feature-internal-recovery.markdown).

## process.isEmbedded

This property returns `true`, if the binary is a static/shared library. This case applies e.g. for mobile deployments (android, iOS).

## process.isPackaged

> This property formerly was known as `process.IsEmbedded` and received the current name during JXcore v Beta-0.3.0.0 development.

This property returns `true`, if the current code block runs under native package (a package compiled with [-native](jxcore-feature-packaging-code-protection.markdown#-native) switch).

For example, let's have the following *index.js*:
```js
console.log("isPackaged", process.isPackaged);
```

We can compile it now with `-native` switch:

    > jx package index.js app -native

Now when we run it, it displays `true`:

    > ./app
    true

## process.keepAlive(timeout)

* `timeout` {Number}

> This method is implemented only in the sub-instance context. When called from a main instance it does nothing.

Marks the main process to be alive. This will inform the thread pool,
that the task has an intention to continue working (probably by doing some delayed or async work).

Normally the thread pool knows, that the task is working only until it returns from the task method.
But if you would use in the task code e.g. delayed execution with `setTimeout()` or `setInterval()`, or any async call,
the task method may return faster, before those delayed/async jobs will have chance to complete.
This is a moment, when `keepAlive()` comes in handy. Later in the code, you can call `release()` to signal the main instance, that the task has completed.

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

The `keepAlive()` method even if is callable only from a sub-instance, does not really keep it alive, but the main instance.

Internally, it increments a counter. On the other hand, every `process.release()` invocation - decrements it.
So when you want to end the application - you should call the `release()` method the same amount of times as you have called `keepAlive()`.
When that counter is zero - the main process may exit now.

## process.release()

> This method is implemented only in the sub-instance context. When called from a main instance it does nothing.

Unmarks the current task (from which the method was called) from being alive. You can use this method to inform the thread pool, that the task finished its work.
If you will do this for all of the tasks, the main instance could freely exit then, and the application may close itself naturally with exit code = 0.

Please refer to `process.keepAlive()` method for full example.

```js
process.release();
```

## process.sendToMain(param)

* `param` {Object}

> This method is implemented only in the sub-instance context. When called from a main instance it does nothing.

Sends a message to the main instance, and there it can be received by attaching to [`message`](jxcore-tasks.markdown#event-message) event.
The `param` can be any value, for example string or json literal object.

```js
process.sendToMain( { obj: "something" } );

// now the main instance can receive the message like this:
jxcore.tasks.on('message', function (threadId, param) {
   console.log('Main instance received a message from sub-instance no ' + threadId +
      '. Message: ', param);
});
```

## process.sendToThread(threadId, param)

* `threadId` {Number}
* `param` {Object}

Sends a message to the specific instance identified by `threadId`.
The `param` can be any value, such as a string or json literal object.


## process.sendToThreads(param)

* `param` {Object}

Similar to `sendToMain()`, except that it sends a message to all of the sub-instances from the thread pool.

## process.subThread

This property returns `true`, if the current code block runs under the sub-instance, or `false` otherwise.

```js
if (process.subThread) {
   console.log("we are in a sub-instance.");
}
```

## process.threadId

Returns the ID of the sub-instance. For multi-instanced application it is a number between 0 and 62 (because the maximum amount of sub-instances is 63).
You can also control the number of sub-instances for your application, see here for more information: [Defaults](jxcore-feature-multitasking.markdown#defaults).

Since one of the sub-instances can have its index equal to 0, we should not test it with:

```
if (process.threadId) {
    // do something
}
```

For a main instance, `threadId` always returns -1.

## process.unloadThread()

> This method is implemented only in the sub-instance context. When called from a main instance it does nothing.

This method marks the sub-instance to be removed from the thread pool and eventually removes it.
The removal itself will happen as soon as the sub-instance finishes execution of its latest task.

You may want to use this method to release sub-instance’s resources, if you don’t plan to use it soon.
Adding any tasks later will create a fresh sub-instance automatically on demand.
