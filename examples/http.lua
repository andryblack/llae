
package.path = package.path .. ';scripts/?.lua'


local llae = require 'llae'
local http = require 'llae.http'
local json = require 'json'

assert(http.createServer(function (req, res)
  --print('received request')
  local body = [[
<html>
<body>
Hello world! ]] ..os.time().. '\n'.. json.encode({
	HOME = os.getenv('HOME')
	})..[[
</body>
</html>
  ]]
  res:set_header("Content-Type", "text/html")
  res:finish(body)
end):listen(1337, '127.0.0.1'))

print('Server running at http://127.0.0.1:1337/')
