var commons = require('./commons');
var types = commons.types;
var Block = commons.block;
var blocks;

function newBlock(delimiter, index) {
  var bl = new Block();
  bl.delimiter = delimiter;
  bl.index = index;
  bl.type = types[delimiter];

  if (commons.activeBlock) {
    bl.parent = commons.activeBlock;
  }

  commons.activeBlock = bl;
}

function identifyScope(scope) {
  var bl = scope.subs.length ? scope.subs[0] : null;

  if (!bl) {
    // {}
    scope.dataType = "object";
    return;
  }

  if (bl && commons.noCode[bl.type]) {
    bl = bl.getNextBlock();
  }

  scope.dataType = "code";

  do {
    if (bl && (bl.type == "STRING" || bl.type == "WORD")) {
      bl = bl.getNextBlock();

      if (bl && bl.type == "COLON") {
        var prev = bl;
        do {
          prev = bl;
          bl = bl.getNextBlock();
        } while (bl && bl.type != "COMMA");

        var hasComma = bl ? true : false;
        if (hasComma)
          bl = bl.getNextBlock();

        if (!bl) {
          scope.dataType = "object";

          if (hasComma) {
            commons.errorInfo = {
              type: "SyntaxError",
              msg: "Unintentional comma at the end of the object definition",
              column: scope.columnIndex,
              row: scope.rowIndex
            };
          }
          return;
        }
      } else break;
    } else break;
  } while(bl);
}

function saveBlock(index) {
  if (!commons.activeBlock) throw new Error("no active block!");

  if (commons.activeBlock.type != "STRING" && commons.activeBlock.type != "SCOPE") {
    commons.activeBlock.columnIndex += commons.activeBlock.index - index;
  }

  var hasParent = commons.activeBlock.parent ? true : false;
  if (!hasParent) {
    commons.activeBlock.parent = blocks;
    commons.activeBlock.parentIndex = blocks.subs.length;
  } else {
    commons.activeBlock.parentIndex = commons.activeBlock.parent.subs.length;
  }

  var skip_adding = false;

  if (commons.activeBlock.type == "SCOPE") {
    // see if scope is an object definition
    identifyScope(commons.activeBlock);
  }
  // detect data type for WORDS
  else if (commons.activeBlock.type == "WORD") {
    commons.activeBlock.findType();
  } // see if we can merge space into previous one
  else if (commons.activeBlock.type == "SPACE") {
    var bl = commons.activeBlock.getPreviousBlock(true);
    if (bl && bl.type == "SPACE") {
      // merge
      bl.length += commons.activeBlock.length;
      skip_adding = true;
    }
  }

  if (!skip_adding) {
    commons.activeBlock.parent.subs.push(commons.activeBlock);
  }

  if (hasParent) {
    commons.activeBlock = commons.activeBlock.parent;
  } else {
    commons.activeBlock = null;
  }
}

function fillString(pchar, char, nchar, index) {
  // \n or ' " can break a string
  if (char == '\n') {
    // if previous char was a '\' then continue ( \\n )
    if (pchar == '\\')
      return;

    // throw a syntax error
    return {
      type: "SyntaxError",
      msg: "String definition breaks.",
      row: commons.rowIndex,
      column: commons.columnIndex
    };
  }

  if (char == commons.activeBlock.delimiter) {
    if (pchar == '\\') {
      if (passSlash(index)) return;
    }

    commons.activeBlock.length = (index - commons.activeBlock.index) + 1;
    saveBlock(index);
  }
}

function passSlash(index) {
  var counter = 0;
  index -=1; // we receive index starting from 1 (this is not the 0-> index)
  for (var i = index - 1; i > commons.activeBlock.index - 1; i--) {
    var ch = commons.code.charAt(i);
    if (ch == '\\')
      counter++;
    else
      break;
  }

  return (counter % 2 == 1);
}

function regexFail(index) {
  var n = commons.activeBlock.index - index;
  // looks like this wasn't a regex
  commons.activeBlock.type = types['/'];
  commons.activeBlock.length = 1;
  saveBlock(index);
  return {move: n};
}

function fillRegex(pchar, char, nchar, index) {
  var comb = char + nchar;
  var finishes = comb.match(/([\/]+[a-z]+)/i);

  // / can break a regex
  if (finishes || (char == '/' && !commons.activeBlock.regexOpen)) {
    // if previous char was a '\' then continue ( \\/ )
    if (pchar == '\\') {
      if (passSlash(index)) return;
    }

    commons.activeBlock.length = (index - commons.activeBlock.index) + (finishes ? 2 : 1);
    saveBlock(index);

    return {move: finishes ? 1 : 0};
  } else {
    if ((pchar != '\\' && char == '[') || (char == '{' && nchar == '{')) {
      commons.activeBlock.regexOpen = char;
    } else if (commons.activeBlock.regexOpen == '{' && char == '}' && nchar == '}') {
      commons.activeBlock.regexOpen = false;
    } else if (commons.activeBlock.regexOpen == '[' && nchar == ']') {
      commons.activeBlock.regexOpen = false;
    } else if (char == ')' && pchar != '\\' && commons.activeBlock.regexOpen) {
      commons.activeBlock.regexOpen = false;
    } else
    // failsafe
    if (char == '\n') {
      return regexFail(index);
    }
  }
}

