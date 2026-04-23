local class = require 'llae.class'
local fs = require 'llae.fs'
local log = require 'llae.log'
local preprocessor = require 'cparse.preprocessor'
local parser = require 'cparse.parser'
local ast = require 'cparse.ast'
local tags = require 'cparse.tags'

local resolve_collector = class(nil, 'resolve_collector')
function resolve_collector:_init()
    self._replace = {}
end

function resolve_collector:add_using(node)
    self._replace[node.name] = node.type
    self._replace['const ' .. node.name .. ' &'] = node.type
    --log.info('add using: ', node.name, node.type)
end

function resolve_collector:resolve(type_str)
    return self._replace[type_str]
end

local bind_module = class(nil, 'bind_module')

function bind_module:_init(name)
    self._name = name
    self._classes = {}
    self._functions = {}
    self._enums = {}
    self._headers = {}
    self._values = {}
    self._resolve = resolve_collector.new()
end

function bind_module:add_header(header)
    table.insert(self._headers, header)
end

function bind_module:add_using(node)
    self._resolve:add_using(node)
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

local lua_type_map = {
    ['extern_constant'] = 'any',
    ['unknown_binding_type'] = 'any',
    ['int'] = 'integer',
    ['size_t'] = 'integer',
    ['float'] = 'number',
    ['double'] = 'number',
    ['bool'] = 'boolean',
    ['std :: string'] = 'string',
    ['void'] = 'none',
    ['llae :: buffer_base_ptr'] = {'string|llae.buffer_base?','llae.buffer_base?'},
    ['llae :: buffer_base_ptr &'] = {'string|llae.buffer_base?','llae.buffer_base?'},
    ['llae :: buffer_base_ptr &&'] = {'string|llae.buffer_base?','llae.buffer_base?'},
    ['llae :: buffer_view'] = {'string|llae.buffer_base','string'},
    ['const llae :: buffer_view &'] = {'string|llae.buffer_base','string'},
}

local function resolve_lua_type(type_str,recursive,is_return)
    local inner = string.match(type_str, 'std :: optional <%s*(.-)%s*>')
    if inner then
        return recursive(inner,is_return) .. '?'
    end
    inner = string.match(type_str, 'common :: intrusive_ptr <%s*(.-)%s*>')
    if inner then
        return recursive(inner,is_return) .. '?'
    end
    return recursive(type_str,is_return)
end

function bind_module:resolve_lua_type_inner(type_str,is_return)
    for _,enum in ipairs(self:get_enums()) do
        if enum:get_name() == type_str then
            return self:get_name() .. '.' .. enum:get_lua_name()
        end
    end
    local res = lua_type_map[type_str]
    if res then
        if is_return then
            return res[2]
        else
            return res[1]
        end
    end
    return type_str
end

function bind_module:resolve_lua_type(type_str,is_return)
    local resolved = self._resolve:resolve(type_str)
    if resolved then
        --log.info('resolve lua type: ', type_str, ' -> ', resolved)
        type_str = resolved
    end
    return resolve_lua_type(type_str,function(inner,is_return)
        return self:resolve_lua_type_inner(inner,is_return)
    end,is_return)
end

local bind_base = class(nil, 'bind_base')
function bind_base:_init(node,prefix,bind)
    self._node = node
    self._name = node.name or error('need name for bind object')
    self._prefix = prefix
    self._bind = bind
end

function bind_base:set_tags(tags)
    self._tags = tags
end

function bind_base:get_tags()
    return self._tags
end

function bind_base:get_lua_comments()
    local clean = self._tags:get_clean()
    if clean then
        local res = {}
        for _, v in ipairs(clean) do
            table.insert(res, '--- ' .. v)
        end
        return table.concat(res, '\n')
    end
    return '--- ' .. self:get_lua_name()
end

function bind_base:get_ast_node()
    return self._node
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
function module_element:_init(node,prefix,bind)
    bind_base._init(self, node, prefix, bind)
    self._module = nil
