// Copyright & License details are available under JXCORE_LICENSE file

var binding = process.binding('evals');

module.exports = Script;
Script.Script = Script;

function Script(code, ctx, filename) {
  if (!(this instanceof Script)) { return new Script(code, ctx, filename); }

  var ns = new binding.NodeScript(code, ctx, filename);

  // bind all methods to this Script object
  Object.keys(binding.NodeScript.prototype).forEach(
          function(f) {
            if (typeof binding.NodeScript.prototype[f] === 'function') {
              this[f] = function() {
                if (!(this instanceof Script)) { throw new TypeError(
                        'invalid call to ' + f); }
                return ns[f].apply(ns, arguments);
              };
            }
          }, this);
}

Script.createScript = function(code, ctx, name) {
  return new Script(code, ctx, name);
};

Script.createContext = binding.NodeScript.createContext;
Script.runInContext = binding.NodeScript.runInContext;
Script.runInThisContext = binding.NodeScript.runInThisContext;
Script.runInNewContext = binding.NodeScript.runInNewContext;