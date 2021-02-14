package.path = 'scripts/?.lua;build/scripts/?.lua'

local utils = require 'llae.utils'
args = utils.parse_args(...)

if args.verbose then
	(require 'llae.log').set_verbose(true)
end

require( 'main' )
