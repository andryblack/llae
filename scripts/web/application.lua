local class = require 'llae.class'
local http = require 'net.http'
local log = require 'llae.log'
local path = require 'llae.path'

local router = require 'web.router'
local url = require 'net.url'

---@class web.server_request : net.http.server_request
---@field query table<string,string>
---@field fragment string?
---@field path string


---@class web.application : web.router
---@field new fun():web.application
---@field baseclass web.router
---@field private _handlers (fun(req:web.server_request,res:net.http.server_response))[]
local web = class(router,'web.application') ---[[@as class web.application]]

function web:_init( ... )
	web.baseclass._init(self,...)
	self._server = assert(http.createServer(function (req, res)
		self:handle_request(req,res)
	end))
	self._handlers = {}
end

function web:set_fs_root(root)
	self._fs_root = path.getabsolute(root)
end

---@return string?
function web:get_fs_root()
	return self._fs_root
end

---get fs path
---@param fn string
---@return string
function web:get_fs_path(fn)
	return (self._fs_root and not path.isabsolute(fn)) and path.join(self._fs_root,fn) or fn
end



---@param req_base net.http.server_request
---@param res net.http.server_response
function web:handle_request( req_base, res )
	--log.info('handle',req:get_method(),req:get_path())
	
	local req = req_base --[[@as web.server_request]]
	local components = url.parse(req:get_path())
	req.query = components.query
	req.fragment = components.fragment
	req.path = components.path

	if self._fs_root then
		local sself = self
		local osend_static_file = res.send_static_file
		function res:send_static_file(p,conf)
			osend_static_file(self,sself:get_fs_path(p),conf)
		end
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

---@param middl web.middle
function web:use( middl )
	middl:use(self)
	return middl
end

---@param options {host:string?,port:integer?,backlog:integer?}?
function web:listen( options )
	local port = options and options.port or 8080
	local host = options and options.host or '127.0.0.1'
	log.info('listen at:','http://'..host..':'..port)
	assert(self._server:listen(port,host,options and options.backlog))
end

function web:stop()
	self._server:stop()
end

function web.static( root, options )
	local static = require 'web.static'
	return static.new(root,options)
end

function web.views( root, data )
	local views = require 'web.views'
	return views.new(root,data)
end

function web.json(...)
	local json = require 'web.json'
	return json.new(...)
end

function web.cookie(...)
	local cookie = require 'web.cookie'
	return cookie.new(...)
end

function web.formparser(...)
	local formparser = require 'web.formparser'
	return formparser.new(...)
end

function web.multipart(...)
	local multipart = require 'web.multipart'
	return multipart.new(...)
end


return web