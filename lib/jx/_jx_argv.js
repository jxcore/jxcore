// Copyright & License details are available under JXCORE_LICENSE file

var argvParsed = null;
var separators = [ "=", ":" ];
var prefixes = [ "--", "-" ]; // leave this order
if (process.platform === "win32")
  prefixes.push("/");

var path = require("path");
var fs = require("fs");

var jxSpecials = {
  "install" : {
    "expectMainModule" : false
  },
  "npm" : {
    "expectMainModule" : false
  },
  "monitor" : {
    "expectMainModule" : false
  },
  "package" : {
    "expectMainModule" : true
  },
  "compile" : {
    "expectMainModule" : true
  },
  "mt" : {
    "expectMainModule" : true
  },
  "mt-keep" : {
    "expectMainModule" : true
  }
};

// encapsulation of single argument
var singleArg = function(arg, _index) {

  var _this = this;

  this.index = _index;
  this.org = arg;
  this.name;
  this.value;
  // indicates if value was given for an arg, e.g. --arg=2 or -d 1 etc.
  this.hasValue;
  // one of [ "--", "-", "/"], e.g. --arg=2 or -d or /T
  this.prefix;
  // one of [ "=", ":", " "], e.g. --arg=2 or --arg:2 or -arg 2
  this.valueSep;
  this.isInt;
  this.asInt;
  this.isBool;
  this.asBool;

  this.setValue = function(_value) {
    _this.value = _value;

    var _int = parseInt(_value);
    if (!isNaN(_int)) {
      _this.isInt = true;
      _this.asInt = _int;
    }

    var b = exports.isBoolValue(_value);
    if (b.isFalse || b.isTrue) {
      _this.isBool = true;
      _this.asBool = b.isFalse ? false : true;
    }

    _this.hasValue = typeof _this.value !== "undefined";
  };

  // splits value by a given separator (or exports.sep)
  this.splitBySep = function(sep) {

    sep = sep || exports.sep;

    var arr = _this.value === undefined ? [] : _this.value.toString()
        .split(sep);
    var ret = [];
    // copying non-empty parts
    for (var a = 0, len = arr.length; a < len; a++)
      if (arr[a].trim())
        ret.push(arr[a]);

    return ret.length ? ret : null;
  };

  // checking if value is available after a separator within this argv,
  // e.g. --arg=1 or --arg:2
  var splitValue = function(_value) {
    for (var a = 0, len = separators.length; a < len; a++) {
      var separator = separators[a];
      var sepId = _value.indexOf(separator);
      if (sepId === -1) {
        _this.name = _value;
      } else {
        _this.name = _value.slice(0, sepId);
        _this.valueSep = separator;
        _this.setValue(_value.slice(sepId + 1));
        break;
      }
    }
  };

  var found = false;
  for (var p = 0, len = prefixes.length; p < len; p++) {
    var prefix = prefixes[p];
    // checking if the arg is prefixed, e.g. --arg or -arg
    if (arg.indexOf(prefix) === 0 && arg.indexOf(" ") === -1) {
      found = true;
      this.prefix = prefix;
      arg = arg.slice(prefix.length);
      splitValue(arg);
      break;
    }
  }

  if (!found)
    this.name = arg;
};

