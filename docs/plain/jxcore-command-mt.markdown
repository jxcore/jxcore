
# Multithreading from the command line

Generally, there are two ways of executing your JavaScript code in multiple threads with JXcore.
You can read about both of them here: [How to run multithreaded code?](http://jxcore.com/multithreaded-javascript-tasks/),
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

Using the `mt` command suits the best, if your code does not execute any delayed or async jobs, otherwise the application may exit, before those jobs will have chance to complete.

Let's consider the following example (*test.js*):

```js
setTimeout(function() {
    console.log("I'm here after 20 secs.");
}, 20000);

console.log("I'm here immediately.");
```

When we'll run it with:

    > jx mt test.js

the process will not last for 20 seconds, because it will end after the last line of the code executes. Too keep the process alive and waiting for any delayed tasks, you can use `mt-keep` command.

### mt-keep[:number]

    > jx mt-keep file.js

or

    > jx mt-keep:7 file.js

Does exactly the same thing as `mt` command, except that keeps each of the threads alive. It means, that the whole application won't exit, unless you call [`process.release()`](jxcore-process.html#jxcore_process_process_release) for each thread.

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

