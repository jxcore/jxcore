// Copyright & License details are available under JXCORE_LICENSE & LICENSE files

var fs = require('fs');
var marked = require('marked');
var path = require('path');

module.exports = toHTML;

function toHTML(input, filename, template, cb) {
  input = input.replace(/\.markdown/g, ".html");
  var lexed = marked.lexer(input);
  fs.readFile(template, 'utf8', function(er, template) {
    if (er) return cb(er);
    render(lexed, filename, template, cb);
  });
}

function render(lexed, filename, template, cb) {
  // get the section
  var section = getSection(lexed);

  filename = path.basename(filename, '.markdown');

  lexed = parseLists(lexed);

  // generate the table of contents.
  // this mutates the lexed contents in-place.
  buildToc(lexed, filename, function(er, toc) {
    if (er) return cb(er);

    template = template.replace(/__FILENAME__/g, filename);
    template = template.replace(/__SECTION__/g, section);
    template = template.replace(/__VERSION__/g, process.version);
    template = template.replace(/__JXVERSION__/g, process.jxversion);
    template = template.replace(/__YEAR__/g, new Date().getFullYear());

    if (filename === "index") {
      // hide TOC for index.html
      template = template.replace(/__TOC__/g, "");
      template = template.replace('div id="toc"', 'div id="toc" style="display: none;"');
    } else {
      template = template.replace(/__TOC__/g, toc);
    }


    // content has to be the last thing we do with
    // the lexed tokens, because it's destructive.
    content = marked.parser(lexed);
    template = template.replace(/__CONTENT__/g, content);

    cb(null, template);
  });
}

// just update the list item text in-place.
// lists that come right after a heading are what we're after.
function parseLists(input) {
  var state = null;
  var depth = 0;
  var output = [];
  output.links = input.links;
  input.forEach(function(tok) {
    if (tok.type === 'code' && tok.text.match(/Stability:.*/g)) {
      tok.text = parseAPIHeader(tok.text);
      output.push({
        type: 'html',
        text: tok.text
      });
      return;
    }
    if (state === null) {
      if (tok.type === 'heading') {
        state = 'AFTERHEADING';
      }
      output.push(tok);
      return;
    }
    if (state === 'AFTERHEADING') {
      if (tok.type === 'list_start') {
        state = 'LIST';
        if (depth === 0) {
          output.push({
            type: 'html',
            text: '<div class="signature">'
          });
        }
        depth++;
        output.push(tok);
        return;
      }
      state = null;
      output.push(tok);
      return;
    }
    if (state === 'LIST') {
      if (tok.type === 'list_start') {
        depth++;
        output.push(tok);
        return;
      }
      if (tok.type === 'list_end') {
        depth--;
        if (depth === 0) {
          state = null;
          output.push({
            type: 'html',
            text: '</div>'
          });
        }
        output.push(tok);
        return;
      }
      if (tok.text) {
        tok.text = parseListItem(tok.text);
      }
    }
    output.push(tok);
  });

  if (input.links) {
    for(var o in input.links) {
      if (input.links[o].href)
        input.links[o].href = input.links[o].href.replace(/\.markdown/g, ".html");
    }
  }

  return output;
}

function parseListItem(text) {
  text = text.replace(/\{([^\}]+)\}/, '<span class="type">$1</span>');
  // XXX maybe put more stuff here?
  return text;
}

function parseAPIHeader(text) {
  text = text.replace(/(.*:)\s(\d)([\s\S]*)/,
          '<pre class="api_stability_$2">$1 $2$3</pre>');
  return text;
}

// section is just the first heading
function getSection(lexed) {
  var section = '';
  for (var i = 0, l = lexed.length; i < l; i++) {
    var tok = lexed[i];
    if (tok.type === 'heading') return tok.text;
  }
  return '';
}

function buildToc(lexed, filename, cb) {
  var indent = 0;
  var toc = [];
  var depth = 0;
  lexed.forEach(function(tok) {
    if (tok.type !== 'heading') return;
    if (tok.depth - depth > 1) { return cb(new Error(
            'Inappropriate heading level\n' + JSON.stringify(tok))); }

    depth = tok.depth;
    // original from node team:
    // var id = getId(filename + '_' + tok.text.trim());
    var id = getId(tok.text.trim());
    toc.push(new Array((depth - 1) * 2 + 1).join(' ') + '* <a href="#' + id
            + '">' + tok.text + '</a>');
    tok.text += '<span><a class="mark" href="#' + id + '" ' + 'id="' + id
            + '">#</a></span>';
  });

  toc = marked.parse(toc.join('\n'));
  cb(null, toc);
}

var idCounters = {};
function getId(text) {
  text = text.toLowerCase();
  // original from node team:
  // text = text.replace(/[^a-z0-9]+/g, '_');
  // text = text.replace(/^_+|_+$/, '');
  // text = text.replace(/^([^a-z])/, '_$1');

  // same as github does
  text = text.replace(/[^a-z0-9\s]+/g, '');
  text = text.replace(/\s/g, '-');

  if (idCounters.hasOwnProperty(text)) {
    text += '_' + (++idCounters[text]);
  } else {
    idCounters[text] = 0;
  }
  return text;
}