local class = require 'llae.class'
local http = require 'net.http'
local log = require 'llae.log'

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

function web:handle_request( req, res )
	log.info('handle',req:get_method(),req:get_path())
	
	local components = url.parse(req:get_path())
	req.query = components.query
	req.fragment = components.fragment
	req.path = components.path

	for _,f in ipairs(self._handlers) do
		f(req, res)
	end
	
	local handle_next = false
	local function do_next()
		log.debug('next')
		handle_next = true
	end
	local handled = self:resolve(req:get_method(),req.path,function(f,params)
		handle_next = false
		req.params = params
		f(req,res,do_next)
		return not handle_next
	end)
	if not handled or not res:is_finished() then
		res:status(404,'Not found'):finish('Not found [' .. req:get_method() .. ']' .. req:get_path())
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
	return static.new(root,options)
end

function web.views( root, data )
	local views = require 'web.views'
	return views.new(root,data)
end

function web.json( )
	local json = require 'web.json'
	return json.new()
end

return web