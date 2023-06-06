project 'llae'

module 'llae'
module 'bzip2'

if cmdargs.debug then
	config('lua','apicheck',true)
end

if cmdargs.inplace then
	config('llae','embed_scripts','scripts')
	config('llae','embed_scripts','tools')
	module 'redis'
else
	config('llae','embed_scripts','build/scripts')
	config('llae','embed_scripts','build/modules/llae/llae-src/tools')
end