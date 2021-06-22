local tool = require 'tool'
local class = require  'llae.class'

local cmd = class(tool)
cmd.name = 'server'
cmd.descr = 'simple HTTP server'
cmd.args = {
	{'port','TCP listening port'},
	{'bind','bind address'},
	{'root','root folder'}
}
local llae = require 'llae'
local http = require 'net.http'
local url = require 'net.url'
local log  = require 'llae.log'
local path = require 'llae.path'
local fs = require 'llae.fs'




function cmd:exec( args )
	local port = tonumber(args.port or 8000)
	local addr = args.bind or '127.0.0.1'
	local root = args.root or fs.pwd()
	assert(http.createServer(function (req, res)
		
		local components = url.parse(req:get_path())
		req.query = components.query
		req.fragment = components.fragment
		req.path = components.path

		log.info('request',req:get_method(),req.path)
		if req:get_method() == 'GET' then
			local f = path.join(root,req.path)
			if fs.isfile(f) then
				return res:send_static_file(f)
			elseif fs.isdir(f) then
				local r = {'<html>','<body>'}
				local parent = req_path
				if #parent > 1 then
					table.insert(r,'<a href="' .. path.dirname(parent) .. '">..</a><p>')
					parent = parent .. '/'
				end
				local files = fs.scandir(f)
				table.insert(r,'<ul>')
				for _,f in ipairs(files) do
					table.insert(r,'<li>')
					table.insert(r,'<a href="' .. parent .. f.name..'">' .. f.name .. '</a>')
					table.insert(r,'</li>')
				end
				table.insert(r,'</ul>')
				table.insert(r,'</html>')
				table.insert(r,'</body>')
				body = table.concat(r,'\n')

				res:set_header("Content-Type", "text/html")
				res:finish(body)

			else
				res:status(404,'Not found'):finish('Not found [' .. req:get_method() .. ']' .. req.path)
			end
		end

	  
	end):listen(port, addr))
	log.info('Server running at http://'..addr..':'..port)
end


return cmd



