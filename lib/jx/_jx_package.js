// Copyright & License details are available under JXCORE_LICENSE file

var checkOff, add;
var nativePackageMarker = 'jxcore.bin(?@' + '@!!$<$?!*)';

var util = require('util');

function stripBOM(content) {
  if (content.charCodeAt(0) === 0xFEFF) {
    content = content.slice(1);
  }
  return content;
}

exports.getPatchMatcher = function(parms) {
  var path = require('path');
  var fs = require('fs');

  var ret = {
    regexes : [],
    test : function() {
    }
  };

  if (!util.isArray(parms) || !parms.length)
    return ret;

  var getRelative = function(par) {
    var relative = path.normalize(par).replace(cwd, '');
    while (relative.slice(0, 1) === path.sep)
      relative = relative.slice(1);

    return relative;
  };

  // without '*' and '?'
  var specials = [ '\\', '^', '$', '.', '|', '+', '(', ')', '[', ']', '{', '}' ];
  var cwd = process.cwd();

  var addRegexp = function(par) {
    var r = par.trim();
    var dot = (r.slice(0, 2) === './');
    var r = path.normalize(r); // cuts ./ as well

    r = getRelative(r);

    for ( var i in specials) {
      if (specials.hasOwnProperty(i))
        r = r.replace(new RegExp('\\' + specials[i], 'g'), '\\' + specials[i]);
    }

    r = r.replace(/\*/g, '.*').replace(/\?/g, '.{1,1}');

    var sep = process.platform === 'win32' ? '\\\\' : '/';

    if (dot) {
      ret.regexes.push(new RegExp('^' + r + '(' + sep + '.*)?' + '$'));
    } else {
      ret.regexes.push(new RegExp('^' + '(.*' + sep + ')?' + r + '(' + sep
          + '.*)?' + '$'));
    }
  };

  for ( var o in parms) {
    if (!parms.hasOwnProperty(o))
      continue;

    addRegexp(parms[o]);
  }

  ret.test = function(file_path) {

    if (!ret.regexes || !ret.regexes.length)
      return false;

    for ( var o in ret.regexes) {
      if (!ret.regexes.hasOwnProperty(o))
        continue;

      var regex = ret.regexes[o];
      if (regex.test(getRelative(file_path)))
        return true;
    }
  };

  return ret;
};

var getPatchMatcherByName = function(name) {
  var parsedArgv = jxcore.utils.argv.parse();
  var parms = jxcore.utils.argv.getValue(name);

  // allow -add without a value
  if (name === 'add' && !parms && parsedArgv[name])
    parms = true;

  if (!parms)
    return null;

  if (typeof parms !== 'string')
    return {
      test : function() {
      }
    };

  parms = _splitBySep(parms);
  return exports.getPatchMatcher(parms);
};

var getFiles = function(folder, startup_path, jxp) {
  var fz = {
    f : [],
    a : []
  };
  var path = require('path');
  var fs = require('fs');

  var mainPath = process.cwd();
  var onFirst = false;
  if (!folder) {
    onFirst = true;
    folder = process.cwd() + path.sep;
  }

  // if startup_path is relative:
  if (startup_path && startup_path.indexOf(mainPath) === -1)
    startup_path = path.join(mainPath, startup_path);

  if (checkOff === undefined)
    checkOff = getPatchMatcherByName('slim');

  if (add === undefined)
    add = getPatchMatcherByName('add');

  var dest_bin = path.join(process.cwd(), jxp.name);
  var dest_bin_native = dest_bin + (process.platform === 'win32' ? '.exe' : '');
  var dest_bin_jx = dest_bin + '.jx';
  if (process.platform === 'win32') {
    dest_bin_native = dest_bin_native.toLocaleLowerCase();
    dest_bin_jx = dest_bin_jx.toLocaleLowerCase();
  }

  var files = fs.readdirSync(folder);
  for ( var o in files) {
    if (!files.hasOwnProperty(o))
      continue;

    var file = files[o];
    var file_path = path.join(folder, file);

    var _file_path = process.platform === 'win32' ? file_path
        .toLocaleLowerCase() : file_path;

    if (_file_path === dest_bin_jx || _file_path === dest_bin_native) {
      jxcore.utils.console.write('skipping existing package', 'magenta');
      jxcore.utils.console.log(' ' + path.basename(file_path));
      continue;
    }

    if (checkOff && checkOff.test(file_path))
      continue;

    var stat = fs.statSync(file_path);

    if (add && startup_path !== file_path && stat.isFile()) {
      if (!add.test(file_path))
        continue;
    }

    if (stat.isDirectory()) {
      if (file.indexOf('.') != 0) {
        var az = getFiles(file_path, startup_path, jxp);
        fz.f = fz.f.concat(az.f);
        fz.a = fz.a.concat(az.a);
      }
      continue;
    }

    var ext = path.extname(file);
    var ufile = file.toUpperCase();

    if (ext == '.js' || ext == '.json' || ext == '.jx') {
      fz.f.push(path.relative(mainPath, file_path));
    } else if (onFirst && (ufile == 'LICENSE-MIT' || ufile == 'LICENSE')) {
      fz.license = path.relative(mainPath, file_path);
      fz.a.push(path.relative(mainPath, file_path));
    } else if (onFirst && (ufile == 'README' || ufile == 'README.MD')) {
      fz.readme = path.relative(mainPath, file_path);
      fz.a.push(path.relative(mainPath, file_path));
    } else {
      fz.a.push(path.relative(mainPath, file_path));
    }
  }
  return fz;
};

