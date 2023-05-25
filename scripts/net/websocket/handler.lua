local class = require 'llae.class'

local handler = class()

function handler:on_data(data)
end

function handler:on_text(data)
	self:on_data(data)
end

function handler:on_binary(data)
	self:on_data(data)
end

function handler:on_close(reason)
end

local func_handler = class(handler)

function func_handler:_init(cb)
	self._cb = cb
end

function func_handler:on_text(data)
	self._cb(data,'text')
end

function func_handler:on_binary(data)
	self._cb(data,'binary')
end

function func_handler:on_close(reason)
	self._cb(nil,reason)
end

function handler.wrap(func)
	return func_handler.new(func)
end

return handler