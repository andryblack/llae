package.path = package.path .. ';scripts/?.lua'

local web = require 'web.application'
local fs = require 'llae.fs'
local path = require 'llae.path'
local class = require 'llae.class'
local log = require 'llae.log'
local app = web.new()

app:use(web.static('./examples',{}))
app:use(web.views('./examples',{
	env = { title = 'Example' }
}))

app:use(web.cookie{})
app:use(web.formparser{})

local auth_mittleware = class(require 'web.middle')
local auth_token = '123456'

function auth_mittleware:use(web)
	web:register_handler(function(req,res)
		local cookie = req:get_cookie('_auth')
		log.info('auth get',req:get_path(),'['..cookie..']')
		if cookie ~= auth_token then
			if req:get_path()~='/login' then
				log.info('redirect')
				return res:redirect('/login')
			else
				log.info('login')
			end
		else
			log.info('authentiticated')
		end
		
	end)
end

app:use(auth_mittleware.new())

app:get('/',function(req,res)
	 local body = [[
<html>
<body>
	<form method="post" action="/logout">
		<input type="submit" value="Logout">
	</form>
</body>
</html>
  ]]
  res:set_header("Content-Type", "text/html")
  return res:finish(body)
end)

app:get('/login',function(req,res)
	 local body = [[
<html>
<body>
	<form method="post">
		<label for="GET-login">Name:</label>
		<input id="GET-login" type="text" name="login">
		<label for="GET-pass">Pass:</label>
		<input id="GET-pass" type="text" name="pass">
		<input type="submit" value="Login">
	</form>
</body>
</html>
  ]]
  res:set_header("Content-Type", "text/html")
  return res:finish(body)
end)

app:post('/login',function(req,res)
  --res:set_header("Content-Type", "text/html")
  if req.form then
  	if req.form.login == 'root' and 
  		req.form.pass == 'admin' then
  		res:set_cookie('_auth',auth_token)
  		log.info('auth success')
  	else
  		log.info('auth failed',req.form.login,req.form.pass)
  	end
  end
  return res:redirect('/')
end)

app:post('/logout',function(req,res)
  --res:set_header("Content-Type", "text/html")
  res:set_cookie('_auth','invalid')
  return res:redirect('/')
end)


app:listen{port=1337}