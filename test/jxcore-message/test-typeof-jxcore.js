// Copyright & License details are available under JXCORE_LICENSE file

// gets members of an object and returns as sorted array
var getMembersNames = function(obj, name) {
  var arr = [];

  for(var o in obj) {
    if (obj.hasOwnProperty(o))
      arr.push(o);
  }

  arr.sort();

  // printing
  console.log("\nnamespace:", name);
  for(var o = 0, len = arr.length; o < len; o++) {
    var _name = arr[o];
    console.log("\ttypeof", _name, "=", typeof obj[_name]);
  }
};

getMembersNames(jxcore, "jxcore");
getMembersNames(jxcore.monitor, "jxcore.monitor");
getMembersNames(jxcore.store, "jxcore.store");
getMembersNames(jxcore.tasks, "jxcore.tasks");
getMembersNames(jxcore.utils, "jxcore.utils");
