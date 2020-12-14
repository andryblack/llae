package.path = 'scripts/?.lua;build/scripts/?.lua'

local utils = require 'llae.utils'
args = utils.parse_args(...)

require( 'main' )
