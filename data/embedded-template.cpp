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
<% v.data_name = '_script_' .. v.name:gsub('[%.%/%-]','_') %>
<% local compressed = (require 'llae.compression').deflate(v.content)
   v.uncompressed_size = #v.content
  %>
/* <%= v.name %> */
static const unsigned char <%= v.data_name %>[] = {
<%- output_script(compressed) %>
};

<% end %>

const lua::embedded_script lua::embedded_script::scripts[] = {

<% for _,v in ipairs(scripts) do %>
{
	"<%= v.name %>",
	<%= v.data_name %>,
	sizeof(<%= v.data_name %>),
	<%= v.uncompressed_size %>,
},
<% end %>

	{
		nullptr,
		nullptr,
		0,0
	}
};
