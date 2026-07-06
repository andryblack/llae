local lu = require('luaunit')
local fs = require('llae.fs')
local path = require('llae.path')

local store_path = package.path
package.path = store_path .. ';tools/?.lua'
local isolate = require('cparse.isolate')
package.path = store_path

TestIsolate = {}

local function make_temp_dir()
  local dir = path.join(fs.pwd(), 'build', 'tests', 'isolate_tmp')
  if fs.isdir(dir) then
    fs.rmdir_r(dir)
  end
  fs.mkdir_r(dir)
  return dir
end

local function cleanup_dir(dir)
  if fs.isdir(dir) then
    fs.rmdir_r(dir)
  end
end

local function write_file(filepath, content)
  fs.mkdir_r(path.dirname(filepath))
  fs.write_file(filepath, content)
end

local function make_instance()
  return isolate.new()
end

function TestIsolate:setUp()
  self.tmp = make_temp_dir()
  self.root = path.join(self.tmp, 'project')
  fs.mkdir_r(self.root)
end

function TestIsolate:tearDown()
  cleanup_dir(self.tmp)
end

function TestIsolate:test_build_index()
  write_file(path.join(self.root, 'build/include/llae-private/mbedtls/ssl.h'), '')
  write_file(path.join(self.root, 'build/include/llae-private/psa/crypto.h'), '')

  local header_dir = path.join(self.root, 'build/include/llae-private')
  local inst = make_instance()
  local index = inst:build_index({
    [header_dir] = 'llae-private/',
  })

  lu.assertEquals(index['mbedtls/ssl.h'], 'llae-private/mbedtls/ssl.h')
  lu.assertEquals(index['psa/crypto.h'], 'llae-private/psa/crypto.h')
end

function TestIsolate:test_build_index_nested()
  write_file(path.join(self.root, 'build/include/llae-private/uv/linux.h'), '')

  local header_dir = path.join(self.root, 'build/include/llae-private')
  local inst = make_instance()
  local index = inst:build_index({
    [header_dir] = 'llae-private/',
  })

  lu.assertEquals(index['uv/linux.h'], 'llae-private/uv/linux.h')
end

function TestIsolate:test_rewrite_spaced_include()
  local inst = make_instance()
  inst._index = { ['uv/aix.h'] = 'llae-private/uv/aix.h' }
  inst._prefixes = { 'llae-private/' }
  local line = '# include "uv/aix.h"'
  lu.assertEquals(inst:rewrite_line(line), '# include "llae-private/uv/aix.h"')
end

function TestIsolate:test_rewrite_quoted_include()
  local inst = make_instance()
  inst._index = { ['mbedtls/ssl.h'] = 'llae-private/mbedtls/ssl.h' }
  inst._prefixes = { 'llae-private/' }
  local line = '#include "mbedtls/ssl.h"'
  lu.assertEquals(inst:rewrite_line(line), '#include "llae-private/mbedtls/ssl.h"')
end

function TestIsolate:test_rewrite_angle_include()
  local inst = make_instance()
  inst._index = { ['mbedtls/ssl.h'] = 'llae-private/mbedtls/ssl.h' }
  inst._prefixes = { 'llae-private/' }
  local line = '#include <mbedtls/ssl.h>'
  lu.assertEquals(inst:rewrite_line(line), '#include "llae-private/mbedtls/ssl.h"')
end

function TestIsolate:test_skip_unknown_include()
  local inst = make_instance()
  inst._index = { ['mbedtls/ssl.h'] = 'llae-private/mbedtls/ssl.h' }
  inst._prefixes = { 'llae-private/' }
  local line = '#include "other.h"'
  lu.assertEquals(inst:rewrite_line(line), line)
end

function TestIsolate:test_skip_already_isolated()
  local inst = make_instance()
  inst._index = { ['mbedtls/ssl.h'] = 'llae-private/mbedtls/ssl.h' }
  inst._prefixes = { 'llae-private/' }
  local line = '#include "llae-private/mbedtls/ssl.h"'
  lu.assertEquals(inst:rewrite_line(line), line)
