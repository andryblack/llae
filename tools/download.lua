local tool = require 'tool'
local class = require  'llae.class'

local cmd = class(tool)
cmd.name = 'download'
cmd.descr = 'download file from url'
cmd.args = {
	{'url','URL to download'},
	{'file','file name to save'},
	{'hash','hash to verify',optional=true},
	{'hash_type','hash type to verify (MD5, SHA1, SHA256, SHA512)',optional=true},
}
local netutils = require 'net.utils'
local log = require 'llae.log'
local async = require 'llae.async'

function cmd:exec( args )
    if not args.url then
        error('url is required')
    end
    if not args.file then
        error('file is required')
    end
    async.run(function()
        local options = {
            log = log,
            progress_func = log.progress,
            hash = args.hash,
            hash_type = args.hash_type,
        }
        local res,err = netutils.download_file(args.url,args.file,options)
        if not res then
            error(err)
        end
        return res
    end,true)
end


return cmd



