# Multi-instanced Javascript Tasks

JXcore, in addition to running multi-instanced applications with `mt`/`mt-keep` command, offers Multi-instanced Javascript Tasks feature,
which allows you from the code running in the main instance to add tasks for execution in a sub-instances.

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

Note that this particular example does nothing but running hello.js file in multiple instances. The same result can be obtained by using mt:keep option as follows:

    > jx mt:keep hello.js

We have written a blog post showing How to turn your existing application into a multi-instanced one with a few lines of code.

See also general description of JXcore [multitasking](jxcore-feature-multitasking.markdown).

# Passing arguments to tasks

By default, when you call a task with a `param` argument, it is internally processed with `JSON.stringify()`, sent to the task and then parsed back (`JSON.parse()`).
For example:

```js
var method = function (param) {
  console.log("Type of param: '" + (typeof param) + "'. Value:", param);
};

var param = {str: "hello"};
jxcore.tasks.addTask(method, param);
```

Displays the following output:

> Type of param: 'object'. Value: { str: 'hello' }

However there are some scenarios, when user already have a stringified value and wants to pass it 'as-is', while having it parsed in the task's method.

In that case, the above code would behave as follows: `addTask()` would internally stringify the argument again to parse it back, but at this point it would still be a string, instead of parsed object:

```js
var param = JSON.stringify({str: "hello"});
jxcore.tasks.addTask(method, param);
```

> Type of param: 'string'. Value: '{"str":"hello"}'

Note the `string` type of the `param` received by a task.

To avoid this, there are few extra methods available:

* jxcore.tasks._addTask()
* jxcore.tasks._runOnce()
* jxcore.tasks._runOnThread()
* method._runTask()
* method._runOnce()

By using them user avoids default `JSON.stringify()` invocation and receives parsed object into the task:

```js
var param = JSON.stringify({str: "hello"});
jxcore.tasks._addTask(method, param);
```

> Type of param: 'object'. Value: { str: 'hello' }

