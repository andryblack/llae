local async = require 'llae.async'
local class = require 'llae.class'
local ast = require 'cparse.ast'
local tags = require 'cparse.tags'
local traverser = class(ast.traverser, 'traverser')

function traverser:_init(logger)
    traverser.baseclass._init(self)
    self._logger = logger
end

function traverser:traverse_class(node)
    self._logger(node:__tostring())
    local t = node.tags or tags.new()
    for _, line in ipairs(t:dump_lines()) do
        self._logger(line)
    end
    traverser.baseclass.traverse_class(self,node)
end

function traverser:traverse_func(node)
    self._logger(node:__tostring())
    local t = node.tags or tags.new()
    for _, line in ipairs(t:dump_lines()) do
        self._logger(line)
    end
    traverser.baseclass.traverse_func(self,node)
end

function traverser:traverse_enum(node)
    self._logger(node:__tostring())
    traverser.baseclass.traverse_enum(self,node)
end

function traverser:traverse_namespace(node)
    self._logger(node:__tostring())
    traverser.baseclass.traverse_namespace(self,node)
end

function traverser:traverse_file(node)
    self._logger(node:__tostring())
    traverser.baseclass.traverse_file(self,node)
end

return traverser