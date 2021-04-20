
local utils = require 'llae.utils'
args = utils.parse_args(...)

if args.dev then
	package.path = 'scripts/?.lua;build/scripts/?.lua'
	table.remove(package.searchers,1)
elseif args.root then
	package.path = args.root .. '/?.lua'
	table.remove(package.searchers,1)
end

if args.verbose then
	(require 'llae.log').set_verbose(true)
end

require( 'main' )
