// Copyright & License details are available under JXCORE_LICENSE file


var fs = require("fs");
var path = require("path");

var json_file = path.join(__dirname, "config.json");
if (!fs.existsSync(json_file)) {
  console.error("The config.json file does not exists:", json_file);
  process.exit(1);
}

var json_str = fs.readFileSync(json_file).toString();
var json = {};
try {
  json = JSON.parse(json_str);
} catch (ex) {
  console.error("Cannot parse json string:");
  console.error(json_str);
  process.exit(1);
}


for (var o in json.should_be_readable) {
  var relative_path = json.should_be_readable[o];
  try {
    fs.readFileSync(relative_path).toString();
  } catch(ex) {
    console.error("Cannot read file from package:\n\t" + relative_path);
  }
}


for (var o in json.should_not_be_readable) {
  var relative_path = json.should_not_be_readable[o];
  try {
    fs.readFileSync(relative_path).toString();
    console.error("File should not be readable from package:" + relative_path);
  } catch(ex) {
    // ok
  }
}