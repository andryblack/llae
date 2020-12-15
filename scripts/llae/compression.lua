local archive = require 'archive'

local compression = {}
for n,v in pairs(archive) do
	compression[n]=v
end

function compression.inflate( data )
	local c = archive.new_inflate_read()
	c:finish(data)
	local d = {}
	while true do
		local ch,err = c:read()
		if not ch then
			if err then
				return nil,err
			end
			break
		end
		table.insert(d,ch)
	end
	return table.concat(d,'')
end

function compression.deflate( data )
	local c = archive.new_deflate_read()
	c:finish(data)
	local d = {}
	while true do
		local ch,err = c:read()
		if not ch then
			if err then
				return nil,err
			end
			break
		end
		table.insert(d,ch)
	end
	return table.concat(d,'')
end

return compression 