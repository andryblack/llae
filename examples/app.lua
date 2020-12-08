package.path = package.path .. ';scripts/?.lua'

local web = require 'web.application'
local fs = require 'llae.fs'
local path = require 'llae.path'
local app = web.new()

app:use(web.static('./examples',{}))
app:use(web.views('./examples',{
	env = { title = 'Example' }
}))

app:get('/',function(req,res)
	return res:render('app',{
		content = [[
<a href="test1">test1</a>
<a href="test1/test2">test2</a>
<a href="test1/test3">test3</a>
<a href="files">files</a>
]]
	})
end)

app:get('/test1',function(req,res)
	return res:render('app',{content='<a href="/">..</a>'})
end)

app:get('/test1/:sub',function(req,res)
	return res:render('app',{
		content = [[
<a href="/">..</a><p>
at: ]] .. req.params.sub .. [[
		]]
	})
end)

app:get('/info.lua',function(req,res,next)
	if req.query.raw then
		return next()
	end
	return res:render('app',{
		content = [[
<a href="/">..</a><p>
info.lua
		]]
	})
end)

app:get('/files',function(req,res)
	local r = {[[
		<a href="/">..</a><p>
]]}
	local files = fs.scandir('./examples')
			
	for _,f in ipairs(files) do
		if f.isfile then
			table.insert(r,'<a href="/files/' .. f.name..'">'.. f.name .. '</a><p>')
		end
	end
	return res:render('app',{content=table.concat(r)})
end)

app:get('/files/:file.lua',function(req,res)
	return res:render('app',{
		content = [[
<a href="/files">..</a><p>
]]..req.params.file..[[<p><a href="/]]..req.params.file..[[.lua">download</a>
		]]
	})
end)

app:get('/files/*file',function(req,res)
	return res:render('app',{
		content = [[
<a href="/files">..</a><p>
]]..req.params.file..[[<p>
		]]
	})
end)

app:listen{port=1337}