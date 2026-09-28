local lu = require 'luaunit'
local fs = require 'llae.fs'
local path = require 'llae.path'
local netutils = require 'net.utils'
local zip = require 'archive.zip'

local FREERTOS_ZIP_URL =
  'https://github.com/FreeRTOS/FreeRTOS-Kernel/releases/download/V11.1.0/FreeRTOS-KernelV11.1.0.zip'
local FREERTOS_ZIP_NAME = 'FreeRTOS-KernelV11.1.0.zip'

testArchiveZip = {}

function testArchiveZip:setUp()
  self.test_dir = path.join(fs.pwd(), 'build', 'test_archive_zip')
  if fs.isdir(self.test_dir) then
    fs.rmdir_r(self.test_dir)
  end
  fs.mkdir_r(self.test_dir)
end

function testArchiveZip:tearDown()
  if self.test_dir and fs.isdir(self.test_dir) then
    fs.rmdir_r(self.test_dir)
  end
end

function testArchiveZip:test_unpack_freertos_kernel()
  local zip_path = path.join(self.test_dir, FREERTOS_ZIP_NAME)
  local status, ok, err = pcall(netutils.download_file, FREERTOS_ZIP_URL, zip_path)
  if not status then
    lu.skip('failed to download FreeRTOS-Kernel zip: ' .. tostring(ok))
  end
  if not ok then
    lu.skip('failed to download FreeRTOS-Kernel zip: ' .. tostring(err))
  end

  local dst = path.join(self.test_dir, 'out')
  fs.mkdir_r(dst)
  zip.unpack_zip(zip_path, dst)

  local root = path.join(dst, 'FreeRTOS-KernelV11.1.0')
  lu.assertTrue(fs.isfile(path.join(root, 'README.md')))
  -- Empty deflated entry that previously left compressed bytes unread.
  local empty_file = path.join(root, 'portable', 'MPLAB', 'PIC18F', 'stdio.h')
  lu.assertTrue(fs.isfile(empty_file))
  lu.assertEquals(#tostring(fs.load_file(empty_file)), 0)
end
