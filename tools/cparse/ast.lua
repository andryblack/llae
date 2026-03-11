local class = require 'llae.class'

local ast = {}

-- ── Base ──────────────────────────────────────────────────────────────────────

---@class ast_node
---@field kind string
---@field name string?
---@field doc string?
---@field template_params string|nil
---@field attributes string|nil
local ast_base = class(nil, 'ast_node')

---Check if this node has the given kind.
---@param kind string
---@return boolean
function ast_base:is(kind)
  return self.kind == kind
end

function ast_base:__tostring()
  local name = self.name or self.kind
  return string.format("[%s '%s']", self.kind, name)
end

-- ── File ──────────────────────────────────────────────────────────────────────

---@class ast_file : ast_node
---@field children table
local ast_file = class(ast_base, 'ast_file')
ast_file.kind = 'file'

---@param children table
function ast_file:_init(children)
  self.children = children or {}
end

-- ── Namespace ─────────────────────────────────────────────────────────────────

---@class ast_namespace : ast_node
---@field name string
---@field children table
local ast_namespace = class(ast_base, 'ast_namespace')
ast_namespace.kind = 'namespace'

---@param name string
---@param children table
function ast_namespace:_init(name, children)
  self.name     = name
  self.children = children or {}
end

-- ── Class / struct / union ────────────────────────────────────────────────────

---@class ast_class : ast_node
---@field name string
---@field struct_kind string   "class"|"struct"|"union"
---@field bases table
---@field children table
---@field var_name string|nil
local ast_class = class(ast_base, 'ast_class')
ast_class.kind = 'class'

---@param name string
---@param struct_kind string
---@param bases table|nil
---@param children table|nil
---@param var_name string|nil
function ast_class:_init(name, struct_kind, bases, children, var_name)
  self.name        = name
  self.struct_kind = struct_kind
  self.bases       = bases     or {}
  self.children    = children  or {}
  self.var_name    = var_name
end

---@class ast_class_forward : ast_node
---@field name string
---@field struct_kind string
local ast_class_forward = class(ast_base, 'ast_class_forward')
ast_class_forward.kind = 'class_forward'

---@param name string
---@param struct_kind string
function ast_class_forward:_init(name, struct_kind)
  self.name        = name
  self.struct_kind = struct_kind
end
-- ── Enum ──────────────────────────────────────────────────────────────────────

---@class ast_enum : ast_node
---@field name string
---@field is_scoped boolean
---@field base_type string|nil
---@field values table|nil
---@field forward boolean|nil
local ast_enum = class(ast_base, 'ast_enum')
ast_enum.kind = 'enum'

---@param name string
---@param is_scoped boolean
---@param base_type string|nil
---@param values table|nil
---@param forward boolean|nil
function ast_enum:_init(name, is_scoped, base_type, values, forward)
  self.name      = name
  self.is_scoped = is_scoped
  self.base_type = base_type
  self.values    = values
  self.forward   = forward
end

-- ── Typedef ───────────────────────────────────────────────────────────────────

---@class ast_typedef : ast_node
---@field name string
---@field type string|nil
---@field inner table|nil
local ast_typedef = class(ast_base, 'ast_typedef')
ast_typedef.kind = 'typedef'

---@param name string
---@param type_str string|nil
---@param inner table|nil
function ast_typedef:_init(name, type_str, inner)
  self.name  = name
  self.type  = type_str
  self.inner = inner
end

-- ── Using ─────────────────────────────────────────────────────────────────────

---@class ast_using : ast_node
---@field name string
---@field type string|nil
local ast_using = class(ast_base, 'ast_using')
ast_using.kind = 'using'

---@param name string
---@param type_str string|nil
function ast_using:_init(name, type_str)
  self.name = name
  self.type = type_str
end

-- ── Using namespace ───────────────────────────────────────────────────────────

---@class ast_using_namespace : ast_node
---@field name string
local ast_using_namespace = class(ast_base, 'ast_using_namespace')
ast_using_namespace.kind = 'using_namespace'

---@param name string
function ast_using_namespace:_init(name)
  self.name = name
end

-- ── Function ──────────────────────────────────────────────────────────────────

---@class ast_func : ast_node
---@field name string
---@field return_type string
---@field params table
---@field qualifiers table
local ast_func = class(ast_base, 'ast_func')
ast_func.kind = 'function'

---@param name string
---@param return_type string
---@param params table
---@param qualifiers table
function ast_func:_init(name, return_type, params, qualifiers)
  self.name        = name
  self.return_type = return_type
  self.params      = params     or {}
  self.qualifiers  = qualifiers or {}
