// Copyright Nubisa Inc. 2014
//
// Object Limits & Total Memory
// 

if(console){
	global.print = console.log;
	global.timer = process.hrtime;
}

var tm;
function getTime(){
	tm = timer(tm);
	var res = tm.join(".");
	tm = timer();
	return res;
}

function load(n, multi){
	var dict = {}, dict2 = {}, dict3 = {};
	
	var sample = "Hello World!";
	var x = multi * (500*999);
	
	tm = timer();

	var range = multi * 500 * 1000;
	print("STARTING LOAD " + n);
	for(var i=0;i<range;i++){
		if(i%x == 0){
			print(i + " - " + getTime());
		}
		dict[i + ""] = sample + i + "a";
	}

	print("PART2");
	for(var i=0;i<range;i++){
		if(i%x == 0){
			print(i + " - " + getTime());
		}
		dict2[i + ""] = sample + i + "b";
	}
	
	print("PART3");
	for(var i=0;i<range;i++){
		if(i%x == 0){
			print(i + " - " + getTime());
		}
		dict3[i + ""] = sample + i + "c";
	}


	print("Hello " + (dict["111"].length 
	+ dict2["2111"].length + dict3["1111"].length));
	delete dict;delete dict2;delete dict3;
}

if(process.subThread)// if multithreaded
	load(1, parseInt(process.argv[3]));
else
	load(1, parseInt(process.argv[2]));


if(!setTimeout){
	sleep(5000);
}
else{
	setTimeout(function(){
		if(process.subThread) // if multithreaded
			process.release();
	},5000);
}
