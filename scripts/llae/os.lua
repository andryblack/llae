local uv = require 'uv'

local llae = require 'llae'

local _M = setmetatable({},{__index=uv.os})
for k,v in pairs(os) do
	_M[k]=v
end

_M.hostname = uv.os.gethostname
_M.at_exit = llae.at_exit
_M.stop = llae.stop

return _M