end

function module_element:set_module(module)
    self._module = module
end

function module_element:get_module()
    return self._module
end

local bind_class = class(module_element, 'bind_class')
function bind_class:_init(node,prefix,bind)
    bind_class.baseclass._init(self, node, prefix, bind)
    self._methods = {}
    self._fields = {}
    self._enums = {}
    self._bases = {}
    self._resolve = resolve_collector.new()
end

function bind_class:set_bases(bases)
    self._bases = bases
end

function bind_class:get_bases()
    return self._bases
end

function bind_class:add_using(node)
    self._resolve:add_using(node)
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
    enum:set_class(self)
    enum:set_module(self._module)
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

function bind_class:resolve_lua_type_inner(type_str,is_return)
    local resolved = self._resolve:resolve(type_str)
    if resolved then
        type_str = resolved
    end
    
    for _,enum in ipairs(self:get_enums()) do
        if enum:get_name() == type_str then
            return self._module:get_name() .. '.' .. self:get_lua_name() .. '.' .. enum:get_lua_name()
        end
    end
    return self._module:resolve_lua_type(type_str,is_return)
end

function bind_class:resolve_lua_type(type_str,is_return)
    local resolved = self._resolve:resolve(type_str)
    if resolved then
        type_str = resolved
    end
    return resolve_lua_type(type_str,function(inner,is_return)
        return self:resolve_lua_type_inner(inner,is_return)
    end,is_return)
end

local bind_func = class(module_element, 'bind_func')
function bind_func:_init(node,prefix,bind)
    bind_func.baseclass._init(self, node, prefix, bind)
end

local param_lua_skip_types = {
    ['llae :: app &'] = true,
    ['lua :: state &'] = true,
}
local function skip_param_for_lua(param)
    if param_lua_skip_types[param.type] then
        return true
    end
    return false
end

local function get_lua_parameters(params,func)
    local res = {}
    local tags = func:get_tags():get('lparam')
    if tags and next(tags) then
        for _, tag in ipairs(tags) do
            table.insert(res, tag.value[1])
        end
        return res
    end
    for _, param in ipairs(params) do
        if not skip_param_for_lua(param) then
            table.insert(res, param.name)
        end
    end
    return res
end

local function get_lua_args(params,func)
    local res = {}
    local tags = func:get_tags():get('lparam')
    if tags and next(tags) then
        for _, tag in ipairs(tags) do
            table.insert(res, {
                name = tag.value[1],
                type = tag.value[2],
                descr = tag.comment or ''
            })
        end
        return res
    end
    for _, param in ipairs(params) do
        if not skip_param_for_lua(param) then
            table.insert(res, {
                name = param.name,
                type = func:resolve_lua_type(param.type,false),
                descr = ''
            })
        end
    end
    return res
end

local function get_lua_results(return_type,async,func)
    local res = {}
    if not return_type then
        return {}
    end
    local tags = func:get_tags():get('lreturn')
    if tags and next(tags) then
        for _, tag in ipairs(tags) do
            table.insert(res, {
                type = tag.value[2],
                name = tag.value[1],
            })
        end
        return res
    end
    if return_type == 'void' then
        return {}
    end
    if return_type == 'lua :: multiret' then
        return {
            {
                type = 'any',
                name = 'result',
            }
        }
    end
    local res = {}
    -- llae::result_promise_ptr<void>
    -- llae::result<>
    local inner = string.match(return_type, 'llae :: result <%s*(.-)%s*>')
    if inner then
        if inner == 'void' then
            inner = 'boolean'
        else
            inner = func:resolve_lua_type(inner,true)
            if inner:sub(-1) == '?' then
                inner = inner:sub(1,-2)
            end
        end
        return {
            {
                type = inner .. '?',
                name = 'result',
            },
            {
                type = 'string?',
                name = 'error',
            }
        }
    end
    inner = string.match(return_type, 'llae :: result_promise_ptr <%s*(.-)%s*>')
    if inner then
        if inner == 'void' then
            inner = 'boolean'
        else
            inner = func:resolve_lua_type(inner,true)
            if inner:sub(-1) == '?' then
                inner = inner:sub(1,-2)
            end
        end
        if not async then
            return {
                {
                    type = inner .. '?',
                    name = 'result',
                },
                {
                    type = 'string?',
                    name = 'error',
                }
            }
        end
        return {{
            type = 'llae.promise<' .. inner .. '>',
            name = 'promise',
        }}
    end
    return res
