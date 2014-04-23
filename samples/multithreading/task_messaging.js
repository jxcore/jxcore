var app = {
    define: function(){
        jxcore.tasks.on('message',function(threadId, msg){
           console.log("thread ", process.threadId, " received ", msg, "from thread", threadId);
        });

        var someMethod = function(n){
            return n+1;
        };
    },

    logic: function(param){
        process.sendToAll(someMethod(param));
    }
};


app.runTask(5);
