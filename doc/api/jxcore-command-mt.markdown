
# Multitasking from the Command Line

Generally, there are two ways of executing your JavaScript code in multiple instances with JXcore.
You can read about both of them here: [How to run multitasked code?](jxcore-feature-multitasking.html#jxcore_feature_multitasking_how_to_run_multitasked_code),
but right now we will focus only on running multitasking code from the command line.

## Maximum number of instances

The maximum number of instances to run with `mt` or `mt-keep` command is 16.
This is different to maximum instances count value when set with [`tasks.setThreadCount()`](jxcore-tasks.html#jxcore_tasks_tasks_setthreadcount_value) for running `jxcore.tasks`.

## Commands

### mt[:number]

    > jx mt file.js

or

    > jx mt file.jx

Runs the code in multiple instances. Each instance executes the code independently.

JXcore by default will create 2 sub-instances in the pool for that application.
However you can change this value by supplying desired `number` of sub-instances after the colon, like:

    > jx mt:4 file.js

In this case, four sub-instances will be created.

Using the `mt` command works best when your code is supposed to perform some operations and exit (after the event loop becomes drained).

Let's consider the following example (*test.js*):

```js
var delay = 1000 * (process.threadId + 1 );

setTimeout(function () {
    console.log("I'm here after " + (delay / 1000) +
        " secs. ThreadId: ", process.threadId);
}, delay);

console.log("I'm here immediately. ThreadID: " + process.threadId);
```

Now, we'll run it with the following command:

    > jx mt test.js

The process will stay alive as long as all of the sub-instances last.
Since this sample runs on 2 sub-instances, one of them (`process.threadId` == 0) will occupy its own event loop for 1 second,
while the other one for 2 seconds (`process.threadId` == 1), and only after that time the application will exit.

### mt-keep[:number]

    > jx mt-keep file.js

or

    > jx mt-keep:7 file.js

`mt-keep` does exactly the same thing as `mt` command, except that it internally calls [`process.keepAlive()`](jxcore-process.html#jxcore_process_process_keepalive_timeout) for each of the sub-instances.
It means, that the whole application won't exit, unless for each of those sub-instances you call upon [`process.release()`](jxcore-process.html#jxcore_process_process_release).

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

As you can see, we have released each sub-instance individually (at different delays), and when the last one is released, the main application's sub-instance exits.
