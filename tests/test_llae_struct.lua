local lu = require 'luaunit'
local struct = require 'llae.struct'

-- Tests for llae.struct.offsetof.
--
-- offsetof(s, f) must return the byte offset of field `f` inside the packed
-- layout described by the field list `s`.  Advancing to the next field has to
-- add the size of the current field (fs), multiplied by its element count
-- (v[3], 1 when the field is not an array):
--
--     r = r + fs * (v[3] or 1)
--
-- The tests below use explicit, hand-computed offsets so that a wrong
-- accumulator expression is reported as a wrong offset (or as an error)
-- instead of being masked by reusing struct.sizeof() in the expectations.

TestStruct = {}

local function assert_offsets(s, expected)
	for name, offset in pairs(expected) do
		lu.assertEquals(struct.offsetof(s, name), offset,
			'wrong offset for field ' .. tostring(name))
	end
end

-- Packed scalars of different sizes.
local scalars = {
	{'u32', 'a'}, -- 4 bytes, offset 0
	{'u16', 'b'}, -- 2 bytes, offset 4
	{'u8',  'c'}, -- 1 byte,  offset 6
	{'u64', 'd'}, -- 8 bytes, offset 7
	{'u8',  'e'}, -- 1 byte,  offset 15
}

function TestStruct:test_offsetof_first_field_is_zero()
	lu.assertEquals(struct.offsetof(scalars, 'a'), 0)
end

function TestStruct:test_offsetof_second_field_regression()
	-- Minimal case from the bug report: a scalar field must advance the
	-- accumulator by its size, not by multiplying the struct definition.
	local s = {
		{'u32', 'first'},
		{'u32', 'second'},
	}
	lu.assertEquals(struct.offsetof(s, 'second'), 4)

	lu.assertEquals(struct.offsetof(s, 'first'), 0)
end

function TestStruct:test_offsetof_packed_scalars()
	assert_offsets(scalars, {
		a = 0,
		b = 4,
		c = 6,
		d = 7,
		e = 15,
	})
	lu.assertEquals(struct.sizeof(scalars), 16)
end

function TestStruct:test_offsetof_fields_with_element_counts()
	-- v[3] is the element count and must multiply the field size.
	local s = {
		{'u8',  'flags', 4}, -- 4 bytes at offset 0
		{'u32', 'value'},    -- 4 bytes at offset 4
		{'u16', 'pair', 3},  -- 6 bytes at offset 8
		{'u8',  'tail'},     -- 1 byte  at offset 14
	}
	assert_offsets(s, {
		flags = 0,
		value = 4,
		pair = 8,
		tail = 14,
	})
	lu.assertEquals(struct.sizeof(s), 15)
end

function TestStruct:test_offsetof_byte_and_zero_terminated_arrays()
	local s = {
		{'zs', 'name', 10}, -- 10 bytes at offset 0
		{'u32', 'value'},   -- 4 bytes  at offset 10
		{'ba', 'raw', 4},   -- 4 bytes  at offset 14
		{'u8', 'last'},     -- 1 byte   at offset 18
	}
	assert_offsets(s, {
		name = 0,
		value = 10,
		raw = 14,
		last = 18,
	})
	lu.assertEquals(struct.sizeof(s), 19)
end

function TestStruct:test_offsetof_anonymous_padding()
	-- A stub field without a name still occupies space.
	local s = {
		{'u8', nil, 3},     -- 3 bytes of padding at offset 0
		{'u32', 'value'},   -- 4 bytes at offset 3
		{'u8', 'last'},     -- 1 byte  at offset 7
	}
	assert_offsets(s, {
		value = 3,
		last = 7,
	})
	lu.assertEquals(struct.sizeof(s), 8)
end

function TestStruct:test_offsetof_inline_nested_struct()
	local inner = {
		{'u32', 'x'}, -- offset 0 inside inner
		{'u16', 'y'}, -- offset 4 inside inner
	}
	local s = {
		{'u32', 'head'},  -- 4 bytes at offset 0
		{inner, 'body'},  -- 6 bytes at offset 4
		{'u64', 'tail'},  -- 8 bytes at offset 10
	}
	assert_offsets(s, {
		head = 0,
		body = 4,
		tail = 10,
	})
	lu.assertEquals(struct.sizeof(s), 18)
end

function TestStruct:test_offsetof_nested_struct_array()
	local point = {
		{'u16', 'x'}, -- 2 bytes
		{'u16', 'y'}, -- 2 bytes
	}
	local s = {
		{'u8', 'tag'},         -- 1 byte  at offset 0
		{point, 'points', 3},  -- 12 bytes at offset 1
		{'u32', 'crc'},        -- 4 bytes  at offset 13
	}
	assert_offsets(s, {
		tag = 0,
		points = 1,
		crc = 13,
	})
	lu.assertEquals(struct.sizeof(s), 17)
end

function TestStruct:test_offsetof_predefined_struct_def()
	local point = {
		{'u16', 'x'},
		{'u16', 'y'},
	}
	local point_def = struct.define(point)
	local s = {
		{'u8', 'tag'},      -- 1 byte at offset 0
		{point_def, 'p'},   -- 4 bytes at offset 1
		{'u8', 'last'},     -- 1 byte at offset 5
	}
	assert_offsets(s, {
		tag = 0,
		p = 1,
		last = 5,
	})
	lu.assertEquals(struct.sizeof(s), 6)
end

function TestStruct:test_offsetof_unknown_field_is_nil()
	lu.assertNil(struct.offsetof(scalars, 'missing'))
	lu.assertNil(struct.offsetof({}, 'missing'))
end

function TestStruct:test_offsetof_does_not_raise_for_named_fields()
	-- Advancing past the first field used to raise
	-- "attempt to perform arithmetic on a table value (local 's')".
	for _, field in ipairs(scalars) do
		local ok, err = pcall(struct.offsetof, scalars, field[2])
		lu.assertTrue(ok, 'offsetof raised for field ' .. field[2] .. ': ' .. tostring(err))
	end
end
