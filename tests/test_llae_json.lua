local lu = require 'luaunit'
local json = require 'llae.json'

TestJson = {}

function TestJson:test_encode_basic_types()
  -- Test nil
  lu.assertEquals(json.encode(nil), "null")
  
  -- Test boolean
  lu.assertEquals(json.encode(true), "true")
  lu.assertEquals(json.encode(false), "false")
  
  -- Test numbers
  lu.assertEquals(json.encode(42), "42")
  lu.assertEquals(json.encode(-123), "-123")
  -- Test floating point number
  local decoded = json.decode(json.encode(3.14))
  lu.assertAlmostEquals(decoded, 3.14, 0.0001)
  
  -- Test string
  lu.assertEquals(json.encode("hello"), '"hello"')
  lu.assertEquals(json.encode("special \"chars\""), '"special \\"chars\\""')
end

function TestJson:test_encode_arrays()
  -- Test empty array
  local empty = json.array{}
  lu.assertEquals(json.encode(empty), "[]")
  
  -- Test simple array
  local arr = json.array{1, 2, 3}
  lu.assertEquals(json.encode(arr), "[1,2,3]")
  
  -- Test nested arrays
  local nested = json.array{1, json.array{2, 3}, 4}
  lu.assertEquals(json.encode(nested), "[1,[2,3],4]")
  
  -- Test sparse arrays with nil values
  local sparse = json.array{1, nil, 3}
  lu.assertEquals(json.encode(sparse), "[1,null,3]")

  -- Test sparse array with multiple consecutive nils
  local multi_nil = json.array{1, nil, nil, 4}
  lu.assertEquals(json.encode(multi_nil), "[1,null,null,4]")

  -- Test sparse array with nil at start
  local nil_start = json.array{nil, 2, 3}
  lu.assertEquals(json.encode(nil_start), "[null,2,3]")

  -- Test sparse array with nil at end (trimmed)
  local nil_end = json.array{1, 2, nil}
  lu.assertEquals(json.encode(nil_end), "[1,2]")

  -- Test sparse array with multiple nils at end (all trimmed)
  local multi_nil_end = json.array{1, nil, nil}
  lu.assertEquals(json.encode(multi_nil_end), "[1]")

  -- Test sparse array with only nils
  local all_nil = json.array{nil, nil, nil}
  lu.assertEquals(json.encode(all_nil), "[]")

  -- Test mixed sparse array with nested arrays and objects
  local mixed = json.array{
    nil,
    json.array{1, nil, 3},
    {x = 1},
    nil,
    5
  }
  lu.assertEquals(json.encode(mixed), '[null,[1,null,3],{"x":1},null,5]')
end

function TestJson:test_encode_objects()
  -- Test empty object
  lu.assertEquals(json.encode({}), "{}")
  
  -- Test simple object
  local obj = {name = "test", value = 42}
  local encoded = json.encode(obj)
  -- Since object keys order is not guaranteed, we decode and compare
  local decoded = json.decode(encoded)
  lu.assertEquals(decoded.name, "test")
  lu.assertEquals(decoded.value, 42)
  
  -- Test nested object
  local nested = {
    user = {
      name = "John",
      age = 30
    },
    active = true
  }
  decoded = json.decode(json.encode(nested))
  lu.assertEquals(decoded.user.name, "John")
  lu.assertEquals(decoded.user.age, 30)
  lu.assertEquals(decoded.active, true)
end

function TestJson:test_encode_sorted()
  -- Test object with sorted keys
  local obj = {c = 3, a = 1, b = 2}
  lu.assertEquals(
    json.encode_sorted(obj),
    '{"a":1,"b":2,"c":3}'
  )
  
  -- Test nested objects with sorted keys
  local nested = {
    z = {y = 2, x = 1},
    b = {d = 4, c = 3}
  }
  lu.assertEquals(
    json.encode_sorted(nested),
    '{"b":{"c":3,"d":4},"z":{"x":1,"y":2}}'
  )
end

function TestJson:test_decode_basic()
  -- Test basic types
  lu.assertNil(json.decode("null"))
  lu.assertTrue(json.decode("true"))
  lu.assertFalse(json.decode("false"))
  lu.assertEquals(json.decode("42"), 42)
  lu.assertEquals(json.decode("3.14"), 3.14)
  lu.assertEquals(json.decode('"hello"'), "hello")
