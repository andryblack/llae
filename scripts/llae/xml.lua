local xml = require 'xml'
local class = require 'llae.class'

local lxml = {}
for n,v in pairs(xml) do
	lxml[n]=v
end

local mt = {}

local node_mt = class(nil,'xml.node')


local element_mt = class(node_mt,'xml.element')

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

local document_mt = class(element_mt,'xml.document')

function document_mt:get_root(  )
	return self.childs[1]
end

mt[xml.node_type.element] = element_mt
mt[xml.node_type.document] = document_mt

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