#include "lua/bind.h"
<% for _,header in ipairs(module:get_headers()) do %>
#include "<%= header %>"<% end %>

<% for _,class in ipairs(module:get_classes()) do %>
void luabind_<%= class:get_bind_name() %>(lua::state& l);<% end %>

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
    lua::bind::function(l,"<%= func:get_lua_name() %>",&<%= func:get_prefix() %>::<%= func:get_name() %><% if func:get_policy() then %>,lua::bind::<%= func:get_policy() %><% end %>);<% end %>
    <% for _,enum in ipairs(module:get_enums()) do %>
    l.newtable();
    <% for _,val in ipairs(enum:get_values()) do %>
    lua::bind::value(l, "<%= enum:get_lua_value_name(val.name) %>", <%= enum:get_prefix() %>::<% if enum:is_scoped() then %><%= enum:get_name() %>::<% end %><%= val.name %>);<% end %>
    l.setfield(-2, "<%= enum:get_lua_name() %>");
    <% end %>
    return 1;
}
