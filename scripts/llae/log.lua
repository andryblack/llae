local _M = {}

_M._red = '\x1b[0m'
_M._white = '\x1b[37m'
_M._green = '\x1b[32m'
_M._blue = '\x1b[34m'
_M._reset = '\x1b[0m'

function _M.info( ... )
	print(_M._green .. '[info]' .. _M._reset,...)
end

function _M.debug( ... )
	print(_M._blue .. '[debug]' .. _M._reset,...)
end

function _M.error( ... )
	print(_M._red .. '[error]' .. _M._reset,...)
end

return _M