var nameFix = function(a) {
  if (!a)
    return '';

  var isw = process.platform === 'win32';
  var repFrom = isw ? /[\/]/g : /[\\]/g;
  var repTo = isw ? '\\' : '/';

  return a.replace(repFrom, repTo);
};

// returns argv[3], or if not provided: basename(argv[2)
var getPackageName = function(argv) {
  var name = null;
  if (argv.length >= 4)
    name = argv[3];
  if (!name || name.slice(0, 1) === '-') {

    var parsedArgv = jxcore.utils.argv.parse();
    if (parsedArgv.name && parsedArgv.name.hasValue
        && parsedArgv.name.value.length)
      return parsedArgv.name.value;

    name = argv[2];
    if (name.slice(-3).toLowerCase() === '.js') {
      var path = require('path');
      name = path.basename(name.slice(0, name.length - 3));
    }
  }
  return name;
};

var binaryFileIndexOf = function(file_name, testString, writeDotAt) {
  var fs = require('fs');

  if (!fs.existsSync(file_name))
    throw new Error('Cannot find file for string search: ' + file_name);

  var testStringLen = testString.length;
  var loc = 0;
  var sz = fs.statSync(file_name);
  var fd = fs.openSync(file_name, 'r');
  var buffer = null;
  var chunkSize = 1024 * 8; // the bigger, the faster
  var cnt = 0;
  var ret = -1;

  var chars = [];
  for (var o = 0; o < testStringLen; o++)
    chars.push(testString.charCodeAt(o));

  while (true) {
    if (writeDotAt && loc % writeDotAt == 0)
      process.stdout.write('.');

    cnt++;
    buffer = new Buffer(chunkSize);
    fs.readSync(fd, buffer, 0, chunkSize, loc);

    // this is much faster for SM (and V8 too) than iterating through buffer
    // chars!
    var str = buffer.toString();

    var id = str.indexOf(testString);
    if (id !== -1) {
      // now we know for sure, that the chunk contains the string
      // let's do precise check char by char instead of by string.indexOf()
      // this block should occur just once per entire string occurrence in
      // entire file,
      // so there is no performance penalty here, if the searched string is
      // unique

      for (var o = 0, len = buffer.length; o < len; o++) {
        if (buffer[o] === chars[0]) {

          var foundCnt = 1;
          for (var a = 1; a < testStringLen; a++) {
            if (buffer[o + a] === chars[a])
              foundCnt++;
            else
              break;
          }

          if (foundCnt === testStringLen) {
            ret = loc + o;
            break;
          }
        }
      }
    }

    if (ret !== -1 || loc > sz.size)
      break;

    // make sure you don't skip the string if chunk contains only part of it
    loc += (chunkSize - testStringLen - 1);
  }

  buffer = null;
  fs.closeSync(fd);
  fd = null;

  return ret;
};

var _splitBySep = function(val) {
  if (!val)
    return null;

  var arr = null;
  if (typeof val === 'string')
    arr = val.split(jxcore.utils.argv.sep);
  else if (util.isArray(val))
    arr = val;
  else
    arr = [ val.toString() ];

  var ret = [];
  // copying non-empty parts
  for (var a = 0, len = arr.length; a < len; a++)
    if (arr[a].trim())
      ret.push(arr[a]);

  return ret;
};

var extractOptions = null;