// parses array of args (e.g. process.argv)
var argvParser = function(arr, options) {

  var result = {};
  var index = -1;
  if (!options)
    options = {};

  var checkNextValue = function() {
    if (!arr.length)
      return;

    var v = new singleArg(arr[0]);
    if (v.prefix) {
      // the next param is not a value, it is just another arg
      v = null;
      return;
    } else {
      arr.shift();
      index++;
      return v.org;
    }
  };

  var addNoInternals = function(obj) {
    if (!(obj instanceof singleArg))
      return;

    if (options.internals) {
      if (options.internals.indexOf(obj.name) !== -1
          || options.internals.indexOf((obj.prefix || "") + obj.name) !== -1)
        return;
    }

    noInternals.push(obj.org);
    if (obj.hasValue && obj.valueSep === " ")
      noInternals.push(obj.value);
  };

  var set = function(name, arg) {
    if (!result[name] || options.overwrite)
      result[name] = arg;

    addNoInternals(arg);
  };

  var setExtra = function(name, obj) {
    if (!result["_"])
      result["_"] = {};

    result["_"][name] = obj;
    addNoInternals(obj);
  };

  // copy
  var arr = arr.slice(0);
  var noPrefix = [];
  var noInternals = [];
  var expectMainFileName = true;
  var arg;
  while (arg = arr.shift()) {
    index++;

    if (index === 0) {
      noInternals.push(arg);
      continue;
    }

    if (index === 1) {
      // mt / mt-keep special treatments
      if (arg.slice(0, 2) === "mt") {
        arg = "--" + arg;
        if (arg.indexOf(":") === -1)
          arg += ":2";
      } else {

      }
    }

    var _arg = new singleArg(arg, index);

    // checking for special jx command and eventually mainModule file name in
    // argv
    if (expectMainFileName && !_arg.prefix) {
      if (jxSpecials[_arg.name]) {
        setExtra(_arg.name, true);
        expectMainFileName = jxSpecials[_arg.name].expectMainModule;
      } else {
        expectMainFileName = false;
        setExtra("mainModule", arg);
      }
      noInternals.push(arg);
      continue;
    }

    if (_arg.prefix) {
      if (options.splitSingleDashed && _arg.prefix === "-"
          && _arg.name.length > 1) {
        // multiple params after single dash, like: -abc
        var splitted = _arg.name.split('');
        for (var o = 0, len = splitted.length; o < len; o++) {
          set(splitted[o], new singleArg(splitted[o], index));
        }
        continue;
      }

      // regular params with values: -a 5 --name=value --name value
      if (!_arg.hasValue) {
        var val = checkNextValue();
        if (val !== undefined) {
          _arg.setValue(val);
          _arg.valueSep = " "; // space
        }
      }

      set(_arg.name, _arg);
    } else {
      // params without leading dashes (- or --) are treated like values
      _arg.setValue(_arg.name);
      _arg.hasValue = false;
      noPrefix.push(_arg.name);
      set(_arg.name, _arg);
    }
  }

  if (noPrefix.length)
    setExtra("withoutPrefix", noPrefix);

  setExtra("withoutInternals", noInternals);

  return result;
};

// argv and options are optional
exports.parse = function(argv, options) {

  if (Object.prototype.toString.call(argv) === '[object Array]') {
    // argv is given
    return argvParser(argv, options);
  } else {
    // argv is not given, so we'll parse process.argv

    if (argv && Object.prototype.toString.call(argv) === '[object Object]') {
      // argv is options
      options = argv || {};
    }

    if (!options)
      options = {};

    if (argvParsed && !options.force)
      return argvParsed;

    argvParsed = argvParser(process.argv, options);
    return argvParsed;
  }
};

var getSep = function() {

  if (!process.env.JX_ARG_SEP)
    return ","; // default is comma

  var sep = process.env.JX_ARG_SEP;
  if (process.platform === "win32") {
    // windows needs stripping quotes if used, e.g.: set JX_ARG_SEP="@"
    var first = sep.trim().slice(0, 1);
    var last = sep.trim().slice(-1);
    if ((first === '"' && last === '"') || (first === "'" && last === "'")) {
      sep = sep.trim(); // trimming before first quote and after last one
      sep = sep.slice(1, sep.length - 1);
    } else {
      // if quotes are not used, unix would trim, so let's trim on windows too,
      // e.g.:
      // set JX_ARG_SEP=@ && echo ok
      // (there are spaces after @)
      sep = sep.trim();
    }
  }
  return sep;
};

exports.sep = getSep();

// removes an arg and it's value if exists (alters process.argv!)
// returns true if arg was found and removed
// `arg` is expected to be entire name (including prefixes and value after : or
// =)
exports.remove = function(arg, expectValue) {

  var cnt = 1;
  var id = process.argv.indexOf(arg);
  if (id === -1)
    return false;

  if (expectValue && process.argv[id + 1]) {
    // if the next arg is a value for this arg (is not prefixed),
    // remove it too, e.g. --arg value
    var _arg = new singleArg(process.argv[id + 1]);
    if (!_arg.prefix)
      cnt++;
  }

  process.argv.splice(id, cnt);
  return true;
};

exports.getValue = function(arg, defaultValue) {
  if (!argvParsed)
    exports.parse();

  if (argvParsed[arg]) {
    if (argvParsed[arg].value === undefined)
      return defaultValue;

    return argvParsed[arg].value;
  } else
    return defaultValue;
};

exports.getBoolValue = function(arg, defaultValue) {
  if (!argvParsed)
    exports.parse();

  if (argvParsed[arg])
    // even if !isBool, returns true, because the arg exists
    return argvParsed[arg].isBool ? argvParsed[arg].asBool : true;
  else
    return !!defaultValue;
};

exports.isBoolValue = function(arg) {

  if (typeof arg === "undefined" || arg === null || !arg.toString())
    return {
      isFalse : false,
      isTrue : false
    };

  var lc = arg.toString().toLowerCase();

  return {
    isFalse : (lc === "no" || lc == "false" || lc == "0"),
    isTrue : (lc === "yes" || lc == "true" || lc == "1")
  }
};
