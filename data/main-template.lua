
local utils = require 'llae.utils'
args = utils.parse_args(...)

<% if project:get_config('llae','lua_path') then %>
	local uv = require 'uv'
	local path = require 'llae.path'
	local tokens = {
		cwd = uv.cwd(),
		exedir = path.dirname(uv.exepath()),
	}
	package.path = utils.replace_tokens('<%- table.concat(project:get_config('llae','lua_path'),';') %>',tokens)
	table.remove(package.searchers,1)
<% else %>
if args.dev then
	package.path = 'scripts/?.lua;build/scripts/?.lua'
	table.remove(package.searchers,1)
elseif args.root then
	package.path = args.root .. '/?.lua'
	table.remove(package.searchers,1)
end
<% end %>

if args.verbose then
	(require 'llae.log').set_verbose(true)
end

require( 'main' )