var getExtractOptions = function() {
  if (extractOptions)
    return extractOptions;

  var parsedArgv = jxcore.utils.argv.parse();
  var ret = false;

  if (parsedArgv.extract) {
    if (parsedArgv.extract.isBool && !parsedArgv.extract.asBool)
      return false;

    // default true -extract
    ret = true;
  }

  var _add = function(name, value) {
    if (typeof ret === 'boolean')
      ret = {};
    ret[name] = value;
  };

  var separatedParams = [ 'what', 'pre-actions', 'post-actions' ];
  for (var a = 0, len = separatedParams.length; a < len; a++) {
    var p = separatedParams[a];
    var name = 'extract-' + p;
    if (parsedArgv[name]) {
      if (typeof ret === 'boolean')
        ret = {};
      ret[p] = parsedArgv[name].splitBySep();
    }
  }

  var otherSupportedParams = [ 'where', 'message' ];
  for (var a = 0, len = otherSupportedParams.length; a < len; a++) {
    var p = otherSupportedParams[a];
    var name = 'extract-' + p;
    if (parsedArgv[name]) {
      if (typeof ret === 'boolean')
        ret = {};

      try {
        // if cannot be parsed (like array, or obj)
        ret[p] = JSON.parse(parsedArgv[name].value);
      } catch (ex) {
        // just use as a string
        ret[p] = parsedArgv[name].value;
      }
    }
  }

  if (parsedArgv['extract-app-root']) {
    if (typeof ret === 'boolean')
      ret = {};
    ret['where'] = './';
  }

  if (jxcore.utils.argv.getBoolValue('extract-verbose', false)) {
    if (typeof ret === 'boolean')
      ret = {};
    ret['verbose'] = true;
  }

  if (jxcore.utils.argv.getBoolValue('extract-overwrite', false)) {
    if (typeof ret === 'boolean')
      ret = {};
    ret['overwrite'] = true;
  }

  if (parsedArgv['extract-chmod'])
    _add('chmod', jxcore.utils.argv.getBoolValue('extract-chmod'));

  ret = exports.normalizeExtractOptions(ret, parsedArgv.native);

  extractOptions = ret;
  return ret;
};

exports.normalizeExtractOptions = function(ret, isNative) {
  // extract disabled
  if (!ret)
    return false;

  if (ret !== true && typeof ret !== 'object') {
    jxcore.utils.console.error('Unsupported value for `-extract` option:\n',
        ret);
    process.exit(1);
  }

  // "__dirname" and "./" are aliases
  if (ret['where'] === '__dirname')
    ret['where'] = './';

  var strWhereDir = 'This is because the content should be extracted into package\'s '
      + 'location folder rather than a sub directory.';
  var isExtract = !!ret;
  var isPartialExtract = isExtract && ret['what'];
  var isAppRoot = ret['where'] === './';

  if (ret && isNative && !isAppRoot) {
    jxcore.utils.console
        .error('Native packages `--extract` option is supported only with '
            + '`--extract-app-root` added.');
    jxcore.utils.console.warn(strWhereDir);
    process.exit(1);
  }

  if (isPartialExtract) {
    if (!isAppRoot) {
      jxcore.utils.console
          .error('Partial extract (`--extract-what`) is supported only with '
              + '`--extract-app-root` added.');
      jxcore.utils.console.warn(strWhereDir);
      process.exit(1);
    }
    ret['what'] = _splitBySep(ret['what']);
  }

  // `true` is default, so use `true` if `false` is not defined explicitly
  ret['chmod'] = !jxcore.utils.argv.isBoolValue(ret['chmod']).isFalse;

  return ret;
};

var getShowProgressOption = function() {
  var parsedArgv = jxcore.utils.argv.parse();
  var showProgress = 'list'; // list (default), line, percent, none
  if (parsedArgv['show-progress']) {
    var sp = parsedArgv['show-progress'];
    if (sp.isBool) {
      showProgress = sp.asBool ? 'list' : 'none';
    } else {
      if (sp.value === 'line' || sp.value === 'none' || sp.value === 'percent')
        showProgress = sp.value;
    }
  }

  return showProgress;
};

// receives bool or command string
var getSignOptions = function() {
  var parsedArgv = jxcore.utils.argv.parse();

  if (!parsedArgv.sign)
    return false;

  var sign = parsedArgv.sign;
  if (sign.isBool)
    return sign.asBool;

  if (!sign.hasValue)
    return true;

  return sign.value;
};

var signFile = function(signOptions, fileName) {
  if (process.platform !== 'win32' || !signOptions)
    return;

  var fss = require('fs');

  var _cmd = '';
  var lc = signOptions.toString().toLowerCase();
  if (lc === 'true') {
    _cmd = '/a';
  } else if (fss.existsSync(signOptions)) {
    _cmd = '/f "' + signOptions + '"';
  } else {
    _cmd = signOptions;
  }

  if (_cmd) {
    _cmd = 'signtool sign ' + _cmd.trim() + ' "' + fileName + '"';
    var ret = jxcore.utils.cmdSync(_cmd);
    if (ret.exitCode) {
      jxcore.utils.console.log('\nUnable to sign the native package:',
          'magenta');
      jxcore.utils.console.log(ret.out, 'red');
    } else {
      jxcore.utils.console.log('\nThe package was successfully signed.',
          'yellow');
    }
  }
};

