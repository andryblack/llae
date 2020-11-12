#include "lua/embedded.h"

<% 
local function output_script(data)
	local result = {}
	local length = #data
	table.insert(result,'\t')
	for i = 1, length do
		table.insert(result,string.format("0x%02x,", data:byte(i)))
		if (i % 16 == 0) and i~=length then
			table.insert(result,'\n\t')
		end
	end
	table.insert(result,'\n')
	return table.concat(result,'')
end
%>
<% for _,v in ipairs(scripts) do %>
<% v.data_name = v.name:gsub('[%.%/%-]','_') %>
/* <%= v.name %> */
static const unsigned char <%= v.data_name %>[] = {
<%- output_script(v.content) %>
};

<% end %>

const lua::embedded_script lua::embedded_script::scripts[] = {

<% for _,v in ipairs(scripts) do %>
{
	"<%= v.name %>",
	<%= v.data_name %>,
	sizeof(<%= v.data_name %>),
},
<% end %>

	{
		nullptr,
		nullptr,
		0
	}
};
