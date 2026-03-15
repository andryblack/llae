#include "lua/bind.h"
#include "llae/async_bind.h"
<% for _,header in ipairs(module:get_headers()) do %>
#include "<%= header %>"<% end %>

<% for _,class in ipairs(module:get_classes()) do %>
/* class <%= class:get_prefix()%>::<%= class:get_name() %> */
static void luabind_<%= class:get_bind_name() %>(lua::state& l) {
    <% local constructor = class:get_constructor() if constructor then %>
    <% if constructor:get_bind('raw') then %>
    lua::bind::raw_constructor<<%= class:get_prefix() %>::<%= class:get_name() %>>(l);
    <% else %>
    lua::bind::constructor<<%= class:get_prefix() %>::<%= class:get_name() %>>(l);
    <% end %>
    <% end %>
    <% for _,method in ipairs(class:get_methods()) do %>
    <%= method:get_bind('async') and 'llae::async_function' or 'lua::bind::function' %>(l,"<%= method:get_lua_name() %>",&<%= method:get_prefix() %>::<%= method:get_name() %><% if method:get_policy() then %>,lua::bind::<%= method:get_policy() %><% end %>);<% if method:get_bind('alias') then %>
    lua::bind::function(l,"<%= method:get_bind('alias') %>",&<%= method:get_prefix() %>::<%= method:get_name() %>);<% end %><% end %>
    <% for _,field in ipairs(class:get_fields()) do %>
    lua::bind::field<%= field:get_bind('readonly') and '_ro' or '' %>(l,"<%= field:get_lua_name() %>",&<%= field:get_prefix() %>::<%= field:get_name() %><% if field:get_policy() then %>,lua::bind::<%= field:get_policy() %><% end %>);<% end %>
    <% for _,enum in ipairs(class:get_enums()) do %>
    l.newtable();
    <% for _,val in ipairs(enum:get_values()) do %>
    lua::bind::value(l, "<%= enum:get_lua_value_name(val.name) %>", <%= class:get_prefix() %>::<%= class:get_name() %>::<% if enum:is_scoped() then %><%= enum:get_name() %>::<% end %><%= val.name %>);<% end %>
    l.setfield(-2, "<%= enum:get_lua_name() %>");
    <% end %>
}
<% end %>

int luaopen_<%= module:get_bind_name() %>(lua_State* L) {
    lua::state l(L);
    <% for _,class in ipairs(module:get_sorted_classes()) do %>
    lua::bind::object<<%= class:get_prefix() %>::<%= class:get_name() %>>::register_metatable(l, &luabind_<%= class:get_bind_name() %>);
    <% end %>
    l.createtable();
    <% for _,class in ipairs(module:get_classes()) do %>
    lua::bind::object<<%= class:get_prefix() %>::<%= class:get_name() %>>::get_metatable(l);
    l.setfield(-2,"<%= class:get_lua_name() %>");
    <% end %>
    <% for _,func in ipairs(module:get_functions()) do %>
    <%= func:get_bind('async') and 'llae::async_function' or 'lua::bind::function' %>(l,"<%= func:get_lua_name() %>",&<%= func:get_prefix() %>::<%= func:get_name() %><% if func:get_policy() then %>,lua::bind::<%= func:get_policy() %><% end %>);<% end %>
    <% for _,enum in ipairs(module:get_enums()) do %>
    l.newtable();
    <% for _,val in ipairs(enum:get_values()) do %>
    lua::bind::value(l, "<%= enum:get_lua_value_name(val.name) %>", <%= enum:get_prefix() %>::<% if enum:is_scoped() then %><%= enum:get_name() %>::<% end %><%= val.name %>);<% end %>
    l.setfield(-2, "<%= enum:get_lua_name() %>");
    <% end %>
    <% for _,value in ipairs(module:get_values()) do %>
    lua::bind::value(l, "<%= value:get_lua_name() %>", <%= value:get_prefix() %>::<%= value:get_name() %>);<% end %>
    return 1;
}