Apart from this behavior the methods mentioned above work exactly the same as their non-underscored equivalents.
For their full specification please refer below.

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
Refer to [`process.keepAlive()`](jxcore-process.markdown#processkeepalivetimeout) method for that matter.

## Event: 'message'

* `threadId` {Number}
* `param` {Object}

This event fires every time, when the main process receives a message sent by a sub-instance with `process.sendToMain()` method.

```js
var callback = function (threadId, params) {
   console.log('Main instance received a message from sub-instance no ' + threadId
      + '. Message: ', params);
};

// this event fires on the main instance, after it receives a message from a sub-instance
jxcore.tasks.on('message', callback);

// a sub-instance sends the message with sendToMain().
// see the API for more information
process.sendToMain( { obj: "something" } );
```

## tasks.addTask(method, param, callback)

* `method` {Function}
* `param` {Object} [optional]
* `callback` {Function} [optional]

Adds new task **as a method** to be processed in a separate instance. The task will be processed as soon as possible.
If there is any idle sub-instance, it will be used to execute the task immediately. Otherwise it may wait until the other tasks will finish or some of the sub-instances will become idle.

The task is the function `method` with optional `param` as an argument. After the method completes, the `callback` will be invoked with arguments `(err, result)` where `result` is the result of the task `method`.

There are few principles regarding adding the tasks, that you should be aware of:

1. The sub-instance that will run the task is not specified and will be chosen automatically. You can find out which sub-instance is used for that task only from inside the task itself (see `process.threadId` property). Alternatively, `tasks.runOnThread()` can be used to specify the sub-instance to run on.
2. If for example on your system `getThreadCount()` returns 3, and you add 3 tasks one after another, there is no guarantee, that each of them will run in its own sub-instance, but still it is one of the possibilities. But things can go also in many other ways: all of the 3 tasks can run in the same sub-instance, or two of them in one, and the third in another, etc.
3. Do not assume the tasks will be executed in the order of which they were added.
4. When the task `method` returns from its execution block (for example by calling return statement),
it is considered to be completed. 
If the task performs any delayed execution with `setTimeout()`, `setInterval()` or other asynchronous calls,
the task method may return before those delayed/async jobs will have had a chance to complete.
For gaining control over task’s execution time, please use `process.keepAlive()` and `process.release()` methods from [process](jxcore-process.markdown)  API.

Adding a task with callback:

```js
var method = function (param) {
   console.log("We are in a sub-instance now. Argument value is:", param);
   return "ok";
};

jxcore.tasks.addTask(method, { str: "hello" }, function (err, result) {
   if(err) throw err;
   console.log("This is result from method running in sub-instance:", result);
   // should display "ok"
});
```

There is also an alternative way to run a function as a task. See below for `method.runTask()`.

### method.runTask(param, callback)

* `param` {Object}
* `callback` {Function} [optional]

This is shorter alternative to `tasks.addTask()`.
JXcore adds `runTask()` method to function's prototype, so each function can be added as a task directly by calling `method.runTask()`:

```js
var method = function (param) {
    console.log("We are in a sub-instance now. Argument value is:", param);
    return "ok";
};

var callback = function (err, result) {
    console.log(result);
};

method.runTask({ str: "hello" }, callback);
```

## tasks.addTask(object, param, callback)

* `object` {Function} - this is the object containing `define()` and/or `logic()` methods, which will be executed in a sub-instance.
* `param` {Object} - argument for `logic()` method
* `callback` {Function} [optional]

Adds new task **as an object** to be processed in a separate sub-instance. The task will be processed as soon as possible.
If there is any idle sub-instance, it will be used to execute the task immediately. Otherwise it may wait until the other tasks will finish or some of the sub-instances will become idle.

After the task completes, the `callback` will be invoked with arguments `(err, result)` where `result` is the result of the task's `logic()` method.

There is one important thing to be noted here: the `callback` is invoked only when there is no `waitLogic` defined or when it evaluates to `false` (see `waitLogic` description below).

As you already might know from previous part of the documentation, the sub-instances are separated from the main instance as well as from each other. Moreover, tasks themselves are also separated from each other, even if they are running in the same sub-instance.

There are however some scenarios, where it could be useful to share some data between the tasks being executed in the same sub-instance.

For this case we implemented *define & logic* approach.

```js
var task = {};
task.define = function() {
    // you may define here static variables, accessible to all tasks on this sub-instance
    var x = 0;
};
task.logic = function(param) {
    // this method runs once per every task in the sub-instance
    // variables declared in define() method are freely accessible from inside logic().
    // you can also modify them
    x = 10;
};
```

### define()

  * This method runs only once per whole sub-instance (at first `addTask()` invocation.)
  * Purpose of this method is to declare variables, initialize objects, make some e.g. database connections, etc. All of them will be static and accessible to all tasks, which means any subsequent `logic()` invocations of this particular task object, running on this sub-instance.
  * This is a required method which neither takes any parameters nor returns any values. Any parameters will be ignored.

### logic(param)

  * This is the actual task’s execution code.
  * It may receive one argument and this can be any javascript object.
  * Variables and objects declared in `define()` method are freely accessible from inside `logic()` method. You can also freely modify them, but please remember that since everything declared in `define()` is static, if you change some variable’s value in one task, it will have affect for all subsequent tasks which run in this sub-instance. Also do not assume the tasks will be executed in the order of which they were added.
  * This method is optional which means that the entire task’s job should be embedded inside the `define()` method. As mentioned before, it will run only once per sub-instance, no matter how many times the task was added to the queue.

### waitLogic `boolean`

  * When this parameter is provided and is set to `true`, the `logic()` does not get executed immediately after `define()` completes.
  Instead, it waits, until you explicitly call `continueLogic()` from inside `define()`. If you will not do it, the `logic()` will never get called.
  This mechanism is intended to be used in scenarios, when `define()` runs async work, and `logic()` should be run afterwards.
  Whenever `waitLogic` is used (set to `true`), the `callback` is not called after the task completes.
  This is mostly due to historical reasons and might be changed in future.

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

Example with `waitLogic` usage:

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
    jxcore.tasks.addTask(task, o + " Hello", function (err, res) {
        console.log("END", res);
    });
}
```

Below is a sample usage of `addTask` with `callback`:

```js
var task = {
  define: function () {
  },
  logic: function (param) {
    console.log("We are in a sub-instance now. Argument value is:", param);
    return "ok";
  }
};

var callback = function (err, result) {
  if(err) throw err;
  console.log(result);
};

jxcore.tasks.addTask(task, {str: "hello"}, callback);
```

## tasks.forceGC()

Forces garbage collection on V8 heap. Please use it with caution. It may trigger the garbage collection process immediately, which may freeze the application for a while and stop taking the requests during this time.

## tasks.getThreadCount()

Returns the number of sub-instances currently used by application (size of the thread pool).

## tasks.jobCount()

Returns number of tasks currently waiting in the queue.

## tasks.killThread(threadId)

Kills a sub-instance with given ID. This can be used for controlling the execution time of the task.

In example below, when the task starts it notifies the main instance about that fact.
From that moment the main instance may start counting the timeout for killing the sub-instance.

```js
// main instance receives the message from a task
jxcore.tasks.on("message", function(threadId, obj){
    if(obj.started){
        //kill the task after a second
        setTimeout(function(){
            jxcore.tasks.killThread(threadId);
            console.log("sub-instance killed", threadId);
        },1000);
    }
});

