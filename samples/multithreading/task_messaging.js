
//below is a small module can be assigned into threads
var app = {
    define: function(){ //everything global for the module should be defined here

        jxcore.tasks.on('message',function(threadId, msg){
            console.log("thread ", process.threadId, " received ", msg, "from thread", threadId);
        });

        var someMethod = function(n){
            return n+1;
        };

        process.keepAlive(); // this call will keep the main thread from exiting.
    },

    logic: function(param){ // this is the logical part that we will be calling
        setTimeout(function(){
            var number = someMethod(param); // this method is defined under 'define' section.
            process.sendToThreads(number); // send the number to all the other sub threads
            // process.sendToMain is the one for sending messages to main thread

            process.release(); // we can release the process counter.
        },10);

    }
};


jxcore.tasks.runOnce(app, 5);

// when the runOnce gets the call, it will place the above object into a thread and then
// call the 'logic' function with a given parameter. By default JXcore thread pool size is 2 (configurable)
// As a result, this will be happening twice separately per each thread.
// Since we don't know when the other threads are already applied the 'define' part, 'sendToThreads' called
// under a setTimeout for this sample.
// You may also see 'waitLogic' for an alternative approach.