end

function TestJson:test_decode_arrays()
  -- Test empty array
  local arr = json.decode("[]", {mark_arrays = true})
  lu.assertTrue(json.is_array(arr))
  lu.assertEquals(#arr, 0)
  
  -- Test simple array
  arr = json.decode("[1,2,3]", {mark_arrays = true})
  lu.assertTrue(json.is_array(arr))
  lu.assertEquals(#arr, 3)
  lu.assertEquals(arr[1], 1)
  lu.assertEquals(arr[2], 2)
  lu.assertEquals(arr[3], 3)
  
  -- Test nested arrays
  arr = json.decode("[1,[2,3],4]", {mark_arrays = true})
  lu.assertTrue(json.is_array(arr))
  lu.assertTrue(json.is_array(arr[2]))
  lu.assertEquals(arr[1], 1)
  lu.assertEquals(arr[2][1], 2)
  lu.assertEquals(arr[2][2], 3)
  lu.assertEquals(arr[3], 4)

  -- Test sparse arrays with null values
  arr = json.decode("[1,null,3]", {mark_arrays = true})
  lu.assertTrue(json.is_array(arr))
  lu.assertEquals(arr[1], 1)
  lu.assertNil(arr[2])
  lu.assertEquals(arr[3], 3)

  -- Test array with multiple consecutive nulls
  arr = json.decode("[1,null,null,4]", {mark_arrays = true})
  lu.assertTrue(json.is_array(arr))
  lu.assertEquals(arr[1], 1)
  lu.assertNil(arr[2])
  lu.assertNil(arr[3])
  lu.assertEquals(arr[4], 4)

  -- Test array with null at start
  arr = json.decode("[null,2,3]", {mark_arrays = true})
  lu.assertTrue(json.is_array(arr))
  lu.assertNil(arr[1])
  lu.assertEquals(arr[2], 2)
  lu.assertEquals(arr[3], 3)

  -- Test array with trailing nulls
  arr = json.decode("[1,2,null]", {mark_arrays = true})
  lu.assertTrue(json.is_array(arr))
  lu.assertEquals(arr[1], 1)
  lu.assertEquals(arr[2], 2)
  lu.assertNil(arr[3])

  -- Test array with only nulls
  arr = json.decode("[null,null,null]", {mark_arrays = true})
  lu.assertTrue(json.is_array(arr))
  lu.assertNil(arr[1])
  lu.assertNil(arr[2])
  lu.assertNil(arr[3])
end

function TestJson:test_decode_objects()
  -- Test empty object
  local obj = json.decode("{}")
  lu.assertFalse(json.is_array(obj))
  
  -- Test simple object
  obj = json.decode('{"name":"test","value":42}')
  lu.assertEquals(obj.name, "test")
  lu.assertEquals(obj.value, 42)
  
  -- Test nested object
  obj = json.decode([[
    {
      "user": {
        "name": "John",
        "age": 30
      },
      "active": true
    }
  ]])
  lu.assertEquals(obj.user.name, "John")
  lu.assertEquals(obj.user.age, 30)
  lu.assertTrue(obj.active)
end

function TestJson:test_decode_options()
  -- Test safe mode
  local val, err = json.decode('{"invalid": }', {safe = true})
  lu.assertNil(val)
  lu.assertNotNil(err)
  
  -- Test array marking
  local arr = json.decode('[1,2,3]', {mark_arrays = true})
  lu.assertTrue(json.is_array(arr))
  
  -- Test comments
  local obj = json.decode([[
    {
      // Single line comment
      "name": "test", /* Multi-line
                         comment */
      "value": 42
    }
  ]], {allow_comments = true})
  lu.assertEquals(obj.name, "test")
  lu.assertEquals(obj.value, 42)
end

function TestJson:test_error_handling()
  -- Test invalid JSON
  lu.assertErrorMsgContains(
    "premature EOF",
    json.decode,
    '{"invalid":'
  )
  
  -- Test invalid UTF-8
  local val, err = json.decode(
    '{"bad": "\255"}',
    {safe = true, validate_utf = true}
  )
  lu.assertNil(val)
  lu.assertNotNil(err)

  local val,err = json.decode(nil, {safe = true})
  lu.assertNil(val)
  lu.assertNotNil(err)
end

return TestJson