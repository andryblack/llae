---@meta xml

---@class xml
local xml = {}

---@class xml.node
---@field name string
---@field value string
---@field type integer
---@field attributes table<string, string>
---@field childs xml.node[]

---@param data string|llae.buffer_base
---@return xml.node?
---@return string?
function xml.tolua(data) end

---@class xml.node_type
local node_type = {}

node_type.null = 0
node_type.document = 1
node_type.element = 2
node_type.pcdata = 3
node_type.comment = 4
node_type.pi = 5
node_type.declaration = 6
node_type.doctype = 7

xml.node_type = node_type

return xml
