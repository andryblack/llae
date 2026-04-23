---@meta <%= module:get_name() %>
<% local function format_func(func,up) %>
<%- func:get_lua_comments() %><% for _,arg in ipairs(func:get_lua_args()) do %>
---@param <%= arg.name %> <%= arg.type %> <%= arg.descr or '' %><% end
for _,result in ipairs(func:get_lua_results()) do %>
---@return <%= result.type %> <%= result.name %><% end
end
local function format_async_func(func,up) %>
<%- func:get_lua_comments() %><% for _,arg in ipairs(func:get_lua_args()) do %>
---@param <%= arg.name %> <%= arg.type %> <%= arg.descr or '' %><% end
for _,result in ipairs(func:get_lua_results(true)) do %>
---@return <%= result.type %> <%= result.name %><% end
end
local function format_enum(enum,prefix,setname) %>
<%- enum:get_lua_comments() %>
---@enum <%= prefix %>.<%= enum:get_lua_name() %>
local <%= enum:get_lua_local_name() %> = {<% 
for _,enum_value in ipairs(enum:get_lua_values()) do %>
  <%= enum_value.name %> = <%= enum_value.value %>,<% end %>
}
<%= setname %>.<%= enum:get_lua_name() %> = <%= enum:get_lua_local_name() %>
<% end
local module_name = module:get_name() %>
---@class <%= module_name %><% for _,value in ipairs(module:get_values()) do %>
---@field <%= value:get_name() %> <%= module:resolve_lua_type(value:get_type()) %><% end %>
local <%= module_name %> = {}
<% for _,enum in ipairs(module:get_enums()) do
   format_enum(enum,module_name,module_name)
end
for _,class in ipairs(module:get_sorted_classes()) do local class_name = class:get_name() %>
<%- class:get_lua_comments() %>
---@class <%= module_name %>.<%= class_name %><% for _,field in ipairs(class:get_fields()) do %>
---@field <%= field:get_name() %> <%= class:resolve_lua_type(field:get_type()) %> <%= field:get_lua_comments() %><% end %>
local <%= class_name %> = {}
<% for _,enum in ipairs(class:get_enums()) do 
   format_enum(enum,module_name .. '.' .. class_name, class_name) 
end
for _,method in ipairs(class:get_methods()) do
   format_func(method,class) %>
function <%= class_name %><%= method:is_static() and '.' or ':' %><%= method:get_lua_name() %>(<%= table.concat(method:get_lua_parameters(), ', ') %>) end
<% if method:get_bind('async') then
   format_async_func(method,class) %>
function <%= class_name %><%= method:is_static() and '.' or ':' %>async_<%= method:get_lua_name() %>(<%= table.concat(method:get_lua_parameters(), ', ') %>) end
<% end 
   end 
   end %>

<% for _,class in ipairs(module:get_classes()) do %><% name = class:get_name() %>
<%= module_name %>.<%= name %> = <%= name %><% end 
for _,func in ipairs(module:get_functions()) do 
  format_func(func,module) %>
function <%= module_name %>.<%= func:get_lua_name() %>(<%= table.concat(func:get_lua_parameters(), ', ') %>) end
<% if func:get_bind('async') then
  format_async_func(func,module) %>
function <%= module_name %>.<%= func:get_lua_name() %>_async(<%= table.concat(func:get_lua_parameters(), ', ') %>) end
<% end     
   end %>
return <%= module_name %>
