local utils = {}
local fs = require 'llae.fs'
local log = require 'llae.log'

---@param url string
---@param dst string
---@param h crypto.md?
---@param p llae.log.progress?
---@return boolean,string?
local function download_file_impl(url,dst,headers,h,p)
	local uri = (require 'net.url').parse(url)
	fs.unlink(dst)
	if uri.scheme == 'ftp' then
		local ftp = (require 'net.ftp').new()
		assert(ftp:connect(uri.host,uri.port))
		local loaded = 0
		
		ftp:getfile(uri.path,dst,function(ch,total)
			loaded = loaded + #ch
			if h and #ch>0 then
				assert(h:update(ch))
			end
            if p then
                p:update(loaded,total)
            end
		end)
        if p then
            p:close()
        end
        return true
	elseif uri.scheme == 'http' or uri.scheme == 'https' then
		local http = require 'net.http'
		local req = http.createRequest{
			method = 'GET',
			url = url,
			headers = headers
		}

		local resp = assert(req:exec())
		local code = resp:get_code()
		if code ~= 200 then
			resp:close()
            return false, code .. ':' .. resp:get_message()
		end
		local f = assert(fs.open(dst,fs.O_WRONLY|fs.O_CREAT))
		local loaded = 0
		local total = tonumber(resp:get_header('Content-Length'))
		--log.debug('total:',total)
		while true do
			local ch,err = resp:read()
			if not ch then
				if err then
					return false, err
				end
                if p then
				    p:close()
                end
				break
			end
			if #ch > 0 then
				if h then
					assert(h:update(ch))
				end
				f:write(ch)
			end
			loaded = loaded + #ch
            if p then
                p:update(loaded,total)
            end
		end
		f:close()
        return true
    else
        return false, 'unsupported scheme'
	end
end

---@class net.utils.download_file_options
---@field hash string?
---@field hash_type string?
---@field log llae.log?
---@field progress_func fun():llae.log.progress?
---@field headers table<string,string>?

---@param url string
---@param dst string
---@param options net.utils.download_file_options?
---@return boolean,string?
function utils.download_file(url,dst,options)
    local h = nil
    if options and options.hash then
        local crypto = require 'llae.crypto'
        if fs.isfile(dst) then
            local h = assert(crypto.md.new(options.hash_type or 'MD5'))
            local data = fs.load_file(dst)
            if #data > 0 then
                assert(h:update(data))
            end
            local fhash = tostring(assert(h:finish()):hex_encode())
            if fhash == options.hash then
                if options.log then
                    options.log.info('skip, already downloaded')
                end
                return true
            else
                if options.log then
                    options.log.info('hash different, redownload',fhash,options.hash)
                end
            end
        end
        h = assert(crypto.md.new(options.hash_type or 'MD5'))
    end
    fs.unlink(dst)
    local p = nil
    if options and options.progress_func then
        p = options.progress_func()
    end
    local headers = {
        ['Accept'] = '*/*',
        ['Accept-Encoding'] = 'identity;q=1, gzip;q=0.1',
        ['User-Agent'] = 'Wget/1.24.5',
        ['Connection'] = 'close'
    }
    if options and options.headers then
        for name,value in pairs(options.headers) do
            headers[name] = value
        end
    end
    local ok,err = download_file_impl(url,dst,headers,h,p)
    if h then
        local fhash = tostring(assert(h:finish()):hex_encode())
        ---@diagnostic disable-next-line: need-check-nil
        if fhash ~= options.hash then
            ---@diagnostic disable-next-line: need-check-nil
            return false, 'hash different, got ' .. fhash .. ', expected ' .. options.hash
        end
    end
    if p then
        p:close()
    end
    return ok,err
end

return utils