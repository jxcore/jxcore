// Copyright & License details are available under JXCORE_LICENSE file

var fs = require('fs');

var arr = [];
var targetFile, outFile, name;
var args = process.argv;
var clog = jxcore.utils.console.log;
var bit_skip = 0;

for (var i = 0; i < args.length; i++) {
  if (args[i].indexOf('--file=') >= 0) {
    targetFile = args[i].replace("--file=", "");
  }
  
  if (args[i].indexOf('--out=') >= 0) {
    outFile = args[i].replace("--out=", "");
  }
  
  if (args[i].indexOf('--name=') >= 0) {
    name = args[i].replace("--name=", "");
  }

  if (args[i].indexOf('--bit-skip=') >= 0) {
    bit_skip = parseInt(args[i].replace("--bit-skip=", ""));
  }
}

if (!name) {
  clog("set namespace using --name=", "red");
  process.exit(1);
}

if (name.indexOf(" ")>=0) {
  clog("name shouldn't contain empty space. reminder: 'name' corresponds to namespace in c++", "red");
  process.exit(1);
}

if (!targetFile) {
  clog("set source file using --file=", "red");
  process.exit(1);
}

if (!outFile) {
  clog("set output file using --out=", "red");
  process.exit(1);
}

if (!bit_skip || bit_skip < 2 || bit_skip > 8) {
  clog("bit skip offset (2 to 8 --> --bit_skip=) defaulting to '2'", "blue");
  bit_skip = 2;
}


if (!fs.existsSync(targetFile)) {
  clog("couldn't find file", targetFile, "red");
  process.exit(1);
}

var fdata = fs.readFileSync(targetFile);
var data_length = fdata.length;
var starters = ["$", "_", "l", "i"];
var rname = (Date.now() % 255) << 4;
var source = "";
var t = 0, n = 0, max_bit = 255 << (bit_skip + 2), bpos = bit_skip > 3 ? bit_skip - 2 : bit_skip;

rname = starters[rname % 4] + rname;

for (var i = 0; i < data_length; i++) {
  if (i) {
    source += ", ";

    if (i % 20 == 0) {
      source += "\n    ";
    }
  }

  source += fdata[i] << (bit_skip + 2);
  t++;

  n++;
  if (n && n % bpos == 0) {
    source += ", " + parseInt(Math.random() * max_bit);
    t++;
  }
}

fs.writeFileSync( outFile, 
  "#ifndef ENC_LOGIC_HEADER\n" +
  "#define ENC_LOGIC_HEADER\n" +
  "#define ENC_BIT_SKIP " + bit_skip + "\n" +
  "#define ENC_DATA_LENGTH " + t + "\n" +
  "#define REAL_DATA_LENGTH " + data_length + "\n" +
  "#define ENC_SKIP_POS " + bpos + "\n" +
  "namespace "+name+" {\n" +
  "uint16_t " + rname + "[ENC_DATA_LENGTH] = {\n    " + source + "\n};\n\n" +
  "class cs {\n" +
  " public:\n" +
  "  static char* fn() {\n" +
  "    uint16_t* data = " + rname + ";\n" +
  "    char *ret = (char*) malloc(sizeof(char) * (REAL_DATA_LENGTH+1));\n" +
  "    if (!ret) return NULL;\n" +
  "    int n = 0, lpos = 0;\n" +
  "    for(int i = 0; i<ENC_DATA_LENGTH; i++) {\n" +
  "      if (n++ == ENC_SKIP_POS) {\n" +
  "        n = 0;\n" +
  "       continue;\n" +
  "      }\n" +
  "      uint16_t uch = *(data+i);\n" +
  "      char ch = uch >> (ENC_BIT_SKIP + 2);\n" +
  "      *(ret + lpos) = ch;\n" +
  "      lpos++;\n" +
  "    }\n" +
  "    *(ret + lpos) = 0;\n" +
  "    return ret;\n" +
  "  }\n" +
  "};\n" +
  "}\n#endif\n");

clog("header file is created (" + outFile + ")", "green");