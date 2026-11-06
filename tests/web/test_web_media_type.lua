local lu = require('luaunit')
local media_type = require('web.media_type')

TestWebMediaType = {}

function TestWebMediaType:test_get_strips_parameters_and_whitespace()
  lu.assertEquals(media_type.get('application/json; charset=utf-8'), 'application/json')
  lu.assertEquals(media_type.get(' Application/JSON ;charset=UTF-8'), 'application/json')
  lu.assertEquals(media_type.get('text/plain'), 'text/plain')
end

function TestWebMediaType:test_get_returns_nil_for_missing_or_empty_value()
  lu.assertNil(media_type.get(nil))
  lu.assertNil(media_type.get(''))
  lu.assertNil(media_type.get('   '))
  lu.assertNil(media_type.get('; charset=utf-8'))
  lu.assertNil(media_type.get(42))
end

function TestWebMediaType:test_is_compares_media_type_case_insensitively()
  lu.assertTrue(media_type.is('application/json', 'application/json'))
  lu.assertTrue(media_type.is('Application/JSON; charset=UTF-8', 'application/json'))
  lu.assertFalse(media_type.is('application/json-patch+json', 'application/json'))
  lu.assertFalse(media_type.is('text/plain', 'application/json'))
  lu.assertFalse(media_type.is(nil, 'application/json'))
end
