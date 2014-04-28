/**
 * Created by Gonzo on 4/21/14.
 */

var fs = require('fs');

console.log("readFileSync ... \n");

console.log("asset.txt", fs.readFileSync(__dirname + '/asset.txt') + "");
console.log("other.txt", fs.readFileSync(__dirname + '/subFolder/other.txt') + "");

console.log("\nreadDirSync ... \n");

console.log("subFolder", fs.readdirSync(__dirname + "/subFolder"));

//below function's source won't be visible
exports.hiddenFunction = function(){
    /*_jx_protected_*/
    console.log("Hello!");
};

//a normal function hence it's source visible
exports.normalFunction = function(){
    console.log("Hello!");
};
