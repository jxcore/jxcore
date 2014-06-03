# Multithreaded Javascript Tasks

JXcore, in addition to running multithreaded applications with `mt`/`mt-keep` command, offers Multithreaded Javascript Tasks feature,
which allows you from the code running in the main thread to add tasks for execution in a subthread.

The main idea in this approach is to define the tasks, which you will then add to the thread pool for execution.
For this, you can use any of the methods described in the API section.

The task can do anything. See the simplest example:

```js
var method = function () {
    console.log("this is message from thread no", process.threadId);
};

jxcore.tasks.addTask(method);
```

Another one:

```js
var method = {};
method.define = function () {
    require("./hello");
    console.log("http server started on thread", process.threadId);
    process.keepAlive();
};

jxcore.tasks.runOnce(method);
```

Note that this particular example does nothing but running hello.js file in multiple threads. The same result can be obtained by using mt:keep option as follows:

    > jx mt:keep hello.js

We have written a blog post showing How to turn your existing application into a multi-threaded one with a few lines of code.

See also general description of JXcore [multithreading](jxcore-feature-multithreading.markdown).

# API

## Event: 'emptyQueue'

This event fires when all of the added tasks have been processed.

```js
// this event fires when all of the added tasks have been processed
jxcore.tasks.on('emptyQueue', function () {
   console.log('all of the added tasks have been processed');
});
```

