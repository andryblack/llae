local class = require 'llae.class'
local fs = require 'llae.fs'
local path = require 'llae.path'
local log = require 'llae.log'
local template = require 'llae.template'
local utils = require 'llae.utils'

---@class web.views : web.middle
---@field private _cache table<string,function>
---@field new fun(root:string,data:table) : web.views
local views = class(require 'web.middle','web.views')

---@param root string
---@param options {ext:string?,env:table?,nocache:boolean?}?
function views:_init( root, options )
	self._root = root
	self._options = options
	self._ext = (options and options.ext) or 'thtml' 
	self._cache = {}
	self._cache_parts = {}
	self._funcs = {}

	self._funcs.include = function (view,...) 
		return self:_include(view,...)
	end

	self._funcs.include_parts = function (view,...) 
		return self:_include_parts(view,...)
	end
end

function views:_include(view,...)
	local t = self:get(view)
	return t(self:_merge_context(...))
end

function views:_include_parts(view,...)
	local t = self:get_parts(view)
	return t
end

function views:_merge_context(...)
	return utils.merge(self._options.env,self._funcs,...)
end

function views:_resolve_view_path(view)
	return self._app:get_fs_path(path.join(self._root,view .. '.' .. self._ext))
end

function views:_load_template(fn,options)
	return template.load(fn,options)
end

function views:_load_parts_source(fn)
	return tostring(fs.load_file(fn))
end

function views:_compile_part(part,view,part_name)
	local options = utils.merge(self._options,{name = view .. '/' .. part_name})
	return template.compile(part,options)
end

function views:check( fn )
	local s = fs.stat(fn)
	return s and s.isfile
end

function views:get( view )
	local t = self._cache[view]
	if t and not self._options.nocache then
		return t
	end
	local fn = self:_resolve_view_path(view)
	log.debug('load template',fn)
	local roptions = utils.merge(self._options,{name=view})
	t = self:_load_template(fn,roptions)
	self._cache[view] = t
	return t
end

function views:get_parts( view )
	local t = self._cache_parts[view]
	if t and not self._options.nocache then
		return t
	end
	local fn = self:_resolve_view_path(view)
	--log.debug('load parts template',fn)
	local data = self:_load_parts_source(fn)

	t = {}
	local pos = 1
	local name = 'base'
	while true do
		local npos,epos = string.find(data,'%-{.-}%-',pos)
		--log.info('found part:',npos,epos)
		local part = string.sub(data,pos,npos and npos-1)
		local r = self:_compile_part(part,view,name)
		--log.info('compile part',name)
		t[name] = function(...)
			return r(self:_merge_context(...))
		end
		if not npos then
			break
		end
		name = string.match(string.sub(data,npos,epos),'%-{(.+)}%-')
		pos = epos + 1
	end

	self._cache_parts[view] = t
	return t
end

local function format_error(err)
	return 'Error rendering template: ' .. tostring(err)
end

---@param resp net.http.server_response
function views:_render( resp, view, ... )
	local t = self:get(view)
	local context = self:_merge_context(...)
	local ok,content = pcall(t,context)
	if not ok then
		local err = content
		return resp:status(500):finish(format_error(err))
	end
	resp:set_header("Content-Type", "text/html")
	return resp:finish(content)
end

function views:use( app )
	self._app = app
	app:register_handler( function (request, resp )
		resp.render = function (_,view, ...) 
			return self:_render(resp,view,...)
		end
	end)
end

return views