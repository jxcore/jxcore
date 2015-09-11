var types = {
  "++" : "INCREMENT",
  "--" : "DECREASE",
  "+=": "PLUS_EQUAL",
  "*=": "MULTIPLY_EQUAL",
  "-=": "MINUS_EQUAL",
  "/=": "DIV_EQUAL",
  "^=": "BT_EQUAL",
  "|=": "MOD_EQUAL",
  "WORD": "WORD",
  " ": "SPACE",
  '"': "STRING",
  "'": "STRING",
  "//": "COMMENT",
  "/*": "COMMENT",
  "function": "FUNCTION",
  "{": "SCOPE",
  "}": "SCOPE_END", // skip only
  ":": "COLON",
  ";": "SEMI_COLON",
  "+": "PLUS",
  "%":"PERCENTAGE",
  "%=":"PERCENTAGE_EQUAL",
  "Regex": "REGEX",
  "^": "BT_SIGN",
  "/": "DIV",
  "-": "MINUS",
  ",": "COMMA",
  "*": "MULTIPLY",
  "|": "MOD",
  "||": "OR",
  "&":"AND_OPERATOR",
  "&&": "AND",
  "<": "LOWER_THAN",
  "<=": "LOWER_THAN_EQUAL",
  ">": "BIGGER_THAN",
  ">=": "BIGGER_THAN_EQUAL",
  ".": "DOT",
  "!": "NOT",
  "=": "EQUALS",
  "==": "IF_EQUALS",
  "===": "IF_STRONG_EQUALS",
  "!=": "IF_DIFFERENT",
  "!==": "IF_STRONG_DIFFERENT",
  "(": "PTS_OPEN",
  ")": "PTS_CLOSE",
  "\n": "NEW_LINE",
  "\r": "R_LINE",
  "[": "ARRAY_OPEN",
  "]": "ARRAY_CLOSE",
  "if": "IF",
  "else": "ELSE",
  "for": "FOR",
  "while": "WHILE",
  "switch": "SWITCH",
  "do": "DO",
  "case": "CASE",
  "return": "RETURN",
  "break": "BREAK",
  "throw": "THROW",
  "try": "TRY",
  "catch": "CATCH",
  "typeof":"TYPEOF",
  "instanceof":"INSTANCEOF",
  "\t": "TAB",
  "var": "SET_VARIABLE",
  "let": "SET_VARIABLE_2", // todo consider scope differences
  "?": "LINER_IF"
};

var signs = {
  "++" : "INCREMENT",
  "--" : "DECREASE",
  "+=": "PLUS_EQUAL",
  "*=": "MULTIPLY_EQUAL",
  "-=": "MINUS_EQUAL",
  "/=": "DIV_EQUAL",
  "|=": "MOD_EQUAL",
  "{": "SCOPE",
  "}": "SCOPE_END", // skip only
  ":": "COLON",
  ";": "SEMI_COLON",
  "+": "PLUS",
  "%":"PERCENTAGE",
  "%=":"PERCENTAGE_EQUAL",
  "/": "DIV",
  "-": "MINUS",
  ",": "COMMA",
  "*": "MULTIPLY",
  "|": "MOD",
  "||": "OR",
  "&":"AND_OPERATOR",
  "&&": "AND",
  "<": "LOWER_THAN",
  "<=": "LOWER_THAN_EQUAL",
  ">": "BIGGER_THAN",
  ">=": "BIGGER_THAN_EQUAL",
  ".": "DOT",
  "!": "NOT",
  "=": "EQUALS",
  "==": "IF_EQUALS",
  "===": "IF_STRONG_EQUALS",
  "!=": "IF_DIFFERENT",
  "!==": "IF_STRONG_DIFFERENT",
  "(": "PTS_OPEN",
  ")": "PTS_CLOSE",
  "[": "ARRAY_OPEN",
  "]": "ARRAY_CLOSE",
  "?": "LINER_IF",
  "^": "BT_SIGN",
  "^=": "BT_EQUAL"
};

var reverse_signs = {};

