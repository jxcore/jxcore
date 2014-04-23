var method = function(params){
    console.log("received ", params);
};

method.runTask(15); // runs under a separate thread
// alternatively you could call
// jxcore.tasks.addTask(method, 15);

var method2 = function(params){
    return params + 1;
};

method2.runTask(1, function(result){ // runs under a separate threads
    console.log("result ", result);
});
// alternatively you could call
// jxcore.tasks.addTask(method2, 1, function(result){ .... });

