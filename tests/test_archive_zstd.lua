local lu = require 'luaunit'

local zstd = require 'archive.zstd'

testZstd = {}

local function lu_assert(val,err)
	if val then
		return val
	end
	lu.error(err)
end

local test_data = [[Test data for comression
012345
$%%%%%
]]

function testZstd:test_compress_decompress()
	local data = test_data
	local cdata = lu_assert(zstd.compress(data))
	local ddata = lu_assert(zstd.decompress(cdata,#data))
	lu.assertEquals(ddata,ddata)
end

function testZstd:test_decompress_stream()
	local data = test_data
	for i=1,8 do
		data = data .. data .. data .. data
	end
	local cdata = lu_assert(zstd.compress(data))
	local ds = zstd.new_zstd_read()
	assert(ds:write(cdata))
	assert(ds:finish())
	local ddata = {}
	while true do
		local ch,err = ds:read()
		if not ch then
			if err then
				error(err)
			end
			break
		else
			table.insert(ddata,tostring(ch))
		end
	end
	ddata = table.concat(ddata,'')
	lu.assertEquals(ddata,data)
end