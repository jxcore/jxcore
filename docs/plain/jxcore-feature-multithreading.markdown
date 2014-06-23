# Multithreading

Another important feature introduced in JXcore is multithreading.
In a single-threaded project `setTimeout()` and `setInterval()` methods wait in a separate thread,
but the scheduled methods are still executed under the main application thread.
Since the main thread eventually handles every single task, the application may lost its responsiveness under a load or queued heavy tasks.
Even if the application works well in a testing environment, it could start slowing down under a massive load (increased number of clients).

## Introduction

### Subthreads are separated

Every thread in JXcore are called a subthread. The subthread is completely separated from the main thread and from the other subthreads,
because they all run in different V8′s contexts/isolates. It means that they cannot share or have common objects, variables etc. directly.
Thus, there is no use for any global variable defined in a main thread – the subthread will not see it.

But there is another way – JXcore exposes special thread-safe [Memory Store](jxcore-store.markdown), which can be shared among threads. See `jxcore.store.shared` for more information.

Apart from that, there are also two other ways for subthread to communicate with the main thread:

1. sending a message – [`process.sendToMain()`](jxcore-thread.markdown#process-sendtomain-param),
2. returning the result from the task, since this is the regular method.

### Defaults

JXcore uses the following defaults for subthreads for multithreaded applications:

1. minimum amount is set to 2. This is also the default value.
2. maximum amount is set to 63.

You can also control the number of subthreads and there are two ways you can do this, but that depends how you run the application.

If you launch te code with mt/mt-keep command line parameter, you can set the threads count by applying a number after the colon, like:

    > jx mt:4 hello.js

Otherwise you can use [`tasks.setThreadCount()`](jxcore-tasks.markdown#tasks-setthreadcount-value) method.
See also [How to run multithreaded code?](jxcore-feature-multithreading.markdown#how-to-run-multithreaded-code) for more information.

### Memory management

Since node is based on V8 engine, it also inherits it’s defaults and limitations.

Currently, the default V8 memory limit is 512 MB on 32-bit systems and 1 GB on 64-bit systems.
According to the documentation, this can be raised to a maximum of 1 GB and 1.7 GB respectively.

But one of the most important factors about running multithreaded tasks in JXcore is the fact that the main thread
as well as every subthread uses its own V8 heap space, so the V8 memory limits applies to each of them separately!
For example, an application which runs on four threads can hold up to 5 x 1.7 GB of memory (1 for the main thread and 4 for the subthreads)!

And when there are no currently active tasks, JXcore subthreads force an automatic V8 heap cleanup – each of them separately on its own!

Moreover, the queue mechanism for the subthreads uses separate memory blocks than V8 engine.
It means that it occupies a different place in the memory, hence the queues do not fall under the limitations mentioned above.

### Native C++ Node.JS modules

See [native](https://github.com/Nubisa/jxdocs/tree/master/native).

## How to run multithreaded code?

There are two ways of executing your JavaScript code in multiple threads with JXcore.

### Multithreading from the command line

The first and the easiest one is just to use `mt` or `mt-keep` option in the command-line for jx:

    > jx mt-keep:4 easy1.js

This topic is fully described here: [mt / mt-keep](jxcore-command-mt.markdown)

### Multithreaded Javascript Tasks

The second one is by using `jxcore.tasks` object. It requires you to implement all the multithreaded logic by yourself.
Then you run the application without any jx’s options:

    > jx tasks_way.js

The main difference between those two methods is, that with mt/mt-keep approach you have to do absolutely nothing
to run the application multithreaded. However, the same code is running separately for each thread
and you don’t really have any control over changing the thread’s job, once the application started.

With the second (the “tasks”) approach it’s all about adding jobs to the queue of the thread pool.
Tasks start and finish while you can add more tasks in the runtime.
In this model you always have the main thread and the subthreads.
You can also get notified, when all task are completed.
See the API for reference.

This topic is fully described here: [Tasks](jxcore-tasks.markdown)