// adding a task
jxcore.tasks.addTask( function() {
    // informing the main instance, that task is just started
    process.sendToMain({started:true});

    // looping forever
    while(true){};
    console.log("this line will never happen.");
});
```

## tasks.register(method)

* `method` {Function}

You may define a sub-instance initialization task by using `runOnce`, it will run the given task on all the current and future sub-instances.
However, if you don’t know if the solution will use the multi-tasking but still you need to make sure the initializer method is set,
you can use register for this purpose.

For example:

```js
jxcore.tasks.register(function () {
  cwdToLower = function () {
    return process.cwd().toLowerCase();
  };
});

jxcore.tasks.addTask(function () {
  console.log(cwdToLower());
});
```

It is the same as we would use the `global` object explicitly:

```js
global.cwdToLower = function () {
  ...
};
```

In the same manner you can define other variables or constant-like fields:

```js
jxcore.tasks.register(function () {
  global.myVar = "something"
});

jxcore.tasks.runOnce(function () {
  console.log("myVar", myVar, "threadId", process.threadId);
});
```

The above example runs the task for all available sub-instances, showing that `myVar` is available for all of them.


## tasks.runOnce(method, param, doNotRemember)

* `method` {Function} - This is the method, which will be executed once for every existing sub-instance (<em>getThreadCount()</em> times).
* `param` {Object} - Argument for that method.
* `doNotRemember` {Boolean}

Adds new task (the `method` function with optional `param` argument) to be processed just once for every existing sub-instance.
In other words, every sub-instance will call the task once.
You can get number of sub-instances by calling `getThreadCount()` method.
Every sub-instance will process its task as soon as possible.
If there are no other tasks in the queue, the task will be executed immediately.
Otherwise it may wait until the other tasks in this sub-instance will finish.

If you don't want the method definition to be remembered by future sub-instances – use `doNotRemember`. It's `false` by default.

Running tasks with `runOnce()` method will also get `emptyQueue` event fired.

One of the cases for using `runOnce()` could be setting up a http server on each sub-instance.

```js
jxcore.tasks.runOnce(method, "some parameter");
```

## tasks.runOnThread(threadId, method, param, callback)

* `threadId` {Number}
* `method` {Function}
* `param` {Object} [optional]
* `callback` {Function} [optional]

Runs a task (`method` with `param` argument) on individual sub-instance identified as `threadId`.

After the method completes, the `callback` will be invoked with arguments `(err, result)` where `result` is the result of the task `method`.

*Remind that, in case you provide the task in 'define/logic' form, runOnThread runs both of them every time.
If this is not convenient for your scenario, you may check whether the define has been called on that particular instance by marking a global member.*

### method.runOnce(param, doNotRemember)

* `param` {Object} - Argument for that method.
* `doNotRemember` {Boolean}

This is shorter alternative to `tasks.runOnce()`.
JXcore adds `runOnce()` method to function's prototype, so each function can be added as a task directly by calling `method.runOnce()`:

```js
var method = function (param) {
    console.log("Sub-instance no " + process.threadId + ". Argument value is:", param);
};

method.runOnce("hello");
```

## tasks.setThreadCount(value)

* `value` {Number}

Sets the number of sub-instances that you want to have in the thread pool for the application.

Generally, there is no need to use this method as JXcore by default will create 2 sub-instances. In some scenarios you may want to change this number, but if you do, you must call `tasks.setThreadCount()` before the first use of `jxcore.tasks` object. After that, any subsequent calls of this method will be simply ignored.

The minimum, and default value is 2. The maximum value is 63, but keep in mind, that this amount of sub-instances may not always bring performance benefits. In the contrary – in some cases may do even worse, depending on the task implementation, so make sure that you always do some proper testing. Setting value outside this range will raise an exception.

```js
jxcore.tasks.setThreadCount(5);
var tasks = jxcore.tasks;

// the call below will be ignored, since we have just referenced jxcore.tasks object
// and the thread pool is already created.
jxcore.tasks.setThreadCount(10)
```

## tasks.unloadThreads()

Marks all sub-instances to be removed from the thread pool. If they are idle – the method removes them immediately.
Otherwise it waits, until they finish their last tasks and removes them afterwards.
