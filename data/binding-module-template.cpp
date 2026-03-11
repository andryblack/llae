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
    return 1;
}