function fillComment(pchar, char, nchar, index) {
  // \n or */ can break the comment block
  if (char == '\n') {

    // \n can not break a /* block
    if (commons.activeBlock.delimiter == '/*')
      return;

    // do not add the last \n
    commons.activeBlock.length = (index - commons.activeBlock.index);
    saveBlock(index);
    return next(pchar,char, nchar, index);
  }

  if (commons.activeBlock.delimiter == '/*') {
    if (char != '/') return;

    if (pchar == '*') {
      commons.activeBlock.length = (index - commons.activeBlock.index) + 1;
      saveBlock(index);
    }
  }
}

function fillScope(pchar, char, nchar, index) {
  // } can break the code block
  if (char == '}') {
    commons.activeBlock.codeLength = (index - commons.activeBlock.codeIndex) + 1;
    saveBlock(index);

    return next(pchar, char, nchar, index);
  }

  return next(pchar, char, nchar, index, 1);
}

function saveWord(index, force_name) {
  if (commons.activeWord.length) {
    // see if it's empty space or word
    if (commons.activeWord.trim().length)
      newBlock(force_name ? force_name : "WORD", commons.activeWordIndex);
    else
      newBlock(' ', commons.activeWordIndex);

    commons.activeBlock.length = commons.activeWord.length;
    saveBlock(index);
    commons.activeWord = "";
  }
}

function next(pchar, char, nchar, index, skip_activeBlock) {
  if (commons.activeBlock && !skip_activeBlock) {
    // if it's a string block
    if (commons.activeBlock.type == types["'"]) {
      return fillString.apply(null, arguments);
    }

    // if it's a comment block
    if (commons.activeBlock.type == types["//"]) {
      return fillComment.apply(null, arguments);
    }

    // if it's a scope '{ }' block
    if (commons.activeBlock.type == types["{"]) {
      return fillScope.apply(null, arguments);
    }

    if (commons.activeBlock.type == "REGEX") {
      return fillRegex.apply(null, arguments);
    }

    commons.activeBlock.length = (index - commons.activeBlock.index);
    saveBlock(index);
  }

  // check comment block
  if (char == '/' && (nchar == '/' || nchar == '*')) {
    saveWord(index, types.hasOwnProperty(commons.activeWord) ? commons.activeWord : null);
    newBlock("/" + nchar, index);
    commons.activeWord = "";
    return;
  }

  switch(char) {
    case '{': {
      // if there is any word save it
      saveWord(index, types.hasOwnProperty(commons.activeWord) ? commons.activeWord : null);
      newBlock('{', index);
      commons.activeBlock.codeIndex = commons.activeBlock.index;
      commons.activeBlock.index = null; // for scope, function etc. don't hold the normal index
      return;
    }break;

    case "'":
    case '"': {
      if(commons.activeWord.length) {
        if (types.hasOwnProperty(commons.activeWord)) {
          saveWord(index, commons.activeWord);
        } else {
          return {
            type: "SyntaxError",
            msg: "Unexpected char(s) '" + commons.activeWord + "'",
            row: commons.rowIndex,
            column: commons.columnIndex - commons.activeWord.length
          };
        }
      }
      newBlock(char, index);
      return;
    } break;

    case "/": {
      if (types.hasOwnProperty(commons.activeWord)) {
        saveWord(index, commons.activeWord);
      }

      if (!commons.activeWord.trim().length) {
        var node = new Block();
        node.parent = commons.activeBlock ? commons.activeBlock : blocks;
        node.parentIndex = node.parent.subs.length;

        var bl = node.getPreviousBlock();

        if (!bl || (bl && bl.type != "WORD" && bl.type != "ARRAY_CLOSE" && (commons.prexp[bl.type] || bl.type == "PTS_CLOSE" ))) {
          var pass = bl ? commons.prexp[bl.type] : true;
          if (!pass) {
            if (bl.type == "PTS_CLOSE") {
              node = bl;
              // find if for while etc.
              //first find (
              do {
                bl = node.getPreviousBlock();
                if (bl)
                  node = bl;
              } while (bl && bl.type != "PTS_OPEN");

              if (!bl) {
                // something is wrong there is no (
                return {
                  type: "SyntaxError",
                  msg: "Couldn't locate the (",
                  column: commons.columnIndex,
                  row: commons.rowIndex
                };
              }

              bl = bl.getPreviousBlock();

              // see if we have if etc.
              if (commons.headlessScopes[bl.type]) {
                pass = true;
              }
            }
          }

          if (pass) {
            newBlock(char, index);
            commons.activeBlock.type = "REGEX";
            return;
          }
        }
      }

      if (!types.hasOwnProperty(char+nchar) && commons.activeWord.length) {
        // if there is any space save it
        saveWord(index, types.hasOwnProperty(commons.activeWord) ? commons.activeWord : null);
        newBlock(char, index);
        return;
      }
    } break;

    case '\\': {
      if (nchar != 'u') {
        return {
          type: "SyntaxError",
          msg: "Unexpected char '\\'",
          row: commons.rowIndex,
          column: commons.columnIndex
        };
      }
    } break;

    default:
  }

  if (types.hasOwnProperty(char)) {
    var word, tmp, tmp_index = 0;

    var hide_pc = 0;
    // skip to next char in case of != == etc. but not for ===
    if (types.hasOwnProperty(char+nchar) && !types.hasOwnProperty(pchar+char+nchar)) {
      if(!types.hasOwnProperty(pchar + char))
        return;
      else {
        hide_pc = 1;
      }
    }

    // now it's time to see if we have something i.e. ===
    if (types.hasOwnProperty(pchar+char+nchar)) {
      tmp = pchar+char+nchar;
      tmp_index = 1;
    } else if (types.hasOwnProperty(pchar + char)) { // how about == etc. ?
      tmp = pchar+char;
      tmp_index = 0;
    }

    word = commons.activeWord.trim();

    // otherwise save it as a word
    if (word.length) {
      // save the block if it's exist
      if (types.hasOwnProperty(word)) {
        newBlock(word, commons.activeWordIndex);
      } else {
        newBlock("WORD", commons.activeWordIndex);
      }
      commons.activeBlock.length = word.length;
      saveBlock(index);
      commons.activeWord = "";
    }

    // save the block if it's exist
    if (tmp && types.hasOwnProperty(tmp)) {
      newBlock(tmp, index-(tmp.length+1));
      saveBlock(index);
      return {move:tmp_index == 1 ? 1 : 0, hide_pc: hide_pc};
    }

    newBlock(char, (index - commons.activeWord.length));

    commons.activeBlock.length = commons.activeWord.length ? commons.activeWord.length : 1;
    saveBlock(index);
    commons.activeWord = '';
    return;
  }

  if (commons.activeWord.length == 0)
    commons.activeWordIndex = index;

  commons.activeWord += char;
}

