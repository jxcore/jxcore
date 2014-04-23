var method = function(){
    console.log("this method will be called once per each thread.", "thread Id", process.threadId);
};

//by default JXcore threading pool size is 2
jxcore.tasks.setThreadCount(4); // sets it to 4

method.runOnce(); // this will run 4 times.
// alternatively you could call
// jxcore.tasks.runOnce(method);
