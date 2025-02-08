
local res = {}

local convert_values

convert_values = function(array, fn, pg)
  for idx, v in ipairs(array) do
    if v == "NULL" then
      array[idx] = pg.NULL
    elseif fn then
      array[idx] = fn(v)
    else
      array[idx] = v
    end
  end
  return array
end

function res.decode_array(str, convert_fn, pg)
  local out = {}
  if not str:match('^%b{}$') then
    error('failed to parse postgresql array')
  end
  for word in string.gmatch(str:sub(2,-2), '([^,]+)') do
      table.insert(out,word)
  end
  return convert_values(out, convert_fn, pg)
end


return res
