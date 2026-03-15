local class = require 'llae.class'
local fs = require 'llae.fs'
local log = require 'llae.log'
local preprocessor = require 'cparse.preprocessor'
local parser = require 'cparse.parser'
local ast = require 'cparse.ast'
local tags = require 'cparse.tags'


local bind_module = class(nil, 'bind_module')

function bind_module:_init(name)
    self._name = name
    self._classes = {}
    self._functions = {}
    self._enums = {}
    self._headers = {}
    self._values = {}
end

function bind_module:add_header(header)
    table.insert(self._headers, header)
end

function bind_module:get_headers()
    local res = {}
    local set = {}
    for _,header in ipairs(self._headers) do
        if not set[header] then
            table.insert(res, header)
            set[header] = true
        end
    end
    return res
end

function bind_module:add_class(class)
    table.insert(self._classes, class)
    class:set_module(self)
end

function bind_module:get_classes()
    return self._classes
end

function bind_module:get_sorted_classes()
    local res = {}
    local available = {}
    local added = {}
    for _,class in ipairs(self._classes) do
        available[class:get_name()] = class
    end
    while next(available) do    
        for _,class in ipairs(self._classes) do
            if not added[class:get_name()] then
                local all_bases_added = true
                for _,base in ipairs(class:get_bases()) do
                    if available[base] then
                        all_bases_added = false
                        break
                    end
                end
                if all_bases_added then
                    table.insert(res, class)
                    added[class:get_name()] = true
                    available[class:get_name()] = nil
                end
            end
        end
    end
   
    return res
end

function bind_module:add_function(func)
    table.insert(self._functions, func)
    func:set_module(self)
end

function bind_module:get_functions()
    return self._functions
end

function bind_module:add_enum(enum)
    table.insert(self._enums, enum)
    enum:set_module(self)
end

function bind_module:get_enums()
    return self._enums
end

function bind_module:add_value(value)
    table.insert(self._values, value)
    value:set_module(self)
end

function bind_module:get_values()
    return self._values
end

function bind_module:get_name()
    return self._name
end

function bind_module:get_bind_name()
    return self._name:gsub('%.','_'):lower()
end

local bind_base = class(nil, 'bind_base')
function bind_base:_init(name,prefix,bind)
    self._name = name
    self._prefix = prefix
    self._bind = bind
end

function bind_base:get_bind(name)
    if self._bind then
        return self._bind[name]
    end
    return nil
end

function bind_base:get_lua_name()
    if self._bind then
        return self._bind.name or self._name
    end
    return self._name
end

function bind_base:get_policy()
    return self:get_bind('policy')
end

function bind_base:get_name()
    return self._name
end

function bind_base:get_module_name()
    local module = self._bind.module
    if module then
        return module
    end
    if self._prefix == '' then
        error('not found module for object: ' .. self:get_name())
    end
    return self._prefix:gsub('::','.'):lower()
end

function bind_base:get_prefix()
    return self._prefix or ''
end

local module_element = class(bind_base, 'module_element')
function module_element:_init(name,prefix,bind)
    bind_base._init(self, name, prefix, bind)
    self._module = nil
end

function module_element:set_module(module)
    self._module = module
end

function module_element:get_module()
    return self._module
end

local bind_class = class(module_element, 'bind_class')
function bind_class:_init(name,prefix,bind)
    bind_class.baseclass._init(self, name, prefix, bind)
    self._methods = {}
    self._fields = {}
    self._enums = {}
    self._bases = {}
end

function bind_class:set_bases(bases)
    self._bases = bases
end

function bind_class:get_bases()
    return self._bases
end


function bind_class:add_method(method)
    if method:get_name() == self:get_name() then
        if self._constructor then
            error('multiple constructors for class: ' .. self:get_name())
        end
        self._constructor = method
    else
        table.insert(self._methods, method)
    end
end

function bind_class:get_constructor()
    return self._constructor
end

function bind_class:get_methods()
    return self._methods
end

function bind_class:add_field(field)
    table.insert(self._fields, field)
end

function bind_class:get_fields()
    return self._fields
end

function bind_class:add_enum(enum)
    table.insert(self._enums, enum)
end

function bind_class:get_enums()
    return self._enums
end

function bind_class:get_bind_name()
    local prefix = self:get_prefix() or ''
    prefix = prefix:gsub('::','_')
    return prefix .. '_' .. self:get_name():lower()
end

local bind_func = class(module_element, 'bind_func')
function bind_func:_init(name,prefix,bind)
    bind_func.baseclass._init(self, name, prefix, bind)
end

local bind_method = class(bind_base, 'bind_method')
function bind_method:_init(name,prefix,bind)
    bind_method.baseclass._init(self, name, prefix, bind)
end

