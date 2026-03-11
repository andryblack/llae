#include "<%= header %>"
#include "lua/bind.h"

<% for _,class in ipairs(bindings.classes) do %>
/* class <%= class:get_prefix()%>::<%= class:get_name() %> */
void luabind_<%= class:get_bind_name() %>(lua::state& l) {
    <% local constructor = class:get_constructor() if constructor then %>
    <% if constructor:get_bind('raw') then %>
    lua::bind::raw_constructor<<%= class:get_prefix() %>::<%= class:get_name() %>>(l);
    <% else %>
    lua::bind::constructor<<%= class:get_prefix() %>::<%= class:get_name() %>>(l);
    <% end %>
    <% end %>
    <% for _,method in ipairs(class:get_methods()) do %>
    lua::bind::function(l,"<%= method:get_lua_name() %>",&<%= method:get_prefix() %>::<%= method:get_name() %><% if method:get_policy() then %>,lua::bind::<%= method:get_policy() %><% end %>);<% end %>
    <% for _,field in ipairs(class:get_fields()) do %>
    lua::bind::field<%= field:get_bind('readonly') and '_ro' or '' %>(l,"<%= field:get_lua_name() %>",&<%= field:get_prefix() %>::<%= field:get_name() %><% if field:get_policy() then %>,lua::bind::<%= field:get_policy() %><% end %>);<% end %>
}
<% end %>