exports.compile = function(argv, ops) {
  var contents = {
    pack : {},
    project : {},
    docs : {}
  };

  var console = jxcore.utils.console;
  var path = require('path');
  var fss = require('fs');

  var fn = path.resolve(argv[2]);
  var ext = '.';
  if (fn.length > 4) {
    ext = fn.substr(fn.length - 4, 4);
  }

  if (ext != '.jxp') {
    console.error('unknown JX project type "' + fn + '"', 'red');
    process.exit(1);
    return;
  }

  var proj = null;
  try {
    var xt = '' + fss.readFileSync(fn);
    xt = xt.trim();
    proj = JSON.parse(stripBOM(xt));
  } catch (e) {
    console.error(e);
    process.exit(1);
    return;
  }

  if (jxcore.utils.argv.getBoolValue('native')) {
    proj.native = true;
  }

  if (!proj) {
    console.error('corrupted JSON in jxp file', 'red');
    process.exit(1);
    return;
  }

  if (!fss.existsSync(path.join(process.cwd(), proj.startup))) {
    console.error('Project startup file does not exist:', proj.startup, 'red');
    process.exit(1);
  }

  proj.startup = './' + proj.startup;
  var startup_extension = path.extname(proj.startup);
  if (startup_extension.toLowerCase() != '.js') {
    console.error('Project startup file must have a .js extension.', 'red');
    process.exit(1);
  }

  if (!proj.files || !proj.files.length) {
    console.log('no target source file definition inside the j' + 'xp', 'red');
    process.exit(1);
    return;
  }

  if (proj.name && proj.version && proj.output) {
    var str = 'Compiling ' + proj.name + ' ' + proj.version;
    console.log(str, 'green');
  } else {
    console.error(
        '\'name\', \'version\' and \'output\' fields must be defined inside the '
            + 'JXP file', 'red');
    process.exit(1);
    return;
  }

  contents.project = proj;

  proj = null;
  var _package = process.cwd() + path.sep + 'package.json';
  var pext = fss.existsSync(_package);
  if (contents.project['package'] || pext) {
    if (!pext)
      fn = path.resolve(contents.project['package']);
    else {
      contents.project['package'] = _package;
      fs = _package;
    }

    try {
      var x = '' + fss.readFileSync(fn);
      proj = JSON.parse(x);
    } catch (e) {
      console.error(e);
      process.exit();
      return;
    }

    if (!proj) {
      console.error('corrupted JSON in \'' + fn + '\' file', red);
      process.exit(1);
      return;
    }

    contents.pack = proj;
    if (!contents.project.website && (proj.website || proj.homepage)) {
      contents.project.website = proj.website ? proj.website : proj.homepage;
    }

    if (proj.bin && !contents.project.execute) {
      var set = false;
      if (proj.bin.substr) {
        contents.project.execute = proj.bin;
        set = true;
      } else if (proj.bin[0]) {
        proj.bin = proj.bin[0];
      }
      if (!set) {
        if (proj.bin.substr) {
          contents.project.execute = proj.bin;
          set = true;
        } else {
          for ( var o in proj.bin) {
            if (o && proj[o]) {
              contents.project.execute = proj[o];
              set = true;
              break;
            }
          }
        }
      }

      if (set) {
        contents.project.extract = true;
      }
    }

    if (proj.version) {
      contents.project.version = proj.version;
    }

    if (proj.description) {
      contents.project.description = proj.description;
    }

    if (proj.author) {
      contents.project.author = proj.author;
    }
  }

  proj = contents.project;
  var str_dup = JSON.stringify(proj);
  var strobj = JSON.parse(str_dup);
  delete (strobj.files);
  delete (strobj.readme_file);
  delete (strobj.license_file);

  contents.PROS = 'exports.$JXP=' + JSON.stringify(strobj) + ';';
  contents.stats = {};
  contents.dirStats = {};

  if (proj.fs_reach_sources && proj.fs_reach_sources !== true) {
    var arr = {};
    for ( var o in proj.fs_reach_sources) {
      arr['./' + nameFix(o)] = true;
    }
    proj.fs_reach_sources = arr;
  }

  var scomp = process.binding('jxutils_wrap');
  var cw = process.stdout.columns;
  // it may show 0, e.g. on android
  if (!cw) {
    var _ret = jxcore.utils.cmdSync('echo $COLUMNS');
    if (!_ret.exitCode) {
      cw = parseInt(_ret.out);
      if (isNaN(cw))
        cw = 0;
    }
  }

  var showProgress = getShowProgressOption();
  var progressDisplayStarted = false;
  var cursorUp = '\033[1A', clearLine = '\033[K';
  var total = proj.files.length + proj.assets.length;
  var done = 0;
  var lastPercent = 0;

  var displayProgress = function(caption, loc) {
    if (showProgress === 'none')
      return;

    var dx = caption.length + 7; // 4 dots and 3 spaces
    var loc_str = (loc.length < cw - dx) ? loc : '....'
        + loc.substr(loc.length - (cw - dx));

    var str = caption + ' ' + loc_str;

    if (showProgress === 'line') {
      if (progressDisplayStarted)
        str = cursorUp + clearLine + str;
    } else if (showProgress === 'percent') {
      done++;
      var percent = (done * 100 / total).toFixed(0);
      if (percent === lastPercent)
        return;
      str = caption + ' ' + percent + ' %';
      lastPercent = percent;
      if (progressDisplayStarted)
        str = cursorUp + clearLine + str;
    }

    console.log(str);
    if (!progressDisplayStarted)
      progressDisplayStarted = true;
  };

  // legacy means old packaging method using base64
  var getFileAndStats = function(filename, legacy) {

    var ret = {
      doc : null,
      stat : null
    };
    var ext = path.extname(filename).toLowerCase();

    try {
      var fileBuf = fss.readFileSync(filename);
      ret.stat = JSON.stringify(fss.statSync(filename));

      if (!legacy) {

        var com = scomp._cmp(fileBuf);
        fss.appendFileSync(fbuf_name, com);
        ret.doc = {
          start : fbuf_len,
          len : com.length
        };
        fbuf_len += com.length;
        com = null;
      } else {

        if (ext != '.js')
          ret.doc = scomp._cmp(fileBuf.toString('base64')).toString('base64');
        else
          ret.doc = scomp._cmp(stripBOM(fileBuf + '')).toString('base64');
      }
      fileBuf = null;
    } catch (e) {
      if (!ret.doc)
        jxcore.utils.console.error('Cannot read file', filename);
      if (!ret.stat)
        jxcore.utils.console.error('Cannot read stat', filename);
      jxcore.utils.console.error(e);
      process.exit(1);
      return false;
    }

    return ret;
  };

  var setDirStat = function(ploc) {

    if (!ploc || ploc === './' || ploc === '.' || contents.dirStats[ploc])
      return;

    var dir = path.resolve(ploc);
    var dirStat = fss.statSync(dir);

    // store only mode number for directories instead of entire stat object
    // (reducing package size)
    contents.dirStats[ploc] = dirStat.mode;

    setDirStat(path.dirname(ploc));
  };

  var caption = jxcore.utils.console.setColor('adding script', 'yellow');

  var extractOptions = exports.normalizeExtractOptions(proj.extract, proj.native);
  var extract_what = null;
  if (extractOptions && extractOptions['what']) {
    extract_what = extractOptions['what'];
    if (util.isArray(extract_what))
      extract_what = require('_jx_package').getPatchMatcher(
        extractOptions['what']);
  }

  var legacyPackaging = jxcore.utils.argv.parse().legacy;
  var fbuf_name = path.join(process.cwd(), "tmpbuf9475628djhf");
  var fbuf_len = 0;
  if (fss.existsSync(fbuf_name))
    fss.unlinkSync(fbuf_name);

  var ln_files = proj.files.length;
  for (var i = 0; i < ln_files; i++) {
    var loc = proj.files[i].trim();
    loc = nameFix(loc);
    var fn_sub = path.resolve(loc);
    displayProgress(caption, loc);
    loc = './' + loc;

    var ext = path.extname(loc).toLowerCase();
    if (ext != '.json' && ext != '.js') {
      console.log('only "js" or "json" files can be defined as a source code. '
          + '(json and js are case sensitive)', 'red');
      process.exit(1);
    }

    var _ret = getFileAndStats(fn_sub, legacyPackaging);
    if (!_ret)
      return;

    contents.docs[loc] = _ret.doc;
    contents.stats[loc] = _ret.stat;

    if (process.platform !== 'win32' && extractOptions.chmod)
      setDirStat(path.dirname(loc));

    _ret = null;
  }

  jxcore.tasks.forceGC();
  var warn_node = null;

  if (proj.assets) {
    var ln = proj.assets.length;
    var caption = jxcore.utils.console.setColor('adding asset', 'yellow');
    var sizeCheck = 0;
    for (var i = 0; i < ln; i++) {
      var loc = proj.assets[i].trim();
      loc = nameFix(loc);
      var fn = path.resolve(loc);
      displayProgress(caption, loc);

      if (path.extname(loc) == '.node') {
        var matches = extractOptions
            && (!extract_what || (extract_what && extract_what.test && extract_what
                .test(loc)));
        if (!matches)
          warn_node = loc;
      }

      loc = './' + loc;

      var _ret = getFileAndStats(fn, legacyPackaging);
      if (!_ret)
        return;

      contents.docs[loc] = _ret.doc;
      contents.stats[loc] = _ret.stat;

      if (process.platform !== 'win32' && extractOptions.chmod)
        setDirStat(path.dirname(loc));

      _ret = null;

      sizeCheck += contents.docs[loc].length;
      if (i % 50 === 0 || sizeCheck > 1e6) {
        sizeCheck = 0;
        jxcore.tasks.forceGC();
      }
    }
  }

  jxcore.tasks.forceGC();

  if (warn_node) {
    if (warn_node.length > 35)
      warn_node = '....' + warn_node.substr(warn_node.length - 35);
    console.log('Warning!', warn_node, 'red');
    console.log(
        'Adding a .node (native) file into package may fail the application',
        'red');
    jxcore.utils.console.log('Check the related discussion from',
        'https://github.com/jxcore/jxcore/issues/101', 'blue');

    jxcore.utils.console
        .info('However you may mark .node files as extractable, e.g.: '
            + "--extract-what '*.node'");
  }

  var c = JSON.stringify(contents);
  var cmped = scomp._cmp(c);

  c = null;
  jxcore.tasks.forceGC();

  var op = getPackageName(process.argv);
  if (!op || (ops && ops.compile)) {
    op = contents.project.output;
  }

  if (op.length > 3) {
    var last = op.substr(op.length - 3);
    if (last.toLowerCase() == '.jx') {
      op = op.substr(0, op.length - 3);
    }
  }

  if (!legacyPackaging)
    cmped = Buffer.concat([ cmped, fss.readFileSync(fbuf_name),
        exports.numberToBuffer(cmped.length),
        exports.numberToBuffer(cmped.length + 12345) ]);

  if (fss.existsSync(fbuf_name))
    fss.unlinkSync(fbuf_name);

  var cc = jxcore.utils.console.log;
  if (contents.project.native) {
    var os_info = jxcore.utils.OSInfo();
    var cmd_sync = jxcore.utils.cmdSync, ret, copy = os_info.isWindows ? 'copy /Y'
        : 'cp', copy_ext = os_info.isWindows ? '.exe' : '';

    var replaceStrings = function(file_name) {
      // replacing strings inside binary
      // called at one of two different positions in the code
      var index1 = binaryFileIndexOf(file_name, 'bin@ry.v@rsio' + 'n@', 32768);
      var index2 = binaryFileIndexOf(file_name, 'jxcore.bi' + 'n(?@@', 32768);
      if (index1 === -1 || index2 === -1) {
        cc('\nUnable to compile. Make sure the JX binary is not corrupted',
            'red');
        process.exit(1);
        return false;
      }
      var sz = fss.statSync(file_name).size;
      var fd = fss.openSync(file_name, 'r+');
      // first string
      var buffer = new Buffer(
          parseInt((5 * (sz - cmped.length - 32)) - 123456789) + '')
          .toString('hex');
      buffer += ')';
      for (var o = buffer.length; o < 42; o++) {
        buffer += parseInt(Math.random() * 9) + '';
      }
      buffer = new Buffer(new Buffer('bin@ry.v@rsion'
          + buffer.replace(/[d]/g, '*').replace(/[0]/g, '#').replace(/[1]/g,
              '$').replace(/[2]/g, '@').replace(/[3]/g, '!').replace(/[4]/g,
              '(').replace(/[5]/g, '{').replace(/[6]/g, '?').replace(/[7]/g,
              '<').replace(/[8]/g, ']').replace(/[9]/g, '|')));
      fss.writeSync(fd, buffer, 0, 56, index1);
      // second string
      buffer = new Buffer(nativePackageMarker);
      fss.writeSync(fd, buffer, 0, 23, index2);
      fss.closeSync(fd);
      jxcore.tasks.forceGC();
      return true;
    };

    if (true) { // make jslint happy
      var file_name = process.cwd() + path.sep + op;
      file_name = nameFix(file_name);
      var op_str = '';
      if (os_info.isWindows)
        op_str = 'del "' + file_name + copy_ext + '"';
      else
        op_str = process.platform === 'android' ? 'rm -r ' + file_name
            : 'rm -rf ' + file_name;

      cmd_sync(op_str);
      if (fss.existsSync(file_name + copy_ext)) {
        cc('Target file in use', file_name + copy_ext, 'red');
        process.exit(1);
      }
      op_str = copy + ' "' + process.execPath + '" "' + file_name + '"';
      ret = cmd_sync(op_str);
      if (ret.exitCode != 0) {

        // on android there might be an error when copying the file with system
        // command:
        // cp: can't stat 'jx': Value too large for defined data type
        // so we try to copy one more time by using fs module:
        fss.writeFileSync(file_name, fss.readFileSync(process.execPath));
        if (!fss.existsSync(file_name)) {
          cc(ret.out, 'red');
          process.exit(1);
        }

        // copy chmod value
        try {
          var stat = fss.statSync(process.execPath);
          var octal_chmod = '0' + (stat.mode & parseInt('777', 8)).toString(8);
          fss.chmod(file_name, octal_chmod);
        } catch (ex) {
          cc('Cannot set file mode with chmod:', ex.toString(), 'red');
          process.exit(1);
        }
      }

      if (!fss.existsSync(file_name)) {
        cc("Couldn't access to JX binary file or write into current folder. "
            + 'This is an unexpected error though but you may check the '
            + 'permissions for JX binary file(s) or the current folder', 'red');
        process.exit(1);
      }

      // Windows process changes the file more, a string is used to index
      // location
      // of data.
      // Non-windows can replace the strings here. Windows string replace
      // process
      // must wait.
      var sz = fss.statSync(file_name);
      var fd = fss.openSync(file_name, 'a');
      fss.writeSync(fd, cmped, 99, 8, sz.size);
      fss.writeSync(fd, cmped, 44, 8, sz.size + 8);
      fss.writeSync(fd, cmped, 77, 8, sz.size + 16);
      fss.writeSync(fd, cmped, 22, 8, sz.size + 24);
      fss.writeSync(fd, new Buffer([ // New method for indexing data
      0, 0, 0, 0, 0, 0, 0, // Unused (for now?)
      cmped.length >> 24 & 255, cmped.length >> 16 & 255,
          cmped.length >> 8 & 255, cmped.length & 255, 0, 0, 0, 0, // No
          // offset,
          // used for
          // later
          // file
          // modification
          // if
          // necessary
          0xff ]), 0, 16, sz.size + 32);
      fss.writeSync(fd, cmped, 0, cmped.length, sz.size + 48);
      fss.writeSync(fd, cmped, 33, 16, sz.size + cmped.length + 48);
      fss.closeSync(fd);
      if (!os_info.isWindows && !replaceStrings(file_name))
        return;
    }

    if (os_info.isWindows) {
      var verpatch_name = 'jx.exe.ver';
      var verpatch = process.execPath + '.ver';
      if (!fss.existsSync(verpatch))
        verpatch = path.join(path.dirname(process.execPath), verpatch_name);

      if (!fss.existsSync(verpatch))
        verpatch = path.join(process.cwd(), verpatch_name);

      if (!fss.existsSync(verpatch)) {
        // search in the path
        var _where = jxcore.utils.cmdSync('where ' + verpatch_name);
        if (!_where.exitCode) {
          // there can be multiple files. take the first one
          var _arr = _where.out.trim().split('\n');
          verpatch = null;
          for (var o = 0, len = _arr.length; o < len; o++) {
            var _test = _arr[o].trim();
            if (_test && fss.existsSync(_test)) {
              verpatch = _test;
              break;
            }
          }
        } else {
          verpatch = null;
        }
      }

      var quote = function(str) {
        return '"' + str + '"';
      };

      fss.renameSync(file_name, file_name + copy_ext);

      var error = false;
      if (verpatch) {
        // copy file
        var verpatch_local = file_name + '._.exe';
        fss.writeFileSync(verpatch_local, fss.readFileSync(verpatch));

        if (fss.existsSync(verpatch_local)) {
          var run_win = quote(verpatch_local) + ' /va '
              + quote(file_name + copy_ext) + ' '
              + quote(contents.project.version + ' (%date%)') + ' /s desc '
              + quote(contents.project.description)
              + ' /s pb "Powered by JXcore"' + ' /s company '
              + quote(contents.project.company) + ' /s product '
              + quote(contents.project.name) + ' /s (c) '
              + quote(contents.project.copyright || contents.project.company)
              + ' /pv ' + quote(contents.project.version);

          var batch_name = file_name + '_jx_mark.bat';
          fss.writeFileSync(batch_name, run_win);
          var ret = jxcore.utils.cmdSync(batch_name);
          if (ret.exitCode)
            error = 'Error while executing command. ' + ret.out;

          jxcore.utils.cmdSync('del ' + quote(batch_name));
          jxcore.utils.cmdSync('del ' + quote(verpatch_local));
        } else {
          error = 'The `.ver` file could not be copied';
        }
      }

      if (!verpatch || error) {
        cc('');
        if (!verpatch)
          cc('Cannot apply file description information. '
              + "The '.ver' file not found.", 'magenta');
        if (error)
          cc('Cannot apply file description information.', error, 'magenta');
      }

      if (!replaceStrings(file_name + copy_ext))
        return;

      signFile(contents.project.sign, file_name + copy_ext);
    }

    cc('\n[OK] compiled file is ready (' + file_name + copy_ext + ')',
        !warn_node ? 'green' : '');
  } else {
    fss.writeFileSync(op + '.jx', cmped);
    cc('[OK] compiled file is ready (' + contents.project.output + ')',
        !warn_node ? 'green' : '');
  }
  process.exit(0);
};

