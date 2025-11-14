local lu = require 'luaunit'
local llae = require 'llae'

TestWritableBuffer = {}

function TestWritableBuffer:test_alloc()
  -- Test allocating empty buffer
  local buf = llae.writable_buffer.alloc(0)
  lu.assertNotNil(buf)
  lu.assertEquals(buf:get_len(), 0)
  
  -- Test allocating buffer with size
  buf = llae.writable_buffer.alloc(10)
  lu.assertNotNil(buf)
  lu.assertEquals(buf:get_len(), 10)
  
  -- Test allocating large buffer
  buf = llae.writable_buffer.alloc(1024)
  lu.assertNotNil(buf)
  lu.assertEquals(buf:get_len(), 1024)
end

function TestWritableBuffer:test_new()
  -- Test creating from empty string
  local buf = llae.writable_buffer.new("")
  lu.assertNotNil(buf)
  lu.assertEquals(buf:get_len(), 0)
  
  -- Test creating from string
  buf = llae.writable_buffer.new("hello")
  lu.assertNotNil(buf)
  lu.assertEquals(buf:get_len(), 5)
  lu.assertEquals(tostring(buf), "hello")
  
  -- Test creating from binary data
  local data = string.char(0x00, 0x01, 0x02, 0xFF)
  buf = llae.writable_buffer.new(data)
  lu.assertNotNil(buf)
  lu.assertEquals(buf:get_len(), 4)
end

function TestWritableBuffer:test_write_basic()
  -- Test writing at beginning
  local buf = llae.writable_buffer.alloc(10)
  local result = buf:write(1, "hello")
  lu.assertTrue(result)
  lu.assertEquals(buf:sub(1, 5), "hello")
  
  -- Test writing in middle
  buf = llae.writable_buffer.new("1234567890")
  result = buf:write(4, "XX")
  lu.assertTrue(result)
  lu.assertEquals(buf:sub(1, 10), "123XX67890")
  
  -- Test writing at end (offset + data length must not exceed buffer length)
  buf = llae.writable_buffer.new("1234567890")
  result = buf:write(9, "XX")
  lu.assertTrue(result)
  lu.assertEquals(buf:sub(1, 10), "12345678XX")
  
  -- Test writing at position that leaves exactly one byte at end
  buf = llae.writable_buffer.new("1234567890")
  result = buf:write(10, "X")
  lu.assertTrue(result)
  lu.assertEquals(buf:sub(10, 10), "X")
end

function TestWritableBuffer:test_write_full_buffer()
  -- Test writing full buffer
  local buf = llae.writable_buffer.alloc(5)
  local result = buf:write(1, "hello")
  lu.assertTrue(result)
  lu.assertEquals(tostring(buf), "hello")
  
  -- Test writing at offset 1 with full length
  buf = llae.writable_buffer.alloc(3)
  result = buf:write(1, "abc")
  lu.assertTrue(result)
  lu.assertEquals(tostring(buf), "abc")
end

function TestWritableBuffer:test_write_from_buffer()
  -- Test writing from another buffer
  local src_buf = llae.buffer.new("test")
  local dst_buf = llae.writable_buffer.alloc(10)
  local result = dst_buf:write(1, src_buf)
  lu.assertTrue(result)
  lu.assertEquals(dst_buf:sub(1, 4), "test")
  
  -- Test writing from buffer at offset
  dst_buf = llae.writable_buffer.new("1234567890")
  result = dst_buf:write(3, src_buf)
  lu.assertTrue(result)
  lu.assertEquals(dst_buf:sub(1, 10), "12test7890")
end

function TestWritableBuffer:test_write_from_string()
  -- Test writing string at various positions
  local buf = llae.writable_buffer.new("0000000000")
  local result = buf:write(1, "abc")
  lu.assertTrue(result)
  lu.assertEquals(buf:sub(1, 3), "abc")
  
  result = buf:write(5, "def")
  lu.assertTrue(result)
  lu.assertEquals(buf:sub(5, 7), "def")
end

function TestWritableBuffer:test_write_edge_cases()
  -- Test writing single byte
  local buf = llae.writable_buffer.alloc(5)
  local result = buf:write(1, "A")
  lu.assertTrue(result)
  lu.assertEquals(buf:sub(1, 1), "A")
  
  -- Test writing at last position (already tested above)
  
  -- Test writing empty string
  buf = llae.writable_buffer.new("hello")
  result = buf:write(1, "")
  lu.assertTrue(result)
  lu.assertEquals(tostring(buf), "hello")
end

