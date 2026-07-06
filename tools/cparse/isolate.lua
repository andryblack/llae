local class = require 'llae.class'
local fs = require 'llae.fs'
local path = require 'llae.path'

---@class isolate
---@field new fun(): isolate
---@field run fun(dir: string, patterns: string[], replaces: table<string, string>)
---@field match_glob fun(file_path: string, pattern: string): boolean
---@field _index table<string, string>
---@field _prefixes string[]
local isolate = class(nil, 'isolate')

local function make_new_path(prefix, rel)
  prefix = prefix:gsub('/+$', '')
  if prefix == '' then
    return rel
  end
  return prefix .. '/' .. rel
end

local function glob_to_pattern(glob)
  local parts = {}
  for i = 1, #glob do
    local c = glob:sub(i, i)
    if c == '*' then
      table.insert(parts, '[^/]*')
    elseif c == '?' then
      table.insert(parts, '[^/]')
    else
      table.insert(parts, (c:gsub('([%%%^%$%(%)%.%[%]%+%-%?])', '%%%1')))
    end
  end
  return '^' .. table.concat(parts) .. '$'
end

function isolate.match_glob(file_path, pattern)
  return file_path:match(glob_to_pattern(pattern)) ~= nil
end

local function match_any_pattern(file_path, patterns)
  for _, p in ipairs(patterns) do
    if isolate.match_glob(file_path, p) then
      return true
    end
  end
  return false
end

local function collect_prefixes(replaces)
  local prefixes = {}
  for _, prefix in pairs(replaces) do
    local p = prefix:gsub('/+$', '')
    if p ~= '' then
      prefixes[#prefixes + 1] = p .. '/'
      prefixes[#prefixes + 1] = p
    end
  end
  return prefixes
end

function isolate:_init()
  self._index = {}
  self._prefixes = {}
end

function isolate:get_index()
  return self._index
end

---@param replaces table<string, string> Header directory -> include prefix
function isolate:build_index(replaces)
  self._index = {}
  for key, prefix in pairs(replaces) do
    local files, err = fs.scanfiles_r(key)
    if not files then
      error('failed scan dir ' .. key .. ' ' .. (err or ''))
    end
    for _, rel in ipairs(files) do
      self._index[rel] = make_new_path(prefix, rel)
    end
  end
  self._prefixes = collect_prefixes(replaces)
  return self._index
end

---@param dir string
---@param patterns string[]
---@return string[]
function isolate:scan(dir, patterns)
  local files, err = fs.scanfiles_r(dir)
  if not files then
    error('failed scan dir ' .. dir .. ' ' .. (err or ''))
  end
  local res = {}
  for _, rel in ipairs(files) do
    if match_any_pattern(rel, patterns) then
      table.insert(res, rel)
    end
  end
  return res
end

function isolate:rewrite_line(line)
  local trimmed = line:match('^%s*(.-)%s*$') or ''
  if trimmed:match('^//') or trimmed:match('^/%*') then
    return line
  end

  local directive, inc_path = trimmed:match('^(#%s*include)%s+"([^"]+)"')
  if not directive then
    directive, inc_path = trimmed:match('^(#%s*include)%s+<([^>]+)>')
  end
  if not inc_path then
    return line
  end

  for _, pfx in ipairs(self._prefixes) do
    if inc_path:sub(1, #pfx) == pfx then
      return line
    end
  end

  local new_path = self._index[inc_path]
  if not new_path or inc_path == new_path then
    return line
  end

  local indent = line:match('^(%s*)') or ''
  return indent .. directive .. ' "' .. new_path .. '"'
end

local function split_lines(content)
  local had_trailing_newline = #content > 0 and content:sub(-1) == '\n'
  if had_trailing_newline then
    content = content:sub(1, -2)
  end
  local lines = {}
  if content ~= '' then
    local start = 1
    while start <= #content do
      local pos = content:find('\n', start, true)
      if pos then
        table.insert(lines, content:sub(start, pos - 1))
        start = pos + 1
      else
        table.insert(lines, content:sub(start))
        break
      end
    end
  end
  return lines, had_trailing_newline
end

local function join_lines(lines, had_trailing_newline)
  local result = table.concat(lines, '\n')
  if had_trailing_newline then
    result = result .. '\n'
  end
  return result
end

---@return string
---@return boolean changed
function isolate:rewrite_text(content)
  local lines, had_trailing_newline = split_lines(content)
  local changed = false
  for i, line in ipairs(lines) do
    local new_line = self:rewrite_line(line)
    if new_line ~= line then
      changed = true
    end
    lines[i] = new_line
  end
  if not changed then
    return content, false
  end
  return join_lines(lines, had_trailing_newline), true
end

---@return boolean changed
function isolate:process_file(filepath)
  local content = fs.load_file(filepath)
  if not content then
    return false
  end
  local new_content, changed = self:rewrite_text(tostring(content))
  if changed then
    fs.write_file(filepath, new_content)
  end
  return changed
end

---Rewrite `#include` paths in source files under `dir`.
---Builds an index from header files in `replaces` dirs, then rewrites matching includes in-place.
---@param dir string Source directory to scan
---@param patterns string[] Glob patterns for files to process, e.g. `{ '*.c' }`
---@param replaces table<string, string> Header directory -> include prefix
function isolate.run(dir, patterns, replaces)
  local self = isolate.new()
  self:build_index(replaces)
  for _, rel in ipairs(self:scan(dir, patterns)) do
    self:process_file(path.join(dir, rel))
  end
end

return isolate