for(var o in types) {
  if (!signs.hasOwnProperty(o)){
    continue;
  }
  reverse_signs[signs[o]] = o;
}

exports.signs = reverse_signs;

exports.prexp = {
  "COMMA": ",",
  "MOD" : "|",
  "NOT" : "!",
  "OR" : "||",
  "AND" : "&&",
  "SEMI_COLON": ";",
  "EQUALS": "=",
  "PTS_OPEN": "(",
  "COLON": ":",
  "PLUS_EQUAL" :"+=",
  "MULTIPLY_EQUAL": "*=",
  "MINUS_EQUAL": "-=",
  "DIV_EQUAL": "/=",
  "MOD_EQUAL": "|=",
  "ARRAY_OPEN": "[",
  "IF_EQUALS": "==",
  "IF_STRONG_EQUALS":"===",
  "IF_DIFFERENT":"!=",
  "IF_STRONG_DIFFERENT":"!==",
  "LOWER_THAN":"<",
  "LOWER_THAN_EQUAL":"<=",
  "BIGGER_THAN":">",
  "BIGGER_THAN_EQUAL":">=",
  "RETURN":"return",
  "LINER_IF":"?",
  "ELSE":"else",
  "PLUS":"+",
  "MINUS":"-",
  "MULTIPLY":"*",
  "PERCENTAGE":"%"
};

exports.headlessScopes = {
  "IF": "if",
  "FOR": "for",
  "WHILE": "while"
};

exports.noCode = {
  "R_LINE": "\r",
  "SPACE": " ",
  "TAB": "\t",
  "NEW_LINE": "\n",
  "COMMENT": "//"
};

exports.types = types;

var list = {};

for(var o in types) {
  if (!types.hasOwnProperty(o)){
    continue;
  }
  list[types[o]] = o;
}

exports.list = list;

exports.code = "";
exports.activeWord = "";
exports.activeBlock = null;
exports.columnIndex = 1;
exports.rowIndex = 1;

function block() {
  if (!(this instanceof block)) return new block();
  this.columnIndex = exports.columnIndex;
  this.rowIndex = exports.rowIndex;
  this.parentIndex = null;
  this.type = null;
  this.delimiter = null;
  this.variables = {};

  this.index = null;
  this.length = null;

  this.subs = [];
  this.parent = null;

  var _this = this;
  this.getData = function() {
    if (_this.index == null || _this.length == null) return list[this.type];

    return exports.code.substr(_this.index-1, _this.length);
  };

  this.findType = function() {
    var str = _this.getData().trim();
    if (!str) {
      _this.dataType = 'undefined';
      return;
    }

    if (str == 'null') {
      _this.dataType = 'null';
    } else if (str == 'true' || str == 'false')
      _this.dataType = 'boolean';
    else if (!isNaN(parseInt(str)) || !isNaN(parseFloat(str))) {
      _this.dataType = "number";
    } else {
      var prev = _this.getPreviousBlock();

      if (prev && prev.type === "SET_VARIABLE") {
        if (_this.parent) {
          _this.parent.variables[str] = {};
        }
        _this.dataType = "new_variable";
      }
    }
  };

  // returns none space etc previous item
  // --> returns undefined if
  // a - no item
  this.getPreviousBlock = function(dont_skip) {
    if (!_this.parent || _this.parentIndex === null || _this.parentIndex === 0) return;

    var bl = _this.parent.subs;
    for (var i = _this.parentIndex - 1; i >= 0; i--) {
      if (!dont_skip && exports.noCode[bl[i].type]) continue;
      return bl[i];
    }
  };

  // returns none space etc next item
  // --> returns undefined if
  // a - no item
  this.getNextBlock = function(dont_skip) {
    var ln = _this.parent.subs.length - 1;
    if (!_this.parent || _this.parentIndex === null || _this.parentIndex + 1 > ln) return;

    var bl = _this.parent.subs;
    for (var i = _this.parentIndex + 1; i <= ln; i++) {
      if (!dont_skip && exports.noCode[bl[i].type]) continue;
      return bl[i];
    }
  }
}

exports.block = block;