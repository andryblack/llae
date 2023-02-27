project 'llae-examples'

module 'llae'
module 'lz4'
module 'xz'
module 'redis'

--config('llae','embed_scripts','build/scripts')
config('llae','embed_script','main.lua')