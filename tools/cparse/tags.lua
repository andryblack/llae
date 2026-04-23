local class = require 'llae.class'

-- byte constants
local B_DQUOTE = 34  -- '"'
local B_SQUOTE = 39  -- "'"
local B_BSLASH = 92  -- '\'
local B_LPAREN = 40  -- '('
local B_RPAREN = 41  -- ')'
local B_COMMA  = 44  -- ','
local B_LT     = 60  -- '<'
local B_GT     = 62  -- '>'
local B_LBRAK  = 91  -- '['
local B_RBRAK  = 93  -- ']'
local B_LBRACE = 123 -- '{'
local B_RBRACE = 125 -- '}'

---Trim leading and trailing whitespace from a string.
---@param s string?
---@return string
local function trim(s)
  return (s or ''):match('^%s*(.-)%s*$')
end

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

---Adds one clean (non-tag) doc line after trim; ignores empty values.
---@param text string|nil
function tags:add_cleantext(text)
  local t = trim(text)
  if t ~= '' then
    table.insert(self._clean, t)
  end
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

-- ── Parsing helpers (balanced (), <>, [], {}, strings) ───────────────────────


---Advance index `i` past a single- or double-quoted string starting at `i`.
---@param s string
---@param i number  index of opening quote
---@return number  index of first char after closing quote
local function skip_string(s, i)
  local len = #s
  local quote = s:byte(i)
  i = i + 1
  while i <= len do
    local b = s:byte(i)
    if b == B_BSLASH then
      i = i + 2
    elseif b == quote then
      return i + 1
    else
      i = i + 1
    end
  end
  return i
end

---Find index of first top-level comma in `s`, or nil.
---@param s string
---@return number|nil
local function find_first_top_comma(s)
  local len = #s
  local i = 1
  local dp, da, db, dbr = 0, 0, 0, 0
  while i <= len do
    local b = s:byte(i)
    if b == B_DQUOTE or b == B_SQUOTE then
      i = skip_string(s, i)
    elseif s:sub(i, i + 1) == '>>' then
      da = math.max(0, da - 2)
      i = i + 2
    elseif b == B_LPAREN then
      dp = dp + 1
      i = i + 1
    elseif b == B_RPAREN then
      dp = math.max(0, dp - 1)
      i = i + 1
    elseif b == B_LT then
      da = da + 1
      i = i + 1
    elseif b == B_GT then
      da = math.max(0, da - 1)
      i = i + 1
    elseif b == B_LBRAK then
      dbr = dbr + 1
      i = i + 1
    elseif b == B_RBRAK then
      dbr = math.max(0, dbr - 1)
      i = i + 1
    elseif b == B_LBRACE then
      db = db + 1
      i = i + 1
    elseif b == B_RBRACE then
      db = math.max(0, db - 1)
      i = i + 1
    elseif b == B_COMMA and dp == 0 and da == 0 and db == 0 and dbr == 0 then
      return i
    else
      i = i + 1
    end
  end
  return nil
end

---Find index of `)` that closes the `(` at `open_idx`.
---@param s string
---@param open_idx number  index of `(`
---@return number|nil
local function find_matching_close_paren(s, open_idx)
  local len = #s
  local i = open_idx + 1
  local dp, da, db, dbr = 1, 0, 0, 0
  while i <= len do
    local b = s:byte(i)
    if b == B_DQUOTE or b == B_SQUOTE then
      i = skip_string(s, i)
    elseif s:sub(i, i + 1) == '>>' then
      da = math.max(0, da - 2)
      i = i + 2
    elseif b == B_LPAREN then
      dp = dp + 1
      i = i + 1
    elseif b == B_RPAREN then
      dp = math.max(0, dp - 1)
      if dp == 0 and da == 0 and db == 0 and dbr == 0 then
        return i
      end
      i = i + 1
    elseif b == B_LT then
      da = da + 1
      i = i + 1
    elseif b == B_GT then
      da = math.max(0, da - 1)
      i = i + 1
    elseif b == B_LBRAK then
      dbr = dbr + 1
      i = i + 1
    elseif b == B_RBRAK then
      dbr = math.max(0, dbr - 1)
      i = i + 1
    elseif b == B_LBRACE then
      db = db + 1
      i = i + 1
    elseif b == B_RBRACE then
      db = math.max(0, db - 1)
      i = i + 1
    else
      i = i + 1
    end
  end
  return nil
end