Please keep in mind, that if you plan to use some delayed/async work inside a task,
the `emptyQueue` event can be fired before they will have chance to complete.
Refer to [`process.keepAlive()`](jxcore-thread.markdown#process-keepalive-timeout) method for that matter.

## Event: 'message'

* `threadId` {Number}
* `param` {Object}

This event fires every time, when the main process receives a message sent by a subthread with `process.sendToMain()` method.

```js
var callback = function (threadId, params) {
   console.log('Main thread received a message from subthread no ' + threadId
      + '. Message: ', params);
};

// this event fires on the main thread, after it receives a message from a subthread
jxcore.tasks.on('message', callback);

// a subthread sends the message with sendToMain().
// see the Subthreads API for more information
process.sendToMain( { obj: "something" } );
```

## tasks.addTask(method, param, callback, obj)

* `method` {Function}
* `param` {Object}
* `callback` {Function}
* `obj` {Object}

Adds new task **as a method** to be processed in a separate thread. The task will be processed as soon as possible.
If there is any idle subthread, it will be used to execute the task immediately. Otherwise it may wait until the other tasks will finish or some of the subthreads will become idle.

The task is the function `method` with optional `param` as an argument. After the method completes, the `callback` will be invoked.
It can receive one or twe arguments, depending if `obj` is provided or not.

`obj` is a context object from the main thread and it can contain any value.
If it’s not provided, the `callback` method will have only one argument, and it will be the result of the task `method`.
Otherwise, the `callback` will contain two arguments.
The first one is the `obj` object described here, while the second argument is the result of the task `method`.

There are few principles regarding adding the tasks, that you should be aware of:

1. You cannot specify, in which of the subthreads your task will run. The subthread will be chosen automatically. You can find out, which subthread is used for that task only from inside the task itself (see `threadId` property in Subthreads API).
2. If for example on your system `getThreadCount()` returns 3, and you add 3 tasks one after another, there is no guarantee, that each of them will run in its own subthread, but still it is one of the possibilities. But things can go also in many other ways: all of the 3 tasks can run in the same subthread, or two of them in one subthread, and the third in another, etc.
3. Do not assume the tasks will be executed in the order of which they were added.
4. When the task `method` returns from its execution block (for example by calling return statement), it is considered as completed. This means, that if you use in the task code any delayed execution with `setTimeout()` or `setInterval()`, or async calls, the task method may return faster, before those delayed/async jobs will have chance to complete. For gaining control over task’s execution time, please use `process.keepAlive()` and `process.release()` methods from Subthreads API.

Adding a task with one-argument callback:

```js
var method = function (param) {
   console.log("We are in a subthread now. Argument value is:", param);
   return "ok";
};

jxcore.tasks.addTask(method, { str: "hello" }, function (result) {
   console.log("This is result from subthreaded method:", result);
   // should display "ok"
});
```

Adding a task with two-argument callback:

```js
// we're going to pass some object from the main thread to the callback
var mainThreadVariable = {
    log : function(txt) {
        console.log(txt);
    }
};

var callback = function (obj, result) {
    // now we can access mainThreadVariable (by obj argument)
    obj.log(result);
};

jxcore.tasks.addTask(method, { str: "hello" }, callback, mainThreadVariable);
```

There is also an alternative way to run a function as a task. See below for `method.runTask()`.

### method.runTask(param, callback, obj)

* `param` {Object}
* `callback` {Function}
* `obj` {Object}

This is shorter alternative to `tasks.addTask()`.
JXcore adds `runTask()` method to function's prototype, so each function can be added as a task directly by calling `method.runTask()`:

```js
var method = function (param) {
    console.log("We are in a subthread now. Argument value is:", param);
    return "ok";
};

var mainThreadVariable = {
    log : function(txt) {
        console.log(txt);
    }
};

var callback = function (obj, result) {
    // now we can access mainThreadVariable (by obj argument)
    obj.log(result);
};

method.runTask({ str: "hello" }, callback, mainThreadVariable);
```

## tasks.addTask(object, param)

* `object` {Function} - this is the object containing `define()` and/or `logic()` methods, which will be executed in a subthread.
* `param` {Object} - argument for `logic()` method

Adds new task **as an object** to be processed in a separate thread. The task will be processed as soon as possible.
If there is any idle subthread, it will be used to execute the task immediately. Otherwise it may wait until the other tasks will finish or some of the subthreads will become idle.

As you already might know from previous part of the documentation, the subthreads are separated from the main thread as well as from each other. Moreover, tasks themselves are also separated from each other, even if they are running in the same subthread.

There are however some scenarios, where it could be useful to share some data between the tasks being executed in the same thread

For this case we implemented *define & logic* approach.

```js
var task = {};
task.define = function() {
    // you may define here static variables, accessible to all tasks on this subthread
    var x = 0;
};
task.logic = function(param) {
    // this method runs once per every task in the subthread
    // variables declared in define() method are freely accessible from inside logic().
    // you can also modify them
    x = 10;
};
```

**define()**

  * This method runs only once per whole subthread (at first `addTask()` invocation.)
  * Purpose of this method is to declare variables, initialize objects, make some e.g. database connections, etc. All of them will be static and accessible to all tasks, which means any subsequent `logic()` invocations of this particular task object, running on this subthread.
  * This is a required method which neither takes any parameters nor returns any values. Any parameters will be ignored.

**logic(param)**

  * This is the actual task’s execution code.
  * It may receive one argument and this can be any javascript object.
  * Variables and objects declared in `define()` method are freely accessible from inside `logic()` method. You can also freely modify them, but please remember that since everything declared in `define()` is static, if you change some variable’s value in one task, it will have affect for all subsequent tasks which run in this subthread. Also do not assume the tasks will be executed in the order of which they were added.
  * This method is optional which means that the entire task’s job should be embedded inside the `define()` method. As mentioned before, it will run only once per subthread, no matter how many times the task was added to the subthread’s queue.

**waitLogic** `boolean`

  * When this parameter is provided and is set to `true`, the `logic()` does not get executed immediately after `define()` completes.
  Instead, it waits, until you explicitly call `continueLogic()` from inside `define()`. If you will not do it, the `logic()` will never get called.
  This mechanism is intended to be used in scenarios, when `define()` runs async work, and `logic()` should be run afterwards.

When adding a tasks with *define & logic approach*, make sure to apply all of the principles for adding the tasks as methods – see `addTask(method, param, callback, obj)`.

Keep in mind that only those two members of the task object (`define()` and `logic()` methods) are meaningful. Any other object members will be ignored. Moreover, we highly discourage adding any other members, especially if you plan to add massive amount of such tasks, because they can unnecessarily raise memory usage and may impact the performance.

```js
// in this example we are adding two identical tasks.
// the define() method will get called only once,
// but the logic() will be executed twice, as separate tasks.

var task = {};

task.define = function () {
    // static variables
    var x = 0;
    var bool = true;
};

task.logic = function (param) {
    console.log("Hello in ", param);
    // variables declared in define() method are freely accessible from inside logic().
    // you can also modify them, but be careful: do not rely on order of the tasks.
    // task1 doesn't have to be executed before task2!
    bool = !bool;
    console.log("Static variable bool:", bool);
    console.log("Static variable x: ", x++);
};

jxcore.tasks.addTask(task, "task1");
jxcore.tasks.addTask(task, "task2");
```

Another example with `waitLogic`:

```js
var task = {
    define: function () {
        setTimeout(function () {
            console.log("called")
            continueLogic();
        }, 2000);
    },
    logic: function (obj) {
        console.log(obj);
        return obj + "X";
    },
    waitLogic: true
};

for (var o = 0; o < 2; o++) {
    jxcore.tasks.addTask(task, o + " Hello", function (res) {
        console.log("END", res);
    });
}
```

## tasks.forceGC()

Forces garbage collection on V8 heap. Please use it with caution. It may trigger the garbage collection process immediately, which may freeze the application for a while and stop taking the requests during this time.

## tasks.getThreadCount()

Returns the number of subthreads currently used by application (size of the thread pool).

## tasks.jobCount()

Returns number of tasks currently waiting in the queue.

## tasks.killThread(threadId)

Kills a thread with given thread ID. This can be used for controlling the execution time of the task.

In example below, when the task starts it notifies the main thread about that fact.
From that moment the main thread may start counting the timeout for killing the thread.

```js
// main thread receives the message from a task
jxcore.tasks.on("message", function(threadId, obj){
    if(obj.started){
        //kill the task after a second
        setTimeout(function(){
            jxcore.tasks.killThread(threadId);
            console.log("thread killed", threadId);
        },1000);
    }
});

// adding a task
jxcore.tasks.addTask( function() {
    // informing the main thread, that task is just started
    process.sendToMain({started:true});

    // looping forever
    while(true){};
    console.log("this line will never happen.");
});
```

## tasks.runOnce(method, param, doNotRemember)

* `method` {Function} - This is the method, which will be executed once for every existing subthread (<em>getThreadCount()</em> times).
* `param` {Object} - Argument for that method.
* `doNotRemember` {Boolean}

Adds new task (the `method` function with optional `param` argument) to be processed just once for every existing subthread.
In other words, every subthread will call the task once.
You can get number of subthreads by calling `getThreadCount()` method.
Every subthread will process its task as soon as possible.
If there are no other tasks in the subthread’s queue, the task will be executed immediately.
Otherwise it may wait until the other tasks in this subthread will finish.

If you don't want the method definition to be remembered by future threads – use `doNotRemember`. It's `false` by default.

Running tasks with `runOnce()` method will also get `emptyQueue` event fired.

One of the cases for using `runOnce()` could be setting up a http server on each thread.

```js
jxcore.tasks.runOnce(method, "some parameter");
```

## tasks.runOnThread(threadId, method, param, callback, obj)

* `threadId` {Number}
* `method` {Function}
* `param` {Object}
* `callback` {Function}
* `obj` {Object}

Runs a task (`method` with `param` argument) on individual subthread no `threadId`.

After the method completes, the `callback` will be invoked.
It can receive one or two arguments, depending on whether or not `obj` is provided.

`obj` is a context object from the main thread and it can contain any value.
If it’s not provided, the `callback` method will have only one argument, and it will be the result of the task `method`.
Otherwise, the `callback` will contain two arguments.
The first one is the `obj` object described here, while the second argument is the result of the task `method`.


### method.runOnce(param, doNotRemember)

* `param` {Object} - Argument for that method.
* `doNotRemember` {Boolean}

This is shorter alternative to `tasks.runOnce()`.
JXcore adds `runOnce()` method to function's prototype, so each function can be added as a task directly by calling `method.runOnce()`:

```js
var method = function (param) {
    console.log("Subthread no " + process.threadId + ". Argument value is:", param);
};

method.runOnce("hello");
```

## tasks.setThreadCount(value)

* `value` {Number}

Sets the number of subthreads that you want to have in the thread pool for the application.

Generally, there is no need to use this method as JXcore by default will create 2 subthreads. In some scenarios you may want to change this number, but if you do, you must call `tasks.setThreadCount()` before the first use of `jxcore.tasks` object. After that, any subsequent calls of this method will be simply ignored.

The minimum, and default value is 2. The maximum value is 63, but keep in mind, that this amount of subthreads may not always bring performance benefits. In the contrary – in some cases may do even worse, depending on the task implementation, so make sure that you always do some proper testing. Setting value outside this range will raise an exception.

```js
jxcore.tasks.setThreadCount(5);
var tasks = jxcore.tasks;

// the call below will be ignored, since we have just referenced jxcore.tasks object
// and the thread pool is already created.
jxcore.tasks.setThreadCount(10)
```

## tasks.unloadThreads()

Marks all subthreads to be removed from the thread pool. If they are idle – the method removes them immediately.
Otherwise it waits, until they finish their last tasks and removes them afterwards.
