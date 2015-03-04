// Copyright & License details are available under JXCORE_LICENSE file

// assets are read properly by combining process.cwd();
process.chdir(__dirname);

// we run jx package from `_auto_jxcore_package` folder
// but we want to compare the asset content with real file located at `jxcore`,
// so we cut the "_package" part from dirname
var dirname = exports.$JXP ? __dirname.replace("_package", '').replace("_native", '').replace("_auto_", "").replace("_single", "") : __dirname;

var jx = require('jxtools');
var assert = jx.assert;
var fs = require("fs");
var path = require("path");

var assetContents = "";
var ondata = false;
var onend = false;
var flushed = false;

var realContents = fs.readFileSync(path.join(dirname, "_asset_file.txt")).toString();

var streamModule = require('stream');
var src = new streamModule.Transform();

src._transform = function (buff, enc, next) {
  this.push(buff);
  next();
};

src._flush = function () {
  flushed = true;
  this.push(null);
};


var stream = fs.createReadStream("./_asset_file.txt").pipe(src);

stream.on('error', function (err) {
  throw "Error while creating read stream from pipe:\n" + err;
});

stream.on('data', function (chunk) {
  ondata = true;
  assetContents += chunk.toString();
});

stream.on('end', function () {
  onend = true;
  assert.strictEqual(assetContents, realContents, "Content of asset file and real file are not equal.\nfrom asset: \t`" + assetContents + "`\nfrom file: \t`" + realContents + "`");
  assert.strictEqual(assetContents.length, realContents.length, "String length of contents of asset file and real file are not equal.\nfrom asset: \t`" + assetContents.length + "`\nfrom file: \t`" + realContents.length + "`");
});


process.on('exit', function () {
  assert.strictEqual(ondata, true, "stream.on('data') event was not fired.");
  assert.strictEqual(onend, true, "stream.on('end') event was not fired for.");
  assert.strictEqual(flushed, true, "The _flush() method was not called.");
});