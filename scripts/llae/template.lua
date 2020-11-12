local class = require 'llae.class'
local fs = require 'llae.fs'

local template = class(nil,'template')

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

function template:_init( options )
	self._name = 'template'
	self._env = {
		escape = escape
	}
	if options and options.env then
		for k,v in pairs(options.env) do
			self._env[k]=v
		end
	end
end

function template:load( filename )
	self._name = filename
	local data = fs.load_file(filename)
	return self:parse(data)
end

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
	local widx = #chunks + 1
	local ssub = string.sub
	local function plain(str) 
		if str == '' then
			return
		end

		if ssub(str,1,1) == '\n' then
			chunks[widx] = '_p[===[\n'
		else
			chunks[widx] = '_p[===['
		end
		chunks[widx+1] = str
		chunks[widx+2] = ']===];'
		widx = widx + 3
	end
	local sfind = string.find
	local s = sfind(data,'<%',1, true)
	
	while s do
		local m = ssub(data,s+2,s+2)
		local e = sfind(data,'%>',s+2,true)
		if e then
			plain(ssub(data,st,s-1))
				
			if m == '=' then
				local val = ssub(data,s+3,e-1)
			
				chunks[widx] = '_e(' .. val .. ');'
				widx = widx + 1
			elseif m == '-' then
				local val = ssub(data,s+3,e-1)
			
				chunks[widx] = '_s(' .. val .. ');'
				widx = widx + 1
			else
				local val = ssub(data,s+2,e-1)
				chunks[widx] = val .. ';'
				widx = widx + 1
			end

			st = e+2
		else
			st = s+2
		end
		s = sfind(data,'<%',st,true)
	end
	if st ~= #data then
		plain(ssub(data,st,#data))
	end
	chunks[widx] = 'return table.concat(_res)'
	--print(table.concat(chunks))
	return self:compile(chunks)
end

function template:compile( chunks )
	--self._chunks = chunks
	local env = {
		__index = function (t,k) 
			--print(k,t._ctx[k])
			return t._ctx[k] or self._env[k] or _G[k]
		end
	}
	local idx = 0
	self._compiled = assert(load(function()
			idx = idx + 1
			--print('load',idx,chunks[idx])
			return chunks[idx]
		end,self._name,'t',setmetatable({},env)))
	return self._compiled
end

function template:render( context )
	return self._compiled(context)
end

local _M = {
	escape = escape
}

function _M.compile( str, options )
	local t = template.new(options)
	return t:parse(str)
end

function _M.load( filename, options )
	local t = template.new(options)
	return t:load(filename)
end

function _M.render(str, data, options)
	local t = template.new()
	t:parse(str)
	return t:render(data)
end

function _M.render_file(filename, data, options)
	local t = template.new()
	t:load(filename)
	return t:render(data)
end

return _M