exports.package = function(argv) {
  var path = require('path');
  var console = require('console');
  var fs = require('fs');

  var executer = null;
  var sss = argv[2].split('|');
  if (sss.length > 1)
    executer = sss[1];
  var fol = sss[0];
  fol = (path.relative(process.cwd(), fol));

  if (!fs.existsSync(path.join(process.cwd(), fol))) {
    jxcore.utils.console.error('Project startup file does not exist:', fol,
        'red');
    process.exit(1);
  }

  var startup_extension = path.extname(fol);
  if (startup_extension.toLowerCase() != '.js') {
    jxcore.utils.console.log('Project startup file must have a .js extension.',
        'red');
    process.exit(1);
  }

  var parsedArgv = jxcore.utils.argv.parse();
  var preinstall = parsedArgv['preInstall'] || parsedArgv['preinstall'];
  var jxp = {
    'name' : getPackageName(argv),
    'version' : jxcore.utils.argv.getValue('version', '1.0'),
    'author' : jxcore.utils.argv.getValue('author', ''),
    'description' : jxcore.utils.argv.getValue('description', ''),
    'company' : jxcore.utils.argv.getValue('company', ''),
    'copyright' : jxcore.utils.argv.getValue('copyright', ''),
    'website' : jxcore.utils.argv.getValue('website', ''),
    'package' : null,
    'startup' : fol,
    'execute' : executer,
    'extract' : getExtractOptions(),
    'output' : null,
    'files' : [],
    'assets' : [],
    'preInstall' : preinstall ? preinstall.splitBySep() || null : null,
    'library' : jxcore.utils.argv.getBoolValue('library', true),
    'license_file' : null,
    'readme_file' : null,
    'fs_reach_sources' : jxcore.utils.argv.getBoolValue('fs_reach_sources',
        true)
  };

  if (parsedArgv.native)
    jxp.native = true;

  var sign = getSignOptions();
  if (sign)
    jxp.sign = sign;

  try {
    // add only startup file
    var fz = {
      f : [ fol ],
      a : []
    };
    var addOnlyStartupFile = parsedArgv.add
        && (!parsedArgv.add.hasValue || parsedArgv.add.asBool);
    if (!addOnlyStartupFile)
      fz = getFiles(null, fol, jxp);

    jxp.files = fz.f;
    jxp.assets = fz.a;
    jxp.license_file = fz.license;
    jxp.readme_file = fz.readme;

    var fin = jxp.name + '.jxp';
    jxp.output = jxp.name + '.jx';

    fs.writeFileSync(fin, JSON.stringify(jxp, null, '\t'));

    console.log('JXP project file (' + fin + ') is ready.');
    console.log('');
    console.log('preparing the JX file..');
    exports.compile([ '', 'jx', fin ]);
  } catch (e) {
    console.log(e);
  }
};

exports.isNativePackage = function(file_name) {
  var id = -1;
  try {
    id = binaryFileIndexOf(file_name, nativePackageMarker);
  } catch (ex) {
    return false;
  }

  return id !== -1;
};

exports.numberToBuffer = function(number) {

  return new Buffer([ number >> 32, number >> 24, number >> 16, number >> 8, ]);
};

// reads 4 bytes
exports.bufferToNumber = function(buf, offset) {

  if (!offset)
    offset = 0;

  var ret = (buf[offset + 0] << 32) + (buf[offset + 1] << 24)
      + (buf[offset + 2] << 16) + (buf[offset + 3] << 8);

  return ret;
};