end

-- ── Field ─────────────────────────────────────────────────────────────────────

---@class ast_field : ast_node
---@field name string
---@field type string
---@field qualifiers table
local ast_field = class(ast_base, 'ast_field')
ast_field.kind = 'field'

---@param name string
---@param type_str string
---@param qualifiers table
function ast_field:_init(name, type_str, qualifiers)
  self.name       = name
  self.type       = type_str
  self.qualifiers = qualifiers or {}
end

-- ── Access specifier ──────────────────────────────────────────────────────────

---@class ast_access : ast_node
---@field access string   "public"|"protected"|"private"
local ast_access = class(ast_base, 'ast_access')
ast_access.kind = 'access'

---@param access string
function ast_access:_init(access)
  self.access = access
  self.name   = access
end

-- ── Extern block ──────────────────────────────────────────────────────────────

---@class ast_extern_block : ast_node
---@field linkage string
---@field children table
local ast_extern_block = class(ast_base, 'ast_extern_block')
ast_extern_block.kind = 'extern_block'

---@param linkage string
---@param children table
function ast_extern_block:_init(linkage, children)
  self.linkage  = linkage
  self.children = children or {}
  self.name     = linkage
end

-- ── Exports ───────────────────────────────────────────────────────────────────

ast.base            = ast_base
ast.file            = ast_file
ast.namespace       = ast_namespace
ast["class"]        = ast_class
ast.class_forward = ast_class_forward
ast.enum            = ast_enum
ast.typedef         = ast_typedef
ast.using           = ast_using
ast.using_namespace = ast_using_namespace
ast.func            = ast_func
ast.field           = ast_field
ast.access          = ast_access
ast.extern_block    = ast_extern_block

local traverser = class(nil, 'traverser')

function traverser:_init()
  self._stack = {}
end

function traverser:_push(node)
  table.insert(self._stack, node)
end

function traverser:_pop()
  return table.remove(self._stack)
end

function traverser:get_full_name()
  local res = {}
  for _, node in ipairs(self._stack) do
    if node.is_a[ast_namespace] then
      table.insert(res, node.name)
    elseif node.is_a[ast_class] then
      table.insert(res, node.name)
    end
  end
  return table.concat(res, "::")
end

function traverser:traverse(node)
  assert(node)
  assert(node.is_a)
  if node.is_a[ast_file] then
    self:traverse_file(node)
  elseif node.is_a[ast_namespace] then
    self:traverse_namespace(node)
  elseif node.is_a[ast_class] then
    self:traverse_class(node)
  elseif node.is_a[ast_enum] then
    self:traverse_enum(node)
  elseif node.is_a[ast_typedef] then
    self:traverse_typedef(node)
  elseif node.is_a[ast_using] then
    self:traverse_using(node)
  elseif node.is_a[ast_using_namespace] then
    self:traverse_using_namespace(node)
  elseif node.is_a[ast_func] then
    self:traverse_func(node)
  elseif node.is_a[ast_field] then
    self:traverse_field(node)
  elseif node.is_a[ast_access] then
    self:traverse_access(node)
  elseif node.is_a[ast_extern_block] then
    self:traverse_extern_block(node)
  elseif node.is_a[ast_class_forward] then
    self:traverse_class_forward(node)
  else
    error('traverser: unknown node type: ' .. node.kind)
  end
end

function traverser:_traverse_children(node)
  self:_push(node)
  for _, child in ipairs(node.children) do
    self:traverse(child)
  end
  self:_pop()
end

---@param node ast_file
function traverser:traverse_file(node)
  self:_traverse_children(node)
end

---@param node ast_namespace
function traverser:traverse_namespace(node)
  self:_traverse_children(node)
end

---@param node ast_class
function traverser:traverse_class(node)
  self:_traverse_children(node)
end

---@param node ast_enum
function traverser:traverse_enum(node)
end

---@param node ast_typedef
function traverser:traverse_typedef(node)
  
end

---@param node ast_using
function traverser:traverse_using(node)
  
end

---@param node ast_using_namespace
function traverser:traverse_using_namespace(node)
  
end

---@param node ast_field
function traverser:traverse_field(node)
  
end

---@param node ast_access
function traverser:traverse_access(node)
  
end

---@param node ast_extern_block
function traverser:traverse_extern_block(node)
  self:_traverse_children(node)
end

---@param node ast_func
function traverser:traverse_func(node)
  
end

---@param node ast_class_forward
function traverser:traverse_class_forward(node)
end

ast.traverser = traverser

return ast
