local path = require 'llae.path'
local lu = require 'luaunit'

testPath = {}

function testPath:test_isabsolute()
	-- Unix paths
	lu.assertTrue(path.isabsolute('/path'))
	lu.assertFalse(path.isabsolute('path'))
	-- Windows paths (using forward slashes)
	lu.assertTrue(path.isabsolute('C:/Windows/System32'))  -- Windows disk path
	lu.assertTrue(path.isabsolute('/Windows/System32'))    -- Windows UNC path
	-- Additional Windows cases
	lu.assertTrue(path.isabsolute('D:/'))  -- Just drive root
	lu.assertFalse(path.isabsolute('Windows/System32'))  -- Relative Windows path
end

function testPath:test_join()
	lu.assertEquals(path.join('a', 'b', 'c'), 'a/b/c')
	lu.assertEquals(path.join('a'), 'a')
	lu.assertEquals(path.join('', 'b'), '/b')
	-- Windows paths
	lu.assertEquals(path.join('C:', 'Windows', 'System32'), 'C:/Windows/System32')
	lu.assertEquals(path.join('Users', 'Administrator', 'Desktop'), 'Users/Administrator/Desktop')
end

function testPath:test_basename()
	lu.assertEquals(path.basename('/path/to/file.txt'), 'file.txt')
	lu.assertEquals(path.basename('file.txt'), 'file.txt')
	lu.assertEquals(path.basename('/path/to/dir/'), '')
	lu.assertEquals(path.basename('path\\to\\file.txt'), 'file.txt')
	-- Windows paths
	lu.assertEquals(path.basename('C:\\Windows\\System32\\cmd.exe'), 'cmd.exe')
	lu.assertEquals(path.basename('C:\\Program Files\\'), '')
	lu.assertEquals(path.basename('D:\\Games\\Steam\\steam.exe'), 'steam.exe')
end

function testPath:test_dirname()
	lu.assertEquals(path.dirname('/path/to/file.txt'), '/path/to')
	lu.assertEquals(path.dirname('file.txt'), '')
	lu.assertEquals(path.dirname('/path/to/dir/'), '/path/to/dir')
	lu.assertEquals(path.dirname('/'), '/')
	lu.assertEquals(path.dirname('path\\to\\file.txt'), 'path\\to')
	-- Windows paths
	lu.assertEquals(path.dirname('C:\\Windows\\System32\\cmd.exe'), 'C:\\Windows\\System32')
	lu.assertEquals(path.dirname('C:\\Program Files\\'), 'C:\\Program Files')
	lu.assertEquals(path.dirname('D:\\Games\\Steam\\steam.exe'), 'D:\\Games\\Steam')
end

function testPath:test_extension()
	lu.assertEquals(path.extension('file.txt'), 'txt')
	lu.assertEquals(path.extension('/path/to/file.lua'), 'lua')
	lu.assertEquals(path.extension('noextension'), nil)
	lu.assertEquals(path.extension('multiple.dots.in.file.txt'), 'txt')
	-- Windows paths
	lu.assertEquals(path.extension('C:\\Windows\\System32\\cmd.exe'), 'exe')
	lu.assertEquals(path.extension('C:\\Program Files\\myapp.dll'), 'dll')
	lu.assertEquals(path.extension('D:\\folder\\script.bat'), 'bat')
end

function testPath:test_getabsolute()
	-- Note: These tests assume the current working directory
	local fs = require 'llae.fs'
	local pwd = fs.pwd()
	
	lu.assertEquals(path.getabsolute('/absolute/path'), '/absolute/path')
	lu.assertEquals(path.getabsolute('relative/path'), pwd .. '/relative/path')
	lu.assertEquals(path.getabsolute('file.txt'), pwd .. '/file.txt')
	-- Windows paths - note that backslashes are preserved
	lu.assertEquals(path.getabsolute('folder\\subfolder'), pwd .. '/folder\\subfolder')
end

function testPath:test_getrelative()
	local fs = require 'llae.fs'
	local pwd = fs.pwd()
	
	lu.assertEquals(path.getrelative('already/relative'), 'already/relative')
	lu.assertEquals(path.getrelative(pwd .. '/some/path'), 'some/path')
	lu.assertEquals(path.getrelative('/different/root/path'), '/different/root/path')
	-- Windows paths
	lu.assertEquals(path.getrelative('already\\relative'), 'already\\relative')
	lu.assertEquals(path.getrelative('C:\\Windows\\System32'), 'C:\\Windows\\System32')
end

function testPath:test_remove_leading_dirs()
	lu.assertEquals(path.remove_leading_dirs('a/b/c/d', 2), 'c/d')
	lu.assertEquals(path.remove_leading_dirs('a/b/c', 1), 'b/c')
	lu.assertEquals(path.remove_leading_dirs('a/b', 3), nil)
	lu.assertEquals(path.remove_leading_dirs('a', 1), nil)
	-- Windows paths - using forward slashes
	lu.assertEquals(path.remove_leading_dirs('C:/Windows/System32/drivers', 2), 'System32/drivers')
	lu.assertEquals(path.remove_leading_dirs('Users/Administrator/Desktop', 1), 'Administrator/Desktop')
end