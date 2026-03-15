local async = require 'llae.async'
local log = require 'llae.log'
local class = require 'llae.class'
--package.path = package.path .. ';tools/?.lua'
local processor = require 'cparse.process_bind'
local ast = require 'cparse.ast'
local traverser = class(ast.traverser, 'traverser')

function traverser:traverse_class(node)
    log.info(node:__tostring())
    log.info(node.doc)
    traverser.baseclass.traverse_class(self,node)
end

function traverser:traverse_func(node)
    log.info(node:__tostring())
    log.info(node.doc)
    traverser.baseclass.traverse_func(self,node)
end

local function dump_ast_cmd(filename)
    local proc = processor.new()
    local result,ast = proc:process_file(filename)
    local t = traverser.new()
    t:traverse(ast)
end

async.run(function()
    local filename = args[1] or error('need filename')
    dump_ast_cmd(filename)
end,true)