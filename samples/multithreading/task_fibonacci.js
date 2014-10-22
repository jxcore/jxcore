var fibo = {
    define:function() {
        function fibo_(n) {
            return n > 1 ? fibo_(n - 1) + fibo_(n - 2) : n;
        }
    },
    logic:function(i){
        var start = Date.now();
        var n = fibo_(i);
        console.log("fibonnaci number index n =", i, "is:", n, Date.now() - start, "ms", "threadId", process.threadId);
    }
};


var threadCount = 16; // play with the numbers here
jxcore.tasks.setThreadCount(threadCount);

var start_time;
var first_run = true;

// first, dummy task
// the goal is to avoid loading the processor with all the thread tasks right from the start
// let's give some time for threads to load, so for the beginning we add just one, dummy task
jxcore.tasks.addTask(fibo, 1);

jxcore.tasks.on('emptyQueue', function () {
    if (first_run) {
        // the first, dummy task has been completed
        // now all of the threads should be up-and-running
        // and this is the right moment to execute the other tasks 
        first_run = false;
        start_time = Date.now();

        for (var i = 1; i <= 45; i++) {
            jxcore.tasks.addTask(fibo, i);
        }
    } else {
        console.log("Total time for", threadCount, "threads is", Date.now() - start_time, "ms");
    }
});
