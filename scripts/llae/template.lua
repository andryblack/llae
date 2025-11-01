local class = require 'llae.class'
local fs = require 'llae.fs'
local path = require 'llae.path'

---@class llae.template.options
---@field name string?
---@field debug boolean?
---@field env table<string,any>?
---@field open_tag string?
---@field close_tag string?

---@class llae.template
---@field new fun(llae.template.options) : llae.template
local template = class(nil,'llae.template')

local html_escape = {
    ["&"] = "&amp;",
    ["<"] = "&lt;",
    [">"] = "&gt;",
    ['"'] = "&quot;",
    ["'"] = "&#39;",
    ["/"] = "&#47;"
}

local function escape(str)
	if type(str) == "string" then
        return string.gsub(str, "[\">/<'&]", html_escape)
    end
    return tostring(str)
end

---@param options llae.template.options?
function template:_init( options )
	self._name = (options and options.name) or 'template'
	self._env = {
		escape = escape
	}
	self._debug = options and options.debug
	self._open_tag = options and options.open_tag or '<%'
	self._close_tag = options and options.close_tag or '%>'
	if options and options.env then
		for k,v in pairs(options.env) do
			self._env[k]=v
		end
	end
end

---@param filename string
---@param name string?
---@return fun(context: table<string,any>?): string
function template:load( filename , name)
	self._name = name or path.getrelative(filename)
	local data = fs.load_file(filename)
	return self:parse(data)
end

---@param code string
---@return string
function template:process_code( code )
	return code
end

---@param data string
---@return fun(context: table<string,any>?): string
function template:parse( data )
	assert(type(data)=='string')
	local st = 1
	local chunks = {
		'_ctx = ... or {}; local _res = {}; local _ti = table.insert;',
		'local _p = function(ch) _ti(_res,ch) end;',
		'local _escape = escape;',
		'local _s = function(ch) _p(tostring(ch)) end;',
		'local _e = function(ch) _p(_escape(ch)) end;',
	}
	
	local ssub = string.sub
	local function plain(str) 
		if str == '' then
			return
		end

		if ssub(str,1,1) == '\n' then
			table.insert(chunks,'_p[===[\n')
		else
			table.insert(chunks,'_p[===[')
		end
		table.insert(chunks,str)
		table.insert(chunks,']===];')
	end
	local sfind = string.find
	local open_tag = self._open_tag
	local close_tag = self._close_tag
	local open_tag_len = #open_tag
	local close_tag_len = #close_tag
	local s = sfind(data,open_tag,1, true)
	
	while s do
		local tag_end = s + open_tag_len
		local m = ssub(data,tag_end,tag_end)
		local e = sfind(data,close_tag,tag_end,true)
		if e then
			plain(ssub(data,st,s-1))
				
			if m == '=' then
				local val = ssub(data,tag_end+1,e-1)
				table.insert(chunks,'_e(' .. val .. ');')
			elseif m == '-' then
				local val = ssub(data,tag_end+1,e-1)
				table.insert(chunks,'_s(' .. val .. ');')
			else
				local val = ssub(data,tag_end,e-1)
				table.insert(chunks,self:process_code(val) .. ';')
			end

			st = e + close_tag_len
		else
			st = tag_end
		end
		s = sfind(data,open_tag,st,true)
	end
	if st <= #data then
		plain(ssub(data,st,#data))
	end
	table.insert(chunks,'return table.concat(_res)')
	return self:compile(chunks)
end

---@param str string
---@return string
local function build_lines(str)
	local d = {}
	local linenum = 1
	for line in str:gmatch("[^\r\n]+") do
		local ln = tostring(linenum)
		linenum = linenum + 1
		table.insert(d,string.rep(' ',4-#ln) .. ln..': '..line)
	end
	return table.concat(d,'\n')
end

---@param chunks string[]
---@return fun(context: table<string,any>?): string
function template:compile( chunks )
	--self._chunks = chunks
	local env = {
		__index = function (t,k) 
			--print(k,t._ctx[k])
			return t._ctx[k] or self._env[k] or _G[k]
		end
	}
	local idx = 0
	local res,err = load(function()
				idx = idx + 1
				--print('load',idx,chunks[idx])
				return chunks[idx]
			end,self._name,'t',setmetatable({},env))
	if self._debug then
		if res then
			self._compiled = res
		else
			err = err .. '\n' .. build_lines(table.concat(chunks)) .. '\n'
			error('Failed compile template: ' .. '\n' .. err)
		end
	else
		self._compiled = assert(res,err)
	end
	return self._compiled
end

---@param context table<string,any>?
---@return string
function template:render( context )
	return self._compiled(context)
end

local _M = {
	escape = escape,
	template = template
}

---@param str string
---@param options llae.template.options?
---@return fun(context: table<string,any>?): string
function _M.compile( str, options )
	local t = template.new(options)
	return t:parse(str)
end

---@param filename string
---@param options llae.template.options?
---@return fun(context: table<string,any>?): string
function _M.load( filename, options )
	local t = template.new(options)
	return t:load(filename,options and options.name)
end

---@param str string
---@param data table<string,any>?
---@param options llae.template.options?
---@return string
function _M.render(str, data, options)
	local t = template.new(options)
	t:parse(str)
	return t:render(data)
end

---@param filename string
---@param data table<string,any>?
---@param options llae.template.options?
---@return string
function _M.render_file(filename, data, options)
	local t = template.new(options)
	t:load(filename,options and options.name)
	return t:render(data)
end

return _M