end

function bind_func:get_lua_parameters()
    return get_lua_parameters(self._node.params or {},self)
end
function bind_func:get_lua_args()
    return get_lua_args(self._node.params or {},self)
end
function bind_func:get_lua_results(async)
    return get_lua_results(self._node.return_type,async,self)
end

function bind_func:resolve_lua_type(type_str,is_return)
    return self._module:resolve_lua_type(type_str,is_return)
end

local bind_method = class(bind_base, 'bind_method')
function bind_method:_init(node,prefix,bind)
    bind_method.baseclass._init(self, node, prefix, bind)
end

function bind_method:get_lua_parameters()
    return get_lua_parameters(self._node.params or {},self)
end

function bind_method:get_lua_results(async)
    return get_lua_results(self._node.return_type,async,self)
end

function bind_method:get_lua_args()
    return get_lua_args(self._node.params or {},self)
end

function bind_method:resolve_lua_type(type_str,is_return)
    return self._class:resolve_lua_type(type_str,is_return)
end

function bind_method:is_static()
    if self:get_bind('method') then
        return false
    end
    return self._node.qualifiers.static
end

function bind_method:set_class(cls)
    self._class = cls
end

local bind_field = class(bind_base, 'bind_field')
function bind_field:_init(node,prefix,bind)
    bind_field.baseclass._init(self, node, prefix, bind)
end
function bind_field:get_type()
    if self._bind and self._bind.type then
        return self._bind.type
    end
    return self._node.type
end

local bind_enum = class(module_element, 'bind_enum')
function bind_enum:_init(node, prefix, bind, values, is_scoped)
    bind_enum.baseclass._init(self, node, prefix, bind)
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

function bind_enum:set_class(class_ref)
    self._class = class_ref
end

function bind_enum:get_lua_local_name()
    local parts = {}
    if self._module then
        table.insert(parts, self._module:get_name())
    end
    if self._class then
        table.insert(parts, self._class:get_name())
    end
    table.insert(parts, self:get_lua_name())
    return table.concat(parts, '_'):gsub('[^%w_]', '_')
end

local function normalize_enum_value(value)
    value = tostring(value):gsub('^%s+', ''):gsub('%s+$', '')
    local as_number = tonumber(value)
    if as_number ~= nil then
        return tostring(as_number)
    end
    if value:sub(1, 2) == '0x' or value:sub(1, 2) == '0X' then
        local hex = tonumber(value:sub(3), 16)
        if hex ~= nil then
            return tostring(hex)
        end
    end
    return value
end

local function enum_entry_value(base_value, implicit_offset)
    local base_number = tonumber(base_value)
    if base_number ~= nil then
        return tostring(base_number + implicit_offset)
    end
    if implicit_offset == 0 then
        return base_value
    end
    return '(' .. base_value .. ') + ' .. tostring(implicit_offset)
end

function bind_enum:get_lua_values()
    ---@type {name:string, value:string}[]
    local values = {}
    local base_value = nil
    local implicit_offset = 0
    for _, enum_value in ipairs(self._values) do
        local resolved_value = enum_value.value
        if resolved_value then
            base_value = normalize_enum_value(resolved_value)
            implicit_offset = 0
            resolved_value = base_value
        else
            if not base_value then
                base_value = '0'
                implicit_offset = 0
            else
                implicit_offset = implicit_offset + 1
            end
            resolved_value = enum_entry_value(base_value, implicit_offset)
        end
        table.insert(values, {
            name = self:get_lua_value_name(enum_value.name),
            value = resolved_value,
        })
    end
    return values
