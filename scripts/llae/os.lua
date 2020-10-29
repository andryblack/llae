local uv = require 'uv'

local _M = setmetatable({},{__index=uv.os})
for k,v in pairs(os) do
	_M[k]=v
end

_M.hostname = uv.os.gethostname

return _M