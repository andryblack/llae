
package.path = package.path .. ';scripts/?.lua'


local llae = require 'llae'
local http = require 'llae.http'
local fs = require 'llae.fs'
local path = require 'llae.path'

local cached = true

http.createServer(function (req, res)
	local p = '.' .. req:get_path()
	local s = fs.stat(p)
	local body
	if s then
		if s.isdir then
			local r = {'<html>','<body>'}
			local parent = req:get_path()
			if #parent > 1 then
				table.insert(r,'<a href="' .. path.dirname(parent) .. '">..</a><p>')
				parent = parent .. '/'
			end
			local files = fs.scandir(p)
			
			for _,f in ipairs(files) do
				table.insert(r,'<a href="' .. parent .. f.name..'">' .. f.name .. '</a><p>')
			end
			table.insert(r,'</html>')
			table.insert(r,'</body>')
			body = table.concat(r,'\n')
		elseif s.isfile then
			if cached then
				-- implement cache control on file modification time
				res:send_static_file(p)
				return
			else
				local f,e = fs.open(p)
				if f then
					res:set_header('Content-Length',s.size)
					res:flush()
					f:send(res:get_connection())
					res:finish()
					return
				end
			end
		end
	end
	if not body then
		body = [[
<html>
<body>
Not found ]] .. p .. [[
</body>
<html>
]]
	end
  
  res:set_header("Content-Type", "text/html")
  res:finish(body)
end):listen(1337, '127.0.0.1')

print('Server running at http://127.0.0.1:1337/')
llae.set_handler()
llae.run()

print('Terminating server')

llae.dump()