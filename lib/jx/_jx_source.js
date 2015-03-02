// Copyright & License details are available under JXCORE_LICENSE file

if (process._EmbeddedSource) {
  var src = process.appBuffer;
  if (!src) {
    var $uw = process.binding('memory_wrap');
    src = $uw.readEmbeddedSource();
  } else {
    process.appBuffer = null;
    delete (process.appBuffer);
  }

  if (!src) {
    console.error("embedded source file wasn't exist");
    process.exit(1);
  }
  require("jx_source.jx", null, null, null, src);
}