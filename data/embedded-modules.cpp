#include "lua/embedded.h"

<% for _,v in ipairs(modules) do %>
int <%= v.func %>(lua_State* L);
<% end %>

const lua::embedded_module lua::embedded_module::modules[] = {

<% for _,v in ipairs(modules) do %>
{
	"<%= v.name %>",
	&<%= v.func %>,
},
<% end %>

	{
		nullptr,
		nullptr,
	}
};
