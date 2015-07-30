// Copyright & License details are available under JXCORE_LICENSE file

/*
This unit is testing readdirSync() under jx package.
There was an issue with packaged folder structure like below:

./dir/table
./dir/table/folder1
./dir/table/table1.txt
./dir/table2
./dir/table2/folder2

Now readdirSync("./dir/table") was returning "too much":
 [ 'table1.txt', 'folder1', '2' ]

Where "2" came from "table2" (after removing "table" part) which was entirely wrong.


This test makes sense when running as a package (-a, -n switch).
For plain js test works too, but this wasn't the case for the issue.
*/


var fs = require("fs");
var path = require("path");
var assert = require('assert');

var files = fs.readdirSync(path.join(__dirname, "assets", "test-packaging-readdirSync", "table"))

// expected are only two items:
// folder1 (folder)
// table1.txt (file)

assert.ok(files.indexOf("folder1") > -1, "The result array does not contain `folder1`");
assert.ok(files.indexOf("table1.txt") > -1, "The result array does not contain `table1.txt`");

assert.strictEqual(files.length, 2, "The result array should contain only two items, but contains:\n" + JSON.stringify(files, null, 4));

console.log(files);