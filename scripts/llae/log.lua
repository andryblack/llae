local _M = {}

_M._red = '\x1b[31m'
_M._white = '\x1b[37m'
_M._green = '\x1b[32m'
_M._blue = '\x1b[34m'
_M._reset = '\x1b[0m'

_M._prev_line = '\x1b[1A'
_M._clear_line = '\x1b[K'

function _M.set_verbose( v )
	_M._verbose = v
end

function _M.info( ... )
	print(_M._green .. '[info]' .. _M._reset,...)
end

function _M.debug( ... )
	if _M._verbose then
		print(_M._blue .. '[debug]' .. _M._reset,...)
	end
end

function _M.error( ... )
	print(_M._red .. '[error]' .. _M._reset,...)
end

local class = require 'llae.class'
local progress = class()

function progress:_init( width )
	self._width = width or 70
	self._fill = 0
end

function progress:show(  )
	print(_M._white..'[  0%]'.._M._blue..'['..string.rep(' ',self._width)..']'.._M._reset)
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
	print(_M._prev_line.._M._clear_line..
		_M._white..'['..string.format('%3d%%',math.ceil(fill*100/self._width))..']'..
		_M._blue..'['..string.rep('=',fill)..
		string.rep(' ',(self._width-fill))..']'.._M._reset)
	self._fill = fill
end

function progress:close(  )
	print(_M._prev_line.._M._clear_line)
end

function _M.progress( width )
	local p = progress.new(width)
	p:show()
	return p
end

_M.restart_line = _M._prev_line.._M._clear_line

return _M