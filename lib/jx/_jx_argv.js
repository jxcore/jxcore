// Copyright & License details are available under JXCORE_LICENSE file

var argvParsed = null;

var argvParser = function (arr) {

  var result = {_: {}, _array : [] };

  var checkArg = function (arg) {

    var ret = {dashes: null, arg: null};

    if (arg.slice(0, 2) === "--") {
      ret.dashes = "--";
      var s = arg.slice(2);
      var equalSignId = s.indexOf("=");
      if (equalSignId === -1) {
        ret.arg = s;
      } else {
        ret.arg = s.slice(0, equalSignId);
        ret.value = s.slice(equalSignId + 1);
      }
    } else if (arg.slice(0, 1) === "-") {
      ret.dashes = "-";
      ret.arg = arg.slice(1);
    } else {
      ret.arg = arg;
    }

    return ret;
  };

  var checkNextValue = function () {
    if (!arr.length)
      return true;

    var v = checkArg(arr[0]);
    if (v.dashes) {
      // the next param is not a value, it is just another arg
      return true;
    } else {
      arr.shift();
      return v.arg;
    }
  };

  while (arg = arr.shift()) {

    var _arg = checkArg(arg);
    if (_arg.dashes) {
      if (_arg.dashes === "-" && _arg.arg.length > 1) {
        // multiple params after single dash, like: -abc
        var splitted = _arg.arg.split('');
        for(var o = 0, len = splitted.length; o < len; o++){
          result[splitted[o]] = true;
        }
      } else {
        // regular params with values: -a 5 --name=value --name value
        result[_arg.arg] = _arg.value || checkNextValue();
      }
    } else {
      // params without leading dashes (- or --) are treated like values
      result["_"][_arg.arg] = true;
    }
  }

  return result;
};


exports.parse = function (argv) {

  // custom argv
  if (argv) return argvParser(argv);

  // process.argv
  if (!argvParsed)
    argvParsed = argvParser(process.argv.slice(process.isPackaged ? 1 : 2));

  return argvParsed;
};

exports.sep = process.env.JX_ARG_SEP || ",";