end

local bind_value = class(module_element, 'bind_value')
function bind_value:_init(node,prefix,bind)
    bind_value.baseclass._init(self, node, prefix, bind)
end
function bind_value:get_type()
    return self._node.type
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
    local node_tags = node.tags or tags.new()
    local bind = node_tags:collect('luabind')
    if bind then
        local current_bind = self:_get_current_bind()
        local class = bind_class.new(node, self:get_full_name(), bind)
        local module = self._processor:get_module(current_bind and current_bind:get_module_name() or class:get_module_name())
        module:add_class(class)
        self._result.modules[module:get_name()] = module
        table.insert(self._result.classes, class)
        table.insert(self._bind_stack, class)
        class:set_bases(node.bases or {})
        class:set_tags(node_tags)
    end
    traverser.baseclass.traverse_class(self, node)
    if bind then
        table.remove(self._bind_stack)
    end
end

function traverser:get_module_name()
    local current_bind = self:_get_current_bind()
    if current_bind then
        return current_bind:get_module_name()
    end
    local prefix = self:get_full_name()
    prefix = prefix:gsub('::','.'):lower()
    return prefix
end

function traverser:traverse_using(node)
    local node_tags = node.tags or tags.new()
    local bind = node_tags:collect('luabind')
    if bind then
        local current_bind = self:_get_current_bind()
        local module = self._processor:get_module(current_bind or self:get_module_name())
        if current_bind then
            current_bind:add_using(node)
        else
            module:add_using(node)
        end
    end
    traverser.baseclass.traverse_using(self, node)
end

function traverser:traverse_func(node)
    local node_tags = node.tags or tags.new()
    local bind = node_tags:collect('luabind')
    if bind then
        local current_bind = self:_get_current_bind()
        if current_bind then
            local method = bind_method.new(node, self:get_full_name(), bind)
            current_bind:add_method(method)
            method:set_tags(node_tags)
            method:set_class(current_bind)
        else
            local func = bind_func.new(node, self:get_full_name(), bind)
            local module = self._processor:get_module(func:get_module_name())
            module:add_function(func)
            self._result.modules[module:get_name()] = module
            func:set_tags(node_tags)
        end
    end
    traverser.baseclass.traverse_func(self, node)
end

function traverser:traverse_field(node)
    local node_tags = node.tags or tags.new()
    local bind = node_tags:collect('luabind')
    if bind then
        local current_bind = self:_get_current_bind()
        if current_bind then
            local field = bind_field.new(node, self:get_full_name(), bind)
            current_bind:add_field(field)
            field:set_tags(node_tags)
        else
            local value = bind_value.new(node, self:get_full_name(), bind)
            local module = self._processor:get_module(value:get_module_name())
            module:add_value(value)
            self._result.modules[module:get_name()] = module
            value:set_tags(node_tags)
        end
    end
    traverser.baseclass.traverse_field(self, node)
end

function traverser:traverse_enum(node)
    if node.forward then return end
    local node_tags = node.tags or tags.new()
    local bind = node_tags:collect('luabind')
    if bind then
        local current_bind = self:_get_current_bind()
        local enum = bind_enum.new(node, self:get_full_name(), bind, node.values, node.is_scoped)
        if current_bind then
            current_bind:add_enum(enum)
        else
            local module = self._processor:get_module(enum:get_module_name())
            module:add_enum(enum)
            self._result.modules[module:get_name()] = module
        end
        enum:set_tags(node_tags)
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
    local ast = parser.parse(data_pp)
    local traverser = traverser.new(self)
    traverser:traverse(ast)
    return traverser:get_result(),ast
end

function processor:process_file(filename)
    self:begin_header(filename)
    local data = tostring(fs.load_file(filename))
    return self:process_content(data)
end

return processor