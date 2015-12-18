#!/usr/bin/env python

import errno

try:
  import json
except ImportError:
  import simplejson as json

import os
import re
import shutil
import sys

# set at init time
dst_dir = None
node_prefix = None # dst_dir without DESTDIR prefix
target_defaults = None
variables = None

def abspath(*args):
  path = os.path.join(*args)
  return os.path.abspath(path)

def load_config():
  s = open('config.gypi').read()
  s = re.sub(r'#.*?\n', '', s) # strip comments
  s = re.sub(r'\'', '"', s) # convert quotes
  return json.loads(s)

def try_unlink(path):
  try:
    os.unlink(path)
  except OSError, e:
    if e.errno != errno.ENOENT: raise

def try_symlink(source_path, link_path):
  print 'symlinking %s -> %s' % (source_path, link_path)
  try_unlink(link_path)
  os.symlink(source_path, link_path)

def try_mkdir_r(path):
  try:
    os.makedirs(path)
  except OSError, e:
    if e.errno != errno.EEXIST: raise

def try_rmdir_r(path):
  path = abspath(path)
  while path.startswith(dst_dir):
    try:
      os.rmdir(path)
    except OSError, e:
      if e.errno == errno.ENOTEMPTY: return
      if e.errno == errno.ENOENT: return
      raise
    path = abspath(path, '..')

def mkpaths(path, dst):
  if dst.endswith('/'):
    target_path = abspath(dst_dir, dst, os.path.basename(path))
  else:
    target_path = abspath(dst_dir, dst)
  return path, target_path

def try_copy(path, dst):
  source_path, target_path = mkpaths(path, dst)
  print 'installing %s' % target_path
  try_mkdir_r(os.path.dirname(target_path))
  try_unlink(target_path) # prevent ETXTBSY errors
  return shutil.copy2(source_path, target_path)

def try_remove(path, dst):
  source_path, target_path = mkpaths(path, dst)
  print 'removing %s' % target_path
  try_unlink(target_path)
  try_rmdir_r(os.path.dirname(target_path))

def install(paths, dst): map(lambda path: try_copy(path, dst), paths)
def uninstall(paths, dst): map(lambda path: try_remove(path, dst), paths)

def update_shebang(path, shebang):
  print 'updating shebang of %s to %s' % (path, shebang)
  s = open(path, 'r').read()
  s = re.sub(r'#!.*\n', '#!' + shebang + '\n', s)
  open(path, 'w').write(s)

def subdir_files(path, dest, action):
  ret = {}
  for dirpath, dirnames, filenames in os.walk(path):
    files = [dirpath + '/' + f for f in filenames if f.endswith('.h')]
    ret[dest + dirpath.replace(path, '')] = files
  for subdir, files in ret.items():
    action(files, subdir + '/')

def files(action):
  
  if variables.get('node_static_library') == 0 and variables.get('node_shared_library') == 0:
    action(['out/Release/jx'], 'bin/jx')
    action(['src/node.d'], 'lib/dtrace/')
  elif variables.get('node_shared_library') == 1:
    if variables.get('SHARED_OS') == 'darwin':
      action(['out/Release/libjx.dylib'], 'bin/')
    else:
      action(['out/Release/lib.target/libjx.so'], 'bin/')
  else:
    action([
      'out/Release/libjx.a',
      'out/Release/libcares.a',
      'out/Release/libchrome_zlib.a',
      'out/Release/libhttp_parser.a',
      'out/Release/libopenssl.a',
      'out/Release/libuv.a',
      'out/Release/libsqlite3.a',
    ], 'bin/')
    if variables.get('node_engine_mozilla') != 0:
      action(['out/Release/libmozjs.a'], 'bin/')
      action(['deps/mozjs/src/js.msg'], 'include/node/')
    else:
      action([
        'out/Release/libv8_base.a',
        'out/Release/libv8_nosnapshot.a'
        ], 'bin/')
      

  if 'freebsd' in sys.platform or 'openbsd' in sys.platform:
    action(['doc/node.1'], 'man/man1/')
  else:
    action(['doc/node.1'], 'share/man/man1/')

  action([
    'common.gypi',
    'config.gypi',
    'src/node.h',
    'src/jxcore.h',
    'src/wrappers/node_buffer.h',
    'src/node_internals.h',
    'src/node_version.h',
  ], 'include/node/')

  if variables.get('node_engine_mozilla') != 0:
    action([
      'src/jx/Proxy/Mozilla_340/node_object_wrap.h'
    ], 'include/node/jx/Proxy/Mozilla_340/')
  else:
    action([
      'src/jx/Proxy/V8_3_14/node_object_wrap.h'
    ], 'include/node/jx/Proxy/V8_3_14/')

  subdir_files('src/', 'include/node/', action)
  
  subdir_files('deps/http_parser', 'include/node/', action)

  if 'false' == variables.get('node_shared_cares'):
    subdir_files('deps/cares/include', 'include/node/', action)

  if 'false' == variables.get('node_shared_libuv'):
    subdir_files('deps/uv/include', 'include/node/', action)

  if 'false' == variables.get('node_shared_openssl'):
    subdir_files('deps/openssl/openssl/include/openssl', 'include/node/openssl/', action)
    action(['deps/openssl/config/opensslconf.h'], 'include/node/openssl/')

  if variables.get('node_engine_mozilla') == 0: #ios target
    if 'false' == variables.get('node_shared_v8'):
      subdir_files('deps/v8_3_14/include', 'include/node/', action)
  else:
    subdir_files('deps/mozjs/src', 'include/node/', action)
    subdir_files('deps/mozjs/incs', 'include/node/', action)

  if 'false' == variables.get('node_shared_zlib'):
    action([
      'deps/zlib/zconf.h',
      'deps/zlib/zlib.h',
    ], 'include/node/')

def run(args):
  global dst_dir, node_prefix, target_defaults, variables

  # chdir to the project's top-level directory
  os.chdir(abspath(os.path.dirname(__file__), '..'))

  conf = load_config()
  variables = conf['variables']
  target_defaults = conf['target_defaults']

  # argv[2] is a custom install prefix for packagers (think DESTDIR)
  dst_dir = node_prefix = variables.get('node_prefix') or '/usr/local'

  cmd = args[1] if len(args) > 1 else 'install'
  if cmd == 'install': return files(install)
  if cmd == 'uninstall': return files(uninstall)
  raise RuntimeError('Bad command: %s\n' % cmd)

if __name__ == '__main__':
  run(sys.argv[:])
