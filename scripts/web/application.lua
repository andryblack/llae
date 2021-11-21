local class = require 'llae.class'
local http = require 'net.http'
local log = require 'llae.log'
local path = require 'llae.path'

local router = require 'web.router'
local url = require 'net.url'

local web = class(router,'web')


function web:_init( ... )
	web.baseclass._init(self,...)
	self._server = assert(http.createServer(function (req, res)
		self:handle_request(req,res)
	end))
	self._handlers = {}
end

function web:set_root(root)
	self._root = root
end

function web:handle_request( req, res )
	log.info('handle',req:get_method(),req:get_path())
	
	local components = url.parse(req:get_path())
	req.query = components.query
	req.fragment = components.fragment
	req.path = components.path

	local osend_static_file = res.send_static_file
	function res:send_static_file(p,conf)
		local full_path = (path.isabsolute(p) or not self._root) and p or path.join(self._root,p)
		osend_static_file(self,full_path,conf)
	end

	for _,f in ipairs(self._handlers) do
		f(req, res)
		if res:is_finished() then
			return
		end
	end
	
	local handle_next = false
	local function do_next()
		log.debug('next')
		handle_next = true
	end
	local method = req:get_protocol()..':'..req:get_method()
	local handled = self:resolve(method,req.path,function(f,params)
		handle_next = false
		req.params = params
		f(req,res,do_next)
		return not handle_next
	end)
	if not handled or not res:is_finished() then
		res:status(404,'Not found'):finish('Not found [' .. method .. ']' .. req:get_path())
		return
	end
end

function web:register_handler( f )
	table.insert(self._handlers, f)
end

function web:use( middl )
	middl:use(self)
end

function web:listen( options )
	local port = options and options.port or 8080
	local host = options and options.host or '127.0.0.1'
	log.info('listen at:','http://'..host..':'..port)
	assert(self._server:listen(port,host))
end

function web.static( root, options )
	local static = require 'web.static'
	local full_root = (path.isabsolute(root) or not self._root) and root or path.join(self._root,root)
	return static.new(full_root,options)
end

function web.views( root, data )
	local views = require 'web.views'
	local full_root = (path.isabsolute(root) or not self._root) and root or path.join(self._root,root)
	return views.new(full_root,data)
end

function web.json( )
	local json = require 'web.json'
	return json.new()
end

return web