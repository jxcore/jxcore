// Copyright Microsoft. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and / or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

(function () {
  // Save original builtIns
  var Object_defineProperty = Object.defineProperty,
      Object_getOwnPropertyDescriptor = Object.getOwnPropertyDescriptor,
      Object_getOwnPropertyNames = Object.getOwnPropertyNames,
      Object_keys = Object.keys,
      Reflect_construct = Reflect.construct;

  // Simulate V8 JavaScript stack trace API
  function StackFrame(funcName, fileName, lineNumber, columnNumber) {
    this.column = columnNumber;
    this.lineNumber = lineNumber;
    this.scriptName = fileName;
    this.functionName = funcName;
  }

  StackFrame.prototype.getFunctionName = function() {
      return this.functionName;
  };

  StackFrame.prototype.getFileName = function () {
      return this.scriptName;
  };

  StackFrame.prototype.getLineNumber = function() {
    return this.lineNumber;
  };

  StackFrame.prototype.getColumnNumber = function () {
    return this.column;
  };

  StackFrame.prototype.isEval = function() {
    // TODO
    return false;
  };

  StackFrame.prototype.toString = function () {
    return (this.functionName || 'Anonymous function') + ' (' +
      this.scriptName + ':' + this.lineNumber + ':' + this.column + ')';
  };

  function prepareStackTrace(error, stack) {
    var stackString = (error.name ? error.name : 'Error') +
      ': ' + error.message;

    for (var i = 0; i < stack.length; i++) {
      stackString += '\n   at ' + stack[i].toString();
    }

    return stackString;
  }

  function parseStack(stack, skipDepth, startFunc) {
    // remove the first line so this function won't be seen
    var splittedStack = stack.split('\n');
    splittedStack.splice(0, 2);
    var errstack = [];

    var startName = skipDepth < 0 ? startFunc.name : undefined;
    skipDepth = Math.max(0, skipDepth);

    for (var i = skipDepth; i < splittedStack.length; i++) {
      var parens = /\(/.exec(splittedStack[i]);
      var funcName = splittedStack[i].substr(6, parens.index - 7);

      if (startName) {
        if (funcName === startName) {
          startName = undefined;
        }
        continue;
      }
      if (funcName === 'Anonymous function') {
        funcName = null;
      }

      var location = splittedStack[i].substr(parens.index + 1,
          splittedStack[i].length - parens.index - 2);

      var fileName = location;
      var lineNumber = 0;
      var columnNumber = 0;

      var colonPattern = /:[0-9]+/g;
      var firstColon = colonPattern.exec(location);
      if (firstColon) {
        fileName = location.substr(0, firstColon.index);

        var secondColon = colonPattern.exec(location);
        if (secondColon) {
          lineNumber = parseInt(location.substr(firstColon.index + 1,
              secondColon.index - firstColon.index - 1), 10);
          columnNumber = parseInt(location.substr(secondColon.index + 1,
              location.length - secondColon.index), 10);
        }
      }
      errstack.push(
          new StackFrame(funcName, fileName, lineNumber, columnNumber));
    }
    return errstack;
  }

  function findFuncDepth(func) {
    try {
      var curr = captureStackTrace.caller;
      var limit = Error.stackTraceLimit;
      var skipDepth = 0;
      while (curr) {
        skipDepth++;
        if (curr === func) {
          return skipDepth;
        }
        if (skipDepth > limit) {
          return 0;
        }
        curr = curr.caller;
      }
    } catch (e) {
      // Strict mode may throw on .caller. Will try to match by function name.
      return -1;
    }

    return 0;
  }

  function captureStackTrace(err, func) {
    var currentStack;
    try { throw new Error; } catch (e) { currentStack = e.stack; }
    var isPrepared = false;
    var skipDepth = func ? findFuncDepth(func) : 0;

    var currentStackTrace;
    function ensureStackTrace() {
      if (!currentStackTrace) {
        currentStackTrace = parseStack(currentStack, skipDepth, func);
      }
      return currentStackTrace;
    }

    Object_defineProperty(err, 'stack', {
      get: function () {
        if (isPrepared) {
          return currentStack;
        }
        var errstack = ensureStackTrace();
        if (Error.prepareStackTrace) {
          currentStack = Error.prepareStackTrace(err, errstack);
        } else {
          currentStack = prepareStackTrace(err, errstack);
        }
        isPrepared = true;
        return currentStack;
      },
      set: function (value) {
        currentStack = value;
        isPrepared = true;
      },
      configurable: true,
      enumerable: false
    });

    return ensureStackTrace;
  };

  function patchErrorStack() {
    Error.captureStackTrace = captureStackTrace;
  }

  function cloneObject(source, target) {
    Object_getOwnPropertyNames(source).forEach(function(key) {
      try {
        var desc = Object_getOwnPropertyDescriptor(source, key);
        if (desc.value === source) desc.value = target;
        Object_defineProperty(target, key, desc);
      } catch (e) {
        // Catch sealed properties errors
      }
    });
  }

  // Chakra Error instances have some enumerable properties (error number and
  // stack), causing node formatting differences. Try make those properties
  // non-enumerable when creating Error instances.
  // NOTE: This doesn't work if Error is created in Chakra runtime.
  function patchErrorTypes() {
    function makePropertiesNonEnumerable(e) {
      Object_keys(e).forEach(function (key) {
        Object_defineProperty(e, key, { enumerable: false });
      });
      return e;
    }

    var builtInError = Error;

    [Error, EvalError, RangeError, ReferenceError, SyntaxError, TypeError,
      URIError
    ].forEach(function (type) {
      var newType = (function () {
        // Make anonymous function. It may appear in error.stack
        return function () {
          return makePropertiesNonEnumerable(
            Reflect_construct(type, arguments));
        };
      })();
      cloneObject(type, newType);
      newType.toString = function () {
        return type.toString();
      };
      this[type.name] = newType;
    });

    // Delegate Error.stackTraceLimit to saved Error constructor
    Object_defineProperty(this['Error'], 'stackTraceLimit', {
      enumerable: false,
      configurable: true,
      get: function () { return builtInError.stackTraceLimit; },
      set: function (value) { builtInError.stackTraceLimit = value; }
    });
  }

  function patchUtils(utils) {
    var isUintRegex = /^(0|[1-9]\\d*)$/;
    var isUint = function (value) {
      var result = isUintRegex.test(value);
      isUintRegex.lastIndex = 0;
      return result;
    };
    utils.isInstanceOf = function (a, b) {
      return (a instanceof b);
    };
    utils.cloneObject = cloneObject;
    utils.forEachNonConfigurableProperty = function (source, callback) {
      Object_getOwnPropertyNames(source).forEach(function (key) {
        var desc = Object_getOwnPropertyDescriptor(source, key);
        if (desc && !desc.configurable && !callback(key, desc)) {
          return false;
        }
      });

      return true;
    };
    utils.getPropertyNames = function (a) {
      var names = [];
      for (var propertyName in a) {
        names.push(propertyName);
      }
      return names;
    };
    utils.getEnumerableNamedProperties = function (obj) {
      var props = [];
      for (var key in obj) {
        if (!isUint(key))
          props.push(key);
      }
      return props;
    };
    utils.getEnumerableIndexedProperties = function (obj) {
      var props = [];
      for (var key in obj) {
        if (isUint(key))
          props.push(key);
      }
      return props;
    };
    utils.createEnumerationIterator = function (props) {
      var i = 0;
      return {
        next: function () {
          if (i === props.length)
            return { done: true }
          return { value: props[i++] };
        }
      };
    };
    utils.createPropertyDescriptorsEnumerationIterator = function (props) {
      var i = 0;
      return {
        next: function () {
          if (i === props.length) return { done: true }
          return { name: props[i++], enumerable: true };
        }
      };
    };
    utils.getNamedOwnKeys = function (obj) {
      var props = [];
      Object_keys(obj).forEach(function (item) {
        if (!isUint(item))
          props.push(item);
      });
      return props;
    };
    utils.getIndexedOwnKeys = function (obj) {
      var props = [];
      Object_keys(obj).forEach(function (item) {
        if (isUint(item))
          props.push(item);
      });
      return props;
    };

    var createEmptyLambdaFunction = function (length) {
      if (length === undefined) {
        return () => { };
      }

      // length is specified when we are marshalling a bound function of given
      // length
      var func;
      switch (length) {
        case 0: func = () => { }; break;
        case 1: func = (x1) => { }; break;
        case 2: func = (x1, x2) => { }; break;
        case 3: func = (x1, x2, x3) => { }; break;
        case 4: func = (x1, x2, x3, x4) => { }; break;
        case 5: func = (x1, x2, x3, x4, x5) => { }; break;
        case 6: func = (x1, x2, x3, x4, x5, x6) => { }; break;
        case 7: func = (x1, x2, x3, x4, x5, x6, x7) => { }; break;
        case 8: func = (x1, x2, x3, x4, x5, x6, x7, x8) => { }; break;
        default: {
          var str = "(x1";
          for (var i = 2; i <= length; i++) {
            str += ", x" + i;
          }
          str += ") => { }";
          func = eval(str);
        }
      }
      return func.bind({});
    };
    var createEmptyStrictModeFunction = function () {
      return function () { "use strict"; return arguments; };
    };

    var BOUND_FUNCTION_TAG = Symbol("BOUND_FUNCTION_TAG"),
        Function_prototype_bind = Function.prototype.bind;
    Function.prototype.bind = function () {
      var r = Function_prototype_bind.apply(this, arguments);
      r[BOUND_FUNCTION_TAG] = true;
      return r;
    };

    var NORMAL_FUNCTION = 0,
        BOUND_FUNCTION = 1,
        STRICTMODE_FUNCTION = 2;
    var TYPE_BITS = 2, // lower 2 bits for type
        TYPE_MASK = (1 << TYPE_BITS) - 1;

    utils.testFunctionType = function (func) {
      if (func[BOUND_FUNCTION_TAG]) {
        return (func.length << TYPE_BITS) | BOUND_FUNCTION;
      }

      var desc = Object_getOwnPropertyDescriptor(func, 'caller');
      return (desc && desc.get) ? STRICTMODE_FUNCTION : NORMAL_FUNCTION;
    };
    utils.createTargetFunction = function (type) {
      switch (type & TYPE_MASK) {
        case BOUND_FUNCTION:
          return createEmptyLambdaFunction(type >> TYPE_BITS);
        case STRICTMODE_FUNCTION:
          return createEmptyStrictModeFunction();
      }
      return createEmptyLambdaFunction();
    };

    function getOwnNamedDescriptor(fnc, name) {
      var desc = Object_getOwnPropertyDescriptor(fnc, name);
      if (typeof desc !== 'undefined')
        return desc.get;
      else
        return function () { throw Error('Given descriptor was `undefined`'); };
    }

    utils.throwAccessorErrorFunctions = (function () {
      var arr = [];

      var fnc = createEmptyLambdaFunction(0);
      arr.push(getOwnNamedDescriptor(fnc, 'caller'));

      fnc = createEmptyStrictModeFunction();
      arr.push(getOwnNamedDescriptor(fnc, 'caller'));
      arr.push(getOwnNamedDescriptor(fnc, 'arguments'));
      arr.push(getOwnNamedDescriptor(fnc(), 'callee'));

      return arr;
    })();

    utils.getStackTrace = function () {
      return captureStackTrace({}, utils.getStackTrace)();
    };
  }

  // patch console
  patchErrorTypes();
  patchErrorStack();

  // this is the keepAlive object that we will put some utilities function on
  patchUtils(this);
})