local bind_field = class(bind_base, 'bind_field')
function bind_field:_init(name,prefix,bind)
    bind_field.baseclass._init(self, name, prefix, bind)
end

local bind_enum = class(module_element, 'bind_enum')
function bind_enum:_init(name, prefix, bind, values, is_scoped)
    bind_enum.baseclass._init(self, name, prefix, bind)
    self._values = values or {}
    self._is_scoped = is_scoped
end

function bind_enum:get_values()
    return self._values
end

function bind_enum:is_scoped()
    return self._is_scoped
end

function bind_enum:get_lua_value_name(val_name)
    local prefix = self:get_bind('prefix')
    if prefix and val_name:sub(1, #prefix) == prefix then
        return val_name:sub(#prefix + 1)
    end
    return val_name
end

local bind_value = class(module_element, 'bind_value')
function bind_value:_init(name,prefix,bind)
    bind_value.baseclass._init(self, name, prefix, bind)
end

local processor = class(nil, 'processor')

local traverser = class(ast.traverser, 'traverser')

function traverser:_init(processor)
    traverser.baseclass._init(self)
    self._processor = processor
    self._bind_stack = {}
    self._result = {
        classes = {},
        modules = {},
    }
end

function traverser:get_result()
    return self._result
end

function traverser:_get_current_bind()
    return self._bind_stack[#self._bind_stack]
end

function traverser:traverse_class(node)
    local tags = tags.parse(node.doc or '')
    local bind = tags:collect('luabind')
    if bind then
        local current_bind = self:_get_current_bind()
        local class = bind_class.new(node.name, self:get_full_name(), bind)
        local module = self._processor:get_module(current_bind and current_bind:get_module_name() or class:get_module_name())
        module:add_class(class)
        self._result.modules[module:get_name()] = module
        table.insert(self._result.classes, class)
        table.insert(self._bind_stack, class)
        class:set_bases(node.bases or {})
    end
    traverser.baseclass.traverse_class(self, node)
    if bind then
        table.remove(self._bind_stack)
    end
end

function traverser:traverse_func(node)
    local tags = tags.parse(node.doc or '')
    local bind = tags:collect('luabind')
    if bind then
        local current_bind = self:_get_current_bind()
        if current_bind then
            local method = bind_method.new(node.name, self:get_full_name(), bind)
            current_bind:add_method(method)
        else
            local func = bind_func.new(node.name, self:get_full_name(), bind)
            local module = self._processor:get_module(func:get_module_name())
            module:add_function(func)
            self._result.modules[module:get_name()] = module
        end
    end
    traverser.baseclass.traverse_func(self, node)
end

function traverser:traverse_field(node)
    local tags = tags.parse(node.doc or '')
    local bind = tags:collect('luabind')
    if bind then
        local current_bind = self:_get_current_bind()
        if current_bind then
            local field = bind_field.new(node.name, self:get_full_name(), bind)
            current_bind:add_field(field)
        else
            local value = bind_value.new(node.name, self:get_full_name(), bind)
            local module = self._processor:get_module(value:get_module_name())
            module:add_value(value)
            self._result.modules[module:get_name()] = module
        end
    end
    traverser.baseclass.traverse_field(self, node)
end

function traverser:traverse_enum(node)
    if node.forward then return end
    local tags = tags.parse(node.doc or '')
    local bind = tags:collect('luabind')
    if bind then
        local current_bind = self:_get_current_bind()
        local enum = bind_enum.new(node.name, self:get_full_name(), bind, node.values, node.is_scoped)
        if current_bind then
            current_bind:add_enum(enum)
        else
            local module = self._processor:get_module(enum:get_module_name())
            module:add_enum(enum)
            self._result.modules[module:get_name()] = module
        end
    end
end

function processor:_init()
    self._defines = {}
    self._modules = {}
end

function processor:define(token, value)
    self._defines[token] = value
end

function processor:begin_header(filename)
    self._current_header = filename
    self._current_header_module = nil
end

function processor:get_module(name)
    if not self._modules[name] then
        local mod = bind_module.new(name)
        if self._current_header_module then
            log.error('multiple modules in header: ', self._current_header, name, self._current_header_module:get_name())
        end
        self._current_header_module = mod
        self._modules[name] = mod
        --log.info('create module: ', name)
    end
    return self._modules[name]
end

function processor:get_modules()
    return self._modules
end

function processor:process_content(data)
    local pp = preprocessor.new()
    for token, value in pairs(self._defines) do
        pp:define(token, value)
    end
    local data_pp = pp:process(data)
    local p = parser.parse(data_pp)
    local traverser = traverser.new(self)
    traverser:traverse(p)
    return traverser:get_result()
end

function processor:process_file(filename)
    self:begin_header(filename)
    local data = tostring(fs.load_file(filename))
    return self:process_content(data)
end

return processor