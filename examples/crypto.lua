local async = require 'llae.async'
local crypto = require 'llae.crypto'
local log = require 'llae.log'

async.run(function()
    local md5 = assert(crypto.md.new('MD5'))
	assert(md5:update('Hello, World!'))
	local hash = assert(md5:finish())
	log.info('md5',hash)
end)