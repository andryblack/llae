local _M = {}

local _red = '\x1b[31m'
local _white = '\x1b[37m'
local _green = '\x1b[32m'
local _blue = '\x1b[34m'
local _yellow = '\x1b[33m'
local _reset = '\x1b[0m'
local _bold = '\x1b[1m'
local _italic = '\x1b[3m'

local _prev_line = '\x1b[1A'
local _clear_line = '\x1b[K'

local colors = {'black','red','green','yellow','blue','magenta','cyan','white'}

_M.fg = {}
_M.bg = {}

for i,v in ipairs(colors) do
	_M.fg[v] = '\x1b[' .. (30+i-1) .. 'm'
	_M.bg[v] = '\x1b[' .. (40+i-1) .. 'm'
	_M.fg['bright_'..v] = '\x1b[' .. (90+i-1) .. 'm'
	_M.bg['bright_'..v] = '\x1b[' .. (100+i-1) .. 'm'
end

_M.bold = _bold
_M.italic = _italic
_M.reset = _reset


function _M.set_verbose( v )
	_M._verbose = v
end

local prefix_info = _green .. '[I]' .. _reset
local prefix_debuf = _blue .. '[D]' .. _reset
local prefix_error = _red .. '[E]' .. _reset
local prefix_warning = _yellow .. '[W]' .. _reset

function _M.info( ... )
	print(prefix_info,...)
end

function _M.debug( ... )
	if _M._verbose then
		print(prefix_debuf,...)
	end
end

function _M.error( ... )
	print(prefix_error,...)
end

function _M.warning( ... )
	print(prefix_warning,...)
end

local class = require 'llae.class'
local progress = class()

function progress:_init( width )
	self._width = width or 70
	self._fill = 0
end

function progress:show(  )
	print(_white..'[  0%]'.._blue..'['..string.rep(' ',self._width)..']'.._reset)
end

function progress:update( count, total )
	local fill = 0
	if count and total and total > 0 and  (count<=total) then
		fill =  math.ceil(count*self._width/total)
	else
		fill = self._fill + 1
		if fill > self._width then
			fill = 0
		end
	end
	print(_prev_line.._clear_line..
		_white..'['..string.format('%3d%%',math.ceil(fill*100/self._width))..']'..
		_blue..'['..string.rep('=',fill)..
		string.rep(' ',(self._width-fill))..']'.._reset)
	self._fill = fill
end

function progress:close(  )
	print(_prev_line.._clear_line)
end

function _M.progress( width )
	local p = progress.new(width)
	p:show()
	return p
end

_M.restart_line = _prev_line.._clear_line

return _M