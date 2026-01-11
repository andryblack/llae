
local utils = require 'llae.utils'
args = utils.parse_args(...)

if args.verbose then
	(require 'llae.log').set_verbose(true)
end

local main = nil -- embedded

<% if next(project:get_config_value('llae','lua_path') or {}) then %>
	local fs = require 'llae.fs'
	local path = require 'llae.path'
	local tokens = {
		cwd = fs.cwd(),
		exedir = path.dirname(fs.exepath()),
	}
	package.path = utils.replace_tokens('<%- table.concat(project:get_config_value('llae','lua_path'),';') %>',tokens)
	table.remove(package.searchers,1)
	main = assert(package.searchpath('main',package.path))
<% else %>
if args.dev then
	package.path = 'scripts/?.lua;build/scripts/?.lua'
	table.remove(package.searchers,1)
	main = 'scripts/main.lua'
elseif args.root then
	package.path = args.root .. '/?.lua'
	table.remove(package.searchers,1)
	main = args.root .. '/main.lua'
end
if args.main then
	main = args.main
end
<% end %>


<% for _,v in ipairs(project:get_config_value('llae','lua_main') or {}) do %>
<%- v %>
<% end %>

if main then
	dofile(main)
else
	require('main')
end
