// Copyright & License details are available under JXCORE_LICENSE file

// This code renders html table with precompiled binaries download links,
// for each zip package present in jxcore/out_binaries
// usage:
// $ cd jxcore
// $ ./jx tools/download_packages/render_html_table.js

var version = process.jxversion.toLowerCase().replace('beta', "").replace(/[-|\.|v]/g, "").trim();

var builds = [
  {
    header: "Android 4+",
    name: "Android",
    files: ["jx_android32sm.zip", "jx_android32v8.zip", "jx_androidARMsm.zip", "jx_androidARMv8.zip", "jx_androidFATsm.zip", "jx_androidFATv8.zip" ],
    props: {
      "jx_androidFATsm.zip" : { caption: "Android FAT" },
      "jx_androidFATv8.zip" : { caption: "Android FAT" }
    }
  },
  {
    name: "FreeBSD",
    files: ["jx_bsd964sm.zip", "jx_bsd964v8.zip", "jx_bsd1064sm.zip", "jx_bsd1064v8.zip"]
  },
  {
    name: "Debian",
    files: ["jx_deb32sm.zip", "jx_deb32v8.zip", "jx_deb64sm.zip", "jx_deb64v8.zip", "jx_debARMsm.zip", "jx_debARMv8.zip", "jx_debMIPSsm.zip", "jx_debMIPSv8.zip"]
  },
  {
    name: "OSX INTEL",
    files: ["jx_osx64sm.zip", "jx_osx64v8.zip",]
  },
  {
    name: "iOS",
    files: ["jx_iosFATsm.zip"],
    props: {
      "jx_iosFATsm.zip" : { caption: "iOS FAT" }
    }
  },
  {
    name: "RH/Centos/Fedora",
    files: ["jx_rh64sm.zip", "jx_rh64v8.zip",]
  },
  {
    name: "Suse",
    files: ["jx_suse64sm.zip", "jx_suse64v8.zip",]
  },
  {
    name: "Ubuntu/Mint",
    files: ["jx_ub32sm.zip", "jx_ub32v8.zip", "jx_ub64sm.zip", "jx_ub64v8.zip"]
  },
  {
    name: "Windows",
    files: ["jx_win32sm.zip", "jx_win32v8.zip", "jx_win64sm.zip", "jx_win64v8.zip", "jx_winsetup.zip"],
    props: {
      "jx_winsetup.zip": {caption: "Windows Setup (32/64/SM/V8)"}
    },
    footNote : "Windows XP/2003 and below is NOT supported."
  },
];


var fs = require("fs");
var path = require("path");

var out_dir = path.join(__dirname, "../../out_binaries");

var table = [];
table.push("<table>");
table.push("<tbody>");

for(var o in builds) {
  var build = builds[o];

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

    var shasum = "";
    var ret = jxcore.utils.cmdSync("shasum " + path.join(out_dir, file));
    if (ret.exitCode)
      jxcore.utils.console.error("Shasum error", ret.out);
    else
      shasum = ret.out.split(" ")[0];

    var size = fs.statSync(path.join(out_dir, file)).size / 1024 / 1024;

    var name = build.name;
    if (build.props && build.props[file] && build.props[file].caption) name = build.props[file].caption;

    table.push('<tr>');
    table.push('<td style="padding-right: 15px">' + name + " " + arch + " " + engine + '</td>');
    table.push('<td style="padding-right: 10px; text-align: right;">' + Number(size).toFixed(2) + ' MB</td>');
    table.push('<td><a href="https://jxcore.s3.amazonaws.com/' + version + '/' + file + '">Download</a></td>');
    table.push('<td>' + shasum + '</td>');
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