exports.parse = function(filename, code) {
  commons.code = code + "";

  commons.rowIndex = 1;
  commons.columnIndex = 1;
  commons.activeWord = "";
  commons.activeWordIndex = 1;
  commons.activeBlock = null;
  commons.errorInfo = null;

  blocks = new Block();
  blocks.rowIndex = 0;
  blocks.columnIndex = 0;
  blocks.type = "ROOT";
  commons.blocks_ = blocks;

  var hide_pc = false;
  for (var i = 0, length = commons.code.length; i < length; i++) {
    var pc = i == 0 ? null : commons.code.charAt(i - 1);
    var nc = i < length - 1 ? commons.code.charAt(i + 1) : null;
    var ch = commons.code.charAt(i);

    // i.e. x++===y ++ and += might mix
    // if hide_pc is received deliver ~~ instead of +
    // on next call
    var err = next(hide_pc ? "~~" : pc, ch, nc, i+1);

    hide_pc = (err && err.hide_pc);

    if (pc != '\\') {
      if (ch == '\n') {
        commons.rowIndex++;
        commons.columnIndex = 1;
      } else {
        commons.columnIndex++;
      }
    }

    var move = err && err.move ? err.move : 0;

    if (commons.errorInfo) {
      err = commons.errorInfo;
      commons.errorInfo = null;
    }

    if (err && err.msg) {
      if (commons.activeBlock)
        saveBlock(i);

      throw new Error(err.type + ": " + err.msg + " at " + err.row + ":" + err.column + " (" + filename + ")");
    }

    i += move;
  }

  if (commons.activeBlock) {
    if (commons.activeBlock.type == "COMMENT" && commons.activeBlock.delimiter == "//") {
      commons.activeBlock.length = ((commons.code.length - 1) - commons.activeBlock.index);
      saveBlock(commons.activeBlock.index + commons.activeBlock.length);
    } else {
      require('./writer').printBlocks(blocks);
      throw new Error("Reached end of file. One or more scope(s) left open. See at " +
      ((commons.activeBlock.index || commons.activeBlock.codeIndex) + 1)
       + " or (" + commons.activeBlock.rowIndex + ":" + commons.activeBlock.columnIndex + ")");
    }
  }

  if (commons.activeWord.length) {
    saveWord(code.length - commons.activeWord.length, types.hasOwnProperty(commons.activeWord) ? commons.activeWord : null);
  }

  return blocks;
};

var writer = require('./writer');
exports.blockToCode = writer.blockToCode;
exports.printBlocks = writer.printBlocks;

exports.types = commons.types;
exports.signs = commons.signs;