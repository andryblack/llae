local async = require 'llae.async'
local log = require 'llae.log'
local http = require 'net.http'
local crypto = require 'llae.crypto'

local url = args[1] or 'error need url'

local function download_file_impl(url,h)
	local uri = (require 'net.url').parse(url)
	
	
    local req = http.createRequest{
        method = 'GET',
        url = url,
        headers = {
            ['Accept'] = '*/*',
            ['Accept-Encoding'] = 'identity',
            ['User-Agent'] = 'Wget/1.24.5',
            ['Connection'] = 'close'
        }
    }

    local resp = assert(req:exec())
    local code = resp:get_code()
    if code ~= 200 then
        resp:close()
        error(code .. ':' .. resp:get_message())
    end
    local loaded = 0
    local total = tonumber(resp:get_header('Content-Length'))
    log.info('total:',total)
    --log.debug('total:',total)
    local p = log.progress()
    while true do
        local ch,err = resp:read()
        if not ch then
            if err then
                error(err)
            end
            p:close()
            break
        end
        if #ch > 0 then
            if h then
                assert(h:update(ch))
            end
        end
        loaded = loaded + #ch
        p:update(loaded,total)
    end
end

async.run(function()
    http.get_ssl_ctx().set_debug_threshold(99)
    local ssl = require 'ssl'
    ssl.connection.set_verbose(true)
    local h 
    if args[2] then
        h = crypto.md5()
    end
    download_file_impl(url,h)
    if h then
        local sum = tostring(assert(h:finish()):hex_encode())
        if sum ~= args[2] then
            log.error('different check sum:',sum,args[2])
            error("different checksum")
        end
    end
    log.info("download finished")
end)