// Copyright & License details are available under JXCORE_LICENSE file

// This code renders html table with precompiled binaries download links,
// for each zip package present in jxcore/out_binaries
// (packages names are e.g.: jx_osx64v8.zip, jx_osx64sm.zip etc.
// Usage:
// $ cd jxcore
// $ ./jx tools/download_packages/render_html_table.js

var version = process.jxversion.toLowerCase().replace('beta', "").replace(/[-|\.|v]/g, "").trim();

var config = require('./config.json');


var fs = require("fs");
var path = require("path");

var out_dir = path.join(__dirname, "../../out_binaries");

var table = [];
table.push("<table>");
table.push("<tbody>");

var jxrun = function(cmd) {
  var ret = jxcore.utils.cmdSync(cmd);
  if (ret.exitCode)
    jxcore.utils.console.error(cmd + " error", ret.out);
  else
    return ret.out;
};

for(var o in config.builds) {
  var build = config.builds[o];

  // header
  var header = build.header || build.name;
  table.push('<tr>');
  table.push('<td colspan="3">&nbsp;<br><strong>' + header + '</strong></td>');
  table.push('</tr>');


  for (var a in build.files) {

    // single file
    var file = build.files[a];
    var arch = "";
    if (file.indexOf("32") !== -1) arch = "ia32";
    if (file.indexOf("ARM") !== -1) arch = "ARM";
    if (file.indexOf("64") !== -1) arch = "x64";
    if (file.indexOf("MIPS") !== -1) arch = "MIPS";
    var engine = "";
    if (file.indexOf("v8.") !== -1) engine = "(V8)";
    if (file.indexOf("sm.") !== -1) engine = "(SM)";


    var ret1 = jxrun("shasum " + path.join(out_dir, file));
    var sha1 = ret1 ? ret1.split(" ")[0] : '';

    var size = fs.statSync(path.join(out_dir, file)).size / 1024 / 1024;

    var name = build.name;
    if (build.props && build.props[file] && build.props[file].caption) name = build.props[file].caption;

    var _name = name;
    if (_name.indexOf(' ' + arch + ' ') === -1)
      _name += ' ' + arch + ' ';

    if (_name.indexOf(' ' + engine + ' ') === -1)
      _name += ' ' + engine + ' ';

    table.push('<tr>');
    table.push('<td style="padding-right: 15px">' + _name + '</td>');
    table.push('<td style="padding-right: 10px; text-align: right;">' + Number(size).toFixed(2) + ' MB</td>');
    table.push('<td><a href="https://jxcore.s3.amazonaws.com/' + version + '/' + file + '">Download</a></td>');
    table.push('<td>' + sha1 + '</td>');
    table.push('</tr>');
  }

  if (build.footNote) {
    table.push('<tr>');
    table.push('<td colspan="3"><div style="padding-top: 5px; padding-left: 15px;"><i>' + build.footNote + '</i></div></td>');
    table.push('</tr>');
  }
}

table.push("</tbody>");
table.push("</table>");

var str = table.join("\n");


var out_file = path.join(__dirname, "table.html");
fs.writeFileSync(out_file, str);

jxcore.utils.console.write("Table for version ", "green");
jxcore.utils.console.write(version, "red");
jxcore.utils.console.log(" saved to:", out_file.replace(process.cwd(), "."), "green");
