# Multitasking

Another important feature introduced in JXcore is multitasking.
In a single-threaded project `setTimeout()` and `setInterval()` methods wait in a separate instance,
but the scheduled methods are still executed under the main application instance.
Since the main instance eventually handles every single task, the application may lost its responsiveness under a load or queued heavy tasks.
Even if the application works well in a testing environment, it could start slowing down under a massive load (increased number of clients).

## Introduction

### Sub-instances are separated

Every instance in JXcore are called a sub-instance. The sub-instance is completely separated from the main instance and from the other sub-instances,
because they all run in different V8′s contexts/isolates. It means that they cannot share or have common objects, variables etc. directly.
Thus, there is no use for any global variable defined in a main instance – the sub-instance will not see it.

But there is another way – JXcore exposes special thread-safe [Memory Store](jxcore-store.html), which can be shared among instances. See `jxcore.store.shared` for more information.

Apart from that, there are also two other ways for sub-instances to communicate with the main instance:

1. sending a message – [`process.sendToMain()`](jxcore-process.html#jxcore_process_process_sendtomain_param),
2. returning the result from the task, since this is the regular method.

### Defaults

JXcore uses the following defaults for sub-instances for multitasked applications:

1. minimum amount is set to 2. This is also the default value.
2. maximum amount is set to 63.

You can also control the number of sub-instances and there are two ways you can do this, but that depends how you run the application.

If you launch te code with mt/mt-keep command line parameter, you can set the instances count by applying a number after the colon, like:

    > jx mt:4 hello.js

Otherwise you can use [`tasks.setThreadCount()`](jxcore-tasks.html#jxcore_tasks_tasks_setthreadcount_value) method.
See also [How to run multitasked code?](jxcore-feature-multitasking.html#jxcore_feature_multitasking_how_to_run_multitasked_code) for more information.

### Memory management

Since node is based on V8 engine, it also inherits it’s defaults and limitations.

Currently, the default V8 memory limit is 512 MB on 32-bit systems and 1 GB on 64-bit systems.
According to the documentation, this can be raised to a maximum of 1 GB and 1.7 GB respectively.

But one of the most important factors about running multi-instanced tasks in JXcore is the fact that the main instance
as well as every sub-instance uses its own V8 heap space, so the V8 memory limits applies to each of them separately!
For example, an application which runs on four instances can hold up to 5 x 1.7 GB of memory (1 for the main instance and 4 for the sub-instances)!

And when there are no currently active tasks, JXcore sub-instances force an automatic V8 heap cleanup – each of them separately on its own!

Moreover, the queue mechanism for the sub-instances uses separate memory blocks than V8 engine.
It means that it occupies a different place in the memory, hence the queues do not fall under the limitations mentioned above.

### Native C++ Node.JS modules

See [native](https://github.com/jxcore/jxcore/tree/master/doc/native).

## How to run multitasked code?

There are two ways of executing your JavaScript code in multiple tasks with JXcore.

### Multitasking from the command line

The first and the easiest one is just to use `mt` or `mt-keep` option in the command-line for jx:

    > jx mt-keep:4 easy1.js

This topic is fully described here: [mt / mt-keep](jxcore-command-mt.html)

### Multi-instanced Javascript Tasks

The second one is by using `jxcore.tasks` object. It requires you to implement all the multi-instanced logic by yourself.
Then you run the application without any jx’s options:

    > jx tasks_way.js

The main difference between those two methods is, that with mt/mt-keep approach you have to do absolutely nothing
to run the application multi-instanced. However, the same code is running separately for each instance
and you don’t really have any control over changing the instance’s job, once the application started.

With the second (the “tasks”) approach it’s all about adding jobs to the queue of the pool.
Tasks start and finish while you can add more tasks in the runtime.
In this model you always have the main instance and the sub-instances.
You can also get notified, when all task are completed.
See the API for reference.

This topic is fully described here: [Tasks](jxcore-tasks.html)
