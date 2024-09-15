
<% local args = project:get_cmdargs() 
if args['deploy-root'] then %>
	LLAE_ROOT = "<%= args['deploy-root'] %>"
<% end %>
LLAE_VERSION="0.1"