end

function TestIsolate:test_skip_comment_lines()
  local inst = make_instance()
  inst._index = { ['mbedtls/ssl.h'] = 'llae-private/mbedtls/ssl.h' }
  local line1 = '// #include "mbedtls/ssl.h"'
  local line2 = '/* #include "mbedtls/ssl.h" */'
  lu.assertEquals(inst:rewrite_line(line1), line1)
  lu.assertEquals(inst:rewrite_line(line2), line2)
end

function TestIsolate:test_rewrite_text()
  local inst = make_instance()
  inst._index = { ['mbedtls/ssl.h'] = 'llae-private/mbedtls/ssl.h' }
  inst._prefixes = { 'llae-private/' }
  local content = '#include "mbedtls/ssl.h"\nstatic int x;\n'
  local new_content, changed = inst:rewrite_text(content)
  lu.assertTrue(changed)
  lu.assertEquals(new_content, '#include "llae-private/mbedtls/ssl.h"\nstatic int x;\n')
end

function TestIsolate:test_match_glob()
  lu.assertTrue(isolate.match_glob('foo.c', '*.c'))
  lu.assertFalse(isolate.match_glob('foo.h', '*.c'))
  lu.assertTrue(isolate.match_glob('library/foo.c', 'library/*.c'))
  lu.assertFalse(isolate.match_glob('other/foo.c', 'library/*.c'))
end

function TestIsolate:test_scan()
  local src_dir = path.join(self.root, 'library')
  write_file(path.join(src_dir, 'ssl.c'), '')
  write_file(path.join(src_dir, 'ssl.h'), '')
  write_file(path.join(src_dir, 'nested', 'other.c'), '')

  local inst = make_instance()
  local files = inst:scan(src_dir, { '*.c', 'nested/*.c' })
  table.sort(files)
  lu.assertEquals(files, { 'nested/other.c', 'ssl.c' })
end

function TestIsolate:test_run_inplace()
  write_file(path.join(self.root, 'build/include/llae-private/mbedtls/ssl.h'), '')
  local src_dir = path.join(self.root, 'library')
  write_file(path.join(src_dir, 'ssl.c'), '#include "mbedtls/ssl.h"\n')
  write_file(path.join(src_dir, 'ssl.h'), '#include "mbedtls/ssl.h"\n')

  isolate.run(src_dir, { '*.c' }, {
    [path.join(self.root, 'build/include/llae-private')] = 'llae-private/',
  })

  lu.assertEquals(tostring(fs.load_file(path.join(src_dir, 'ssl.c'))),
    '#include "llae-private/mbedtls/ssl.h"\n')
  lu.assertEquals(tostring(fs.load_file(path.join(src_dir, 'ssl.h'))),
    '#include "mbedtls/ssl.h"\n')
end

function TestIsolate:test_run_nested_include()
  write_file(path.join(self.root, 'build/include/llae-private/uv/linux.h'), '')
  local src_dir = path.join(self.root, 'src')
  write_file(path.join(src_dir, 'unix.c'), '#include "uv/linux.h"\n')

  isolate.run(src_dir, { '*.c' }, {
    [path.join(self.root, 'build/include/llae-private')] = 'llae-private/',
  })

  lu.assertEquals(tostring(fs.load_file(path.join(src_dir, 'unix.c'))),
    '#include "llae-private/uv/linux.h"\n')
end

function TestIsolate:test_run_no_change_when_not_matched()
  write_file(path.join(self.root, 'build/include/llae-private/mbedtls/ssl.h'), '')
  local src_dir = path.join(self.root, 'library')
  local source = '#include "other.h"\n'
  write_file(path.join(src_dir, 'other.c'), source)

  isolate.run(src_dir, { '*.c' }, {
    [path.join(self.root, 'build/include/llae-private')] = 'llae-private/',
  })

  lu.assertEquals(tostring(fs.load_file(path.join(src_dir, 'other.c'))), source)
end