---Split `s` on top-level commas into trimmed non-empty segments (empty `s` → {}).
---@param s string
---@return string[]
local function split_top_level_commas(s)
  s = trim(s)
  if s == '' then
    return {}
  end
  local out = {}
  local seg_start = 1
  local len = #s
  local i = 1
  local dp, da, db, dbr = 0, 0, 0, 0
  while i <= len do
    local b = s:byte(i)
    if b == B_DQUOTE or b == B_SQUOTE then
      i = skip_string(s, i)
    elseif s:sub(i, i + 1) == '>>' then
      da = math.max(0, da - 2)
      i = i + 2
    elseif b == B_LPAREN then
      dp = dp + 1
      i = i + 1
    elseif b == B_RPAREN then
      dp = math.max(0, dp - 1)
      i = i + 1
    elseif b == B_LT then
      da = da + 1
      i = i + 1
    elseif b == B_GT then
      da = math.max(0, da - 1)
      i = i + 1
    elseif b == B_LBRAK then
      dbr = dbr + 1
      i = i + 1
    elseif b == B_RBRAK then
      dbr = math.max(0, dbr - 1)
      i = i + 1
    elseif b == B_LBRACE then
      db = db + 1
      i = i + 1
    elseif b == B_RBRACE then
      db = math.max(0, db - 1)
      i = i + 1
    elseif b == B_COMMA and dp == 0 and da == 0 and db == 0 and dbr == 0 then
      local piece = trim(s:sub(seg_start, i - 1))
      if piece ~= '' then
        table.insert(out, piece)
      end
      seg_start = i + 1
      i = i + 1
    else
      i = i + 1
    end
  end
  local last = trim(s:sub(seg_start))
  if last ~= '' then
    table.insert(out, last)
  end
  return out
end

---Parse one argument segment: named `k=v` or positional.
---@param arg string
---@param value table
local function ingest_arg_segment(arg, value)
  if arg == '' then
    return
  end
  local k, v = arg:match('^([%a_][%w_]*)%s*=%s*(.+)$')
  if k then
    value[k] = trim(v)
  else
    table.insert(value, arg)
  end
end

---Parse a comma-separated argument list (commas only at nesting depth 0).
---@param s string
---@return table
function tags.parse_value_list(s)
  local value = {}
  for _, seg in ipairs(split_top_level_commas(s)) do
    ingest_arg_segment(seg, value)
  end
  return value
end

---First top-level comma splits `s` into `(first, rest)`.
---@param s string
---@return string, string
function tags.split_first_value(s)
  s = trim(s or '')
  if s == '' then
    return '', ''
  end
  local c = find_first_top_comma(s)
  if not c then
    return s, ''
  end
  return trim(s:sub(1, c - 1)), trim(s:sub(c + 1))
end

---Merge two tag lists and clean lines: entries from `a` then `b`; clean lines concatenated.
---@param a tags|nil
---@param b tags|nil
---@return tags
function tags.merge(a, b)
  a = a or tags.new()
  b = b or tags.new()
  local list = {}
  for _, e in ipairs(a._list) do
    table.insert(list, { tag = e.tag, value = e.value, comment = e.comment })
  end
  for _, e in ipairs(b._list) do
    table.insert(list, { tag = e.tag, value = e.value, comment = e.comment })
  end
  local t = tags.new(list)
  for _, line in ipairs(a._clean) do
    table.insert(t._clean, line)
  end
  for _, line in ipairs(b._clean) do
    table.insert(t._clean, line)
  end
  return t
end

---If `@name` is absent, prepend `{ tag = name, value = default_value }` (position 1).
---@param name string
---@param default_value table|nil
function tags:ensure_tag(name, default_value)
  if self:has(name) then
    return
  end
  table.insert(self._list, 1, { tag = name, value = default_value or {} })
end

---Lines for AST dump: tag names summary plus clean (non-tag) doc lines.
---@return string[]
function tags:dump_lines()
  local out = {}
  local names = {}
  for _, e in ipairs(self._list) do
    table.insert(names, '@' .. e.tag)
  end
  if #names > 0 then
    table.insert(out, table.concat(names, ' '))
  end
  for _, line in ipairs(self._clean) do
    table.insert(out, line)
  end
  return out
end

-- ── Parsing ───────────────────────────────────────────────────────────────────

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
        -- optionally consume (...) with balanced closing paren
        if j <= n and text:sub(j, j) == '(' then
          local close = find_matching_close_paren(text, j)
          if close then
            value = tags.parse_value_list(text:sub(j + 1, close - 1))
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
  local t = tags.new()
  t:_parse(text)
  return t
end


return tags
