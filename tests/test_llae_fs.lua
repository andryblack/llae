local fs = require 'llae.fs'
local lu = require 'luaunit'
local path = require 'llae.path'

testFs = {}

function testFs:setUp()
  -- Create test directory structure
  self.test_dir = path.join(fs.pwd(), 'test_fs_tmp')
  fs.mkdir(self.test_dir)
  fs.write_file(path.join(self.test_dir, 'file1.txt'), 'test content 1')
  fs.write_file(path.join(self.test_dir, 'file2.txt'), 'test content 2')
  fs.mkdir(path.join(self.test_dir, 'subdir'))
  fs.write_file(path.join(self.test_dir, 'subdir', 'file3.txt'), 'test content 3')
end

function testFs:tearDown()
  -- Clean up test directory
  if fs.isdir(self.test_dir) then
    fs.rmdir_r(self.test_dir)
  end
end

function testFs:test_isfile()
  local file_path = path.join(self.test_dir, 'file1.txt')
  lu.assertTrue(fs.isfile(file_path))
  lu.assertFalse(fs.isfile(path.join(self.test_dir, 'subdir')))
  lu.assertFalse(fs.isfile(path.join(self.test_dir, 'nonexistent')))
end

function testFs:test_isdir()
  lu.assertTrue(fs.isdir(self.test_dir))
  lu.assertTrue(fs.isdir(path.join(self.test_dir, 'subdir')))
  lu.assertFalse(fs.isdir(path.join(self.test_dir, 'file1.txt')))
  lu.assertFalse(fs.isdir(path.join(self.test_dir, 'nonexistent')))
end

function testFs:test_rmdir_r()
  local test_nested = path.join(self.test_dir, 'nested')
  fs.mkdir(test_nested)
  fs.mkdir(path.join(test_nested, 'dir1'))
  fs.mkdir(path.join(test_nested, 'dir1', 'dir2'))
  fs.write_file(path.join(test_nested, 'dir1', 'file.txt'), 'test')
  
  lu.assertTrue(fs.isdir(test_nested))
  fs.rmdir_r(test_nested)
  lu.assertFalse(fs.isdir(test_nested))
end

function testFs:test_mkdir_r()
  local nested_dir = path.join(self.test_dir, 'a', 'b', 'c')
  fs.mkdir_r(nested_dir)
  lu.assertTrue(fs.isdir(nested_dir))
end

function testFs:test_load_file()
  local file_path = path.join(self.test_dir, 'file1.txt')
  local content = fs.load_file(file_path)
  lu.assertEquals(tostring(content), 'test content 1')
end

function testFs:test_write_file()
  local file_path = path.join(self.test_dir, 'new_file.txt')
  fs.write_file(file_path, 'new content')
  local content = fs.load_file(file_path)
  lu.assertEquals(tostring(content), 'new content')
end

function testFs:test_scanfiles_r()
  local files = fs.scanfiles_r(self.test_dir)
  table.sort(files)
  lu.assertEquals(files, {
    'file1.txt',
    'file2.txt',
    'subdir/file3.txt'
  })
end

function testFs:test_find_exe()
  -- Test with absolute path to a known executable
  local ls_path = fs.find_exe('ls')  -- 'ls' should exist on Unix systems
  lu.assertNotNil(ls_path)
  lu.assertTrue(fs.isfile(ls_path))
  
  -- Test with nonexistent executable
  lu.assertErrorMsgContains(
    'not found exe',
    function() fs.find_exe('nonexistent_executable_12345') end
  )
end

