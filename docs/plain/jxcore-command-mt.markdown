
# Multithreading from the command line

Generally, there are two ways of executing your JavaScript code in multiple threads with JXcore.
You can read about both of them here: [How to run multithreaded code?](jxcore-feature-multithreading.markdown#how-to-run-multithreaded-code),
but right now we will focus only on running multithreaded code from the command line.

## Commands

### mt[:number]

    > jx mt file.js

or

    > jx mt file.jx

Runs the code in multiple threads. Each thread executes the code independently.

JXcore by default will create 2 threads in the pool for that application, however you can change this value by supplying desired `number` of threads after the colon, like:

    > jx mt:4 file.js

It this case four threads will be created.

Using the `mt` command suits the best, when your code does not execute any delayed or async jobs,
otherwise the application may exit, before those jobs will have chance to complete.

Let's consider the following example (*test.js*):

```js
setTimeout(function() {
    console.log("I'm here after 20 secs.");
}, 20000);

console.log("I'm here immediately.");
```

When we'll run it with:

    > jx mt test.js

the process will not last for 20 seconds, because it will end after the last line of the code executes.

To keep the process alive and waiting for any delayed tasks, you have two options:

1) call [`process.keepAlive()`](jxcore-process.markdown#jxcore-process-process-keepalive-timeout) in the code above (but then at some point also [`process.release()`](jxcore-process.markdown#jxcore-process-process-release) if you want to release the application's process), like:

```js
process.keepAlive();

setTimeout(function() {
    console.log("I'm here after 20 secs.");
    process.release();
}, 20000);

console.log("I'm here immediately.");
```

2) or run the code with `mt-keep` command.

### mt-keep[:number]

    > jx mt-keep file.js

or

    > jx mt-keep:7 file.js

Does exactly the same thing as `mt` command, except that internally calls [`process.keepAlive()`](jxcore-process.markdown#jxcore-process-process-keepalive-timeout) for each of the threads.
It means, that the whole application won't exit, unless for each of those threads you won't call  [`process.release()`](jxcore-process.markdown#jxcore-process-process-release).

```js
var delay = 1000 * (process.threadId + 1 );

setTimeout(function () {
    console.log("I'm here after " + (delay / 1000) +
        " secs. ThreadId: ", process.threadId);
    process.release();
}, delay);

console.log("I'm here immediately. ThreadID: " + process.threadId);
```

When we'll run it with:

    > jx mt-keep:2 file.js

    // output:
    // I'm here immediately. ThreadID: 0
    // I'm here immediately. ThreadID: 1
    // I'm here after 1 secs. ThreadID: 0
    // I'm here after 2 secs. ThreadID: 1

As you can see, we have released each thread individually (at different delay), and when the last thread becomes released, the main application's thread exits.
