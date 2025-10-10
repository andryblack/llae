local xml = require 'xml'
local class = require 'llae.class'

---@class llae.xml : xml
local lxml = setmetatable({}, {__index=xml})

---@type table<integer,xml.node_mt>
local mt = {}

---@class xml.node_mt : xml.node
local node_mt = class(nil,'xml.node_mt')


---@class xml.element_mt : xml.node_mt
local element_mt = class(node_mt,'xml.element_mt')

function element_mt:get_attribute( name )
	return self.attributes[name]
end

function element_mt:find_child( name )
	for _,v in ipairs(self.childs) do
		if v.name == name then
			return v
		end
	end
end

function element_mt:child_value(  )
	local ch = self.childs[1]
	return ch and ch.value
end

---@class xml.document_mt : xml.element_mt
local document_mt = class(element_mt,'xml.document_mt')

function document_mt:get_root(  )
	return self.childs[1]
end

mt[xml.node_type.element] = element_mt
mt[xml.node_type.document] = document_mt

---@param node xml.node
local function assign_mt( node )
	setmetatable(node,mt[node.type] or node_mt)
	for _,v in ipairs(node.childs) do
		assign_mt(v)
	end
end

function lxml.parse( data )
	local x,e = xml.tolua(data)
	if not x then
		return x,e
	end
	assign_mt(x)
	return x
end

return lxml