function TestWritableBuffer:test_write_errors()
  -- Test offset out of range (too large) - results in "data out of range"
  -- because offset is decremented and then checked against buffer length
  local buf = llae.writable_buffer.alloc(5)
  lu.assertErrorMsgContains(
    "data out of range",
    function() buf:write(6, "x") end
  )
  
  -- Test offset out of range (exactly at boundary + 1)
  buf = llae.writable_buffer.alloc(10)
  lu.assertErrorMsgContains(
    "data out of range",
    function() buf:write(11, "x") end
  )
  
  -- Test offset < 1 (should give "offset out of range")
  buf = llae.writable_buffer.alloc(5)
  lu.assertErrorMsgContains(
    "offset out of range",
    function() buf:write(0, "x") end
  )
  
  buf = llae.writable_buffer.alloc(5)
  lu.assertErrorMsgContains(
    "offset out of range",
    function() buf:write(-1, "x") end
  )
  
  -- Test data out of range (data extends beyond buffer)
  buf = llae.writable_buffer.alloc(5)
  lu.assertErrorMsgContains(
    "data out of range",
    function() buf:write(1, "123456") end
  )
  
  -- Test data out of range (data extends beyond buffer at offset)
  buf = llae.writable_buffer.alloc(5)
  lu.assertErrorMsgContains(
    "data out of range",
    function() buf:write(3, "12345") end
  )
end

function TestWritableBuffer:test_write_multiple_operations()
  -- Test multiple writes
  local buf = llae.writable_buffer.alloc(10)
  buf:write(1, "hello")
  buf:write(6, "world")
  lu.assertEquals(tostring(buf), "helloworld")
  
  -- Test overwriting
  buf = llae.writable_buffer.new("1234567890")
  buf:write(1, "abc")
  buf:write(4, "def")
  buf:write(7, "ghi")
  lu.assertEquals(tostring(buf), "abcdefghi0")
end

function TestWritableBuffer:test_inherited_buffer_methods()
  -- Test that writable_buffer inherits buffer methods
  local buf = llae.writable_buffer.new("hello world")
  
  -- Test get_len
  lu.assertEquals(buf:get_len(), 11)
  
  -- Test sub
  lu.assertEquals(buf:sub(1, 5), "hello")
  lu.assertEquals(buf:sub(7, 11), "world")
  
  -- Test find
  local pos = buf:find("world")
  lu.assertEquals(pos, 7)
  
  -- Test byte
  local bytes = {buf:byte(1, 5)}
  lu.assertEquals(bytes[1], string.byte("h"))
  lu.assertEquals(bytes[5], string.byte("o"))
  
  -- Test tostring
  lu.assertEquals(tostring(buf), "hello world")
  
  -- Test reverse
  local reversed = buf:reverse()
  lu.assertEquals(tostring(reversed), "dlrow olleh")
end

function TestWritableBuffer:test_write_binary_data()
  -- Test writing binary data
  local buf = llae.writable_buffer.alloc(10)
  local binary = string.char(0x00, 0xFF, 0x7F, 0x80)
  buf:write(1, binary)
  
  local bytes = {buf:byte(1, 4)}
  lu.assertEquals(bytes[1], 0x00)
  lu.assertEquals(bytes[2], 0xFF)
  lu.assertEquals(bytes[3], 0x7F)
  lu.assertEquals(bytes[4], 0x80)
end

function TestWritableBuffer:test_write_partial_overwrite()
  -- Test partial overwrite
  local buf = llae.writable_buffer.new("1234567890")
  buf:write(3, "XX")
  lu.assertEquals(tostring(buf), "12XX567890")
  
  -- Test overwriting with longer data
  buf = llae.writable_buffer.new("1234567890")
  buf:write(2, "ABCD")
  lu.assertEquals(tostring(buf), "1ABCD67890")
end

function TestWritableBuffer:test_write_boundary_conditions()
  -- Test writing at exact boundary (offset = length, data length = 0)
  local buf = llae.writable_buffer.alloc(5)
  local result = buf:write(5, "")
  lu.assertTrue(result)
  
  -- Test that writing at offset = length + 1 fails with "data out of range"
  -- (because offset is decremented first, then checked)
  buf = llae.writable_buffer.alloc(5)
  lu.assertErrorMsgContains(
    "data out of range",
    function() buf:write(6, "x") end
  )
  
  -- Test that writing at offset = length with single byte is valid
  -- (offset 5 → after decrement = 4, 4 + 1 = 5, which is <= 5)
  buf = llae.writable_buffer.alloc(5)
  result = buf:write(5, "X")
  lu.assertTrue(result)
  lu.assertEquals(buf:sub(5, 5), "X")
  
  -- Test that writing at offset = length with 2 bytes fails
  -- (offset 5 → after decrement = 4, 4 + 2 = 6 > 5)
  buf = llae.writable_buffer.alloc(5)
  lu.assertErrorMsgContains(
    "data out of range",
    function() buf:write(5, "XX") end
  )
  
  -- Test writing at last valid position (offset 4, data length 1, total 5)
  buf = llae.writable_buffer.alloc(5)
  result = buf:write(4, "X")
  lu.assertTrue(result)
  lu.assertEquals(buf:sub(4, 4), "X")
end

return TestWritableBuffer

