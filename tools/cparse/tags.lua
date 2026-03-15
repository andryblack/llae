local class = require 'llae.class'


-- ── tags class ────────────────────────────────────────────────────────────────

---@class tags_entry
---@field tag string
---@field value table

---@class tags
---@field _list tags_entry[]
---@field new fun(list: tags_entry[]): tags
local tags = class(nil, 'tags')

---@param list tags_entry[]
function tags:_init(list)
  self._list = list or {}
  self._clean = {}
end

function tags:get_clean()
  return next(self._clean) and self._clean or nil
end


---Returns all entries with the given tag name.
---@param name string
---@return tags_entry[]
function tags:get(name)
  local result = {}
  for _, entry in ipairs(self._list) do
    if entry.tag == name then
      table.insert(result, entry)
    end
  end
  return result
end

---Returns the first entry with the given tag name, or nil.
---@param name string
---@return tags_entry|nil
function tags:first(name)
  for _, entry in ipairs(self._list) do
    if entry.tag == name then
      return entry
    end
  end
  return nil
end

---Returns true if at least one tag with the given name exists.
---@param name string
---@return boolean
function tags:has(name)
  for _, entry in ipairs(self._list) do
    if entry.tag == name then
      return true
    end
  end
  return false
end

---ipairs-like iterator over all entries with the given tag name.
---@param name string
---@return function, table, number
function tags:foreach(name)
  local matched = self:get(name)
  return ipairs(matched)
end

---Returns the single entry with the given name, or nil if absent.
---Raises an error if more than one tag with that name exists.
---@param name string
---@return tags_entry|nil
function tags:check_unique(name)
  local matched = self:get(name)
  if #matched > 1 then
    error(string.format("tag '@%s' declared multiple times (%d times)", name, #matched))
  end
  return matched[1]
end

---Merges all value tables from every entry with the given name into one table.
---Later entries overwrite earlier ones on key conflicts.
---Returns nil if no tags with that name exist.
---@param name string
---@return table|nil
function tags:collect(name)
  local result = nil
  for _, entry in ipairs(self._list) do
    if entry.tag == name then
      if not result then result = {} end
      for k, v in pairs(entry.value) do
        result[k] = v
      end
    end
  end
  return result
end

-- ── Parsing ───────────────────────────────────────────────────────────────────

local function trim(s)
  return s:match('^%s*(.-)%s*$')
end

-- Parses the content inside parentheses into a value table.
-- Supports positional args: "arg1,arg2" -> { "arg1", "arg2" }
-- Supports named args:      "k1=v1,k2=v2" -> { k1="v1", k2="v2" }
local function parse_args(s)
  local value = {}
  if s == '' then
    return value
  end
  for raw in (s .. ','):gmatch('([^,]*),') do
    local arg = trim(raw)
    if arg ~= '' then
      local k, v = arg:match('^([%a_][%w_]*)%s*=%s*(.+)$')
      if k then
        value[k] = trim(v)
      else
        table.insert(value, arg)
      end
    end
  end
  return value
end

-- Scans plain text for @tagname and @tagname(...) annotations.
-- Non-tag text is accumulated; after parsing, self._clean holds
-- trimmed non-empty lines with all tag annotations removed.
function tags:_parse(text)
  local list = self._list
  local i = 1
  local n = #text
  local segment_start = 1
  local clean_parts = {}

  local function flush_clean(upto)
    if upto > segment_start then
      table.insert(clean_parts, text:sub(segment_start, upto - 1))
    end
  end

  while i <= n do
    if text:sub(i, i) == '@' then
      -- read tag name: must start with letter or underscore
      local j = i + 1
      if j <= n and text:sub(j, j):match('[%a_]') then
        flush_clean(i)
        local name_start = j
        j = j + 1
        while j <= n and text:sub(j, j):match('[%w_]') do
          j = j + 1
        end
        local name = text:sub(name_start, j - 1)
        local value = {}
        -- optionally consume (...)
        if j <= n and text:sub(j, j) == '(' then
          local close = text:find(')', j + 1, true)
          if close then
            value = parse_args(text:sub(j + 1, close - 1))
            j = close + 1
          end
        end
        -- capture inline trailing text as comment: stop at next @tag or newline
        local k = j
        while k <= n and text:sub(k, k) ~= '\n' do
          if text:sub(k, k) == '@' and k + 1 <= n and text:sub(k + 1, k + 1):match('[%a_]') then
            break
          end
          k = k + 1
        end
        local comment = trim(text:sub(j, k - 1))
        table.insert(list, { tag = name, value = value, comment = comment ~= '' and comment or nil })
        segment_start = k
        i = k
      else
        i = i + 1
      end
    else
      i = i + 1
    end
  end
  flush_clean(n + 1)

  -- split accumulated clean text into trimmed non-empty lines,
  -- stripping leading comment markers: /**, ///, lone *
  local clean_text = table.concat(clean_parts)
  for line in (clean_text .. '\n'):gmatch('([^\n]*)\n') do
    local t = trim(line)
    -- skip closing comment marker */
    if t == '*/' then goto continue end
    -- strip opening comment markers: /**, ///, lone *
    t = t:match('^/%*%*%s*(.*)$') or t:match('^///%s*(.*)$') or t:match('^%*/?%s*(.-)%s*$') or t
    -- strip trailing closing marker */
    t = t:match('^(.-)%s*%*/$') or t
    t = trim(t)
    if t ~= '' then
      table.insert(self._clean, t)
    end
    ::continue::
  end

  return list
end

-- ── Public API ────────────────────────────────────────────────────────────────

---Parses @tag annotations from plain text and returns a tags instance.
---@param text string
---@return tags
function tags.parse(text)
  local tags = tags.new()
  tags:_parse(text)
  return tags
end


return tags
