
local types = {}

types.pg_type = {
  [16] = "boolean",
  [17] = "bytea",
  [20] = "number",
  [21] = "number",
  [23] = "number",
  [700] = "number",
  [701] = "number",
  [1700] = "number",
  [114] = "json",
  [3802] = "json",
  [1000] = "array_boolean",
  [1005] = "array_number",
  [1007] = "array_number",
  [1016] = "array_number",
  [1021] = "array_number",
  [1022] = "array_number",
  [1231] = "array_number",
  [1009] = "array_string",
  [1015] = "array_string",
  [1002] = "array_string",
  [1014] = "array_string",
  [2951] = "array_string",
  [199] = "array_json",
  [3807] = "array_json"
}

types.message_type_b = {
  auth = string.byte('R'),
  parameter_status = string.byte('S'),
  backend_key = string.byte('K'),
  ready_for_query = string.byte('Z'),
  parse_complete = string.byte('1'),
  bind_complete = string.byte('2'),
  close_complete = string.byte('3'),
  row_description = string.byte('T'),
  data_row = string.byte('D'),
  command_complete = string.byte('C'),
  error = string.byte('E'),
  notice = string.byte('N'),
  notification = string.byte('A')
}

types.message_type_f = {
  password = string.byte('p'),
  query = string.byte('Q'),
  parse = string.byte('P'),
  bind = string.byte('B'),
  describe = string.byte('D'),
  execute = string.byte('E'),
  close = string.byte('C'),
  sync = string.byte('S'),
  terminate = string.byte('X')
}

types.pg_error = {
  severity =  string.byte('S'),
  code =  string.byte('C'),
  message =  string.byte('M'),
  position =  string.byte('P'),
  detail =  string.byte('D'),
  schema =  string.byte('s'),
  ['table'] =  string.byte('t'),
  constraint =  string.byte('n')
}
local function make_inverse(t)
	local keys = {}
	for k,v in pairs(t) do
		keys[v] = k
	end
	for k,v in pairs(keys) do
		t[k]=v
	end
end
make_inverse(types.pg_type)
make_inverse(types.pg_error)
make_inverse(types.message_type_b)

types.NULL = "\0"

return types