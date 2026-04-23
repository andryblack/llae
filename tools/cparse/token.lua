local class = require 'llae.class'

---@class token
---@field kind string  "keyword"|"ident"|"number"|"string"|"char"|"punct"|"doc_comment"|"field_binding"|"func_binding"|"eof"
---@field value string
---@field line integer
---@field new fun(kind: string, value: string, line: integer): token
local token = class(nil, 'token')

---@param kind string
---@param value string
---@param line integer
function token:_init(kind, value, line)
  self.kind  = kind
  self.value = value
  self.line  = line
end

function token:is_same(other)
  return self.kind == other.kind and self.value == other.value and self.line == other.line
end

---Check kind, and optionally value.
---@param kind string
---@param value string|nil
---@return boolean
function token:is(kind, value)
  if self.kind ~= kind then return false end
  if value ~= nil then return self.value == value end
  return true
end

---True if this is a keyword (optionally check which one).
---@param word string|nil
---@return boolean
function token:is_keyword(word)
  return self:is("keyword", word)
end

---True if this is an identifier.
---@return boolean
function token:is_ident()
  return self.kind == "ident"
end

---True if this is a punctuation token (optionally check the symbol).
---@param sym string|nil
---@return boolean
function token:is_punct(sym)
  return self:is("punct", sym)
end

---True if this is a doc comment token.
---@return boolean
function token:is_doc()
  return self.kind == "doc_comment"
end

---True if this is an @field(...) binding directive (only lexed in extern-parse mode).
---@return boolean
function token:is_field_binding()
  return self.kind == "field_binding"
end

---True if this is an @func(...) stub directive (only lexed in extern-parse mode).
---@return boolean
function token:is_func_binding()
  return self.kind == "func_binding"
end

---True if this is the end-of-file sentinel.
---@return boolean
function token:is_eof()
  return self.kind == "eof"
end

function token:__tostring()
  return string.format("[%s '%s' @%d]", self.kind, self.value, self.line)
end

---Join a list of tokens into a readable type string, applying C++ spacing rules.
---@param toks table   list of token objects
---@param from integer|nil  start index (default 1)
---@param to   integer|nil  end index   (default #toks)
---@return string
function token.join(toks, from, to)
  from = from or 1
  to   = to   or #toks
  if from > to then return "" end
  local parts = {}
  for i = from, to do
    local t    = toks[i]
    local prev = toks[i - 1]
    local no_space_before = prev and (
      t.value == "::" or t.value == "<" or t.value == ">" or t.value == ">>" or
      t.value == "*"  or t.value == "&" or t.value == "&&" or
      t.value == ")"  or t.value == "]" or t.value == ","
    )
    local no_space_after_prev = prev and (
      prev.value == "::" or prev.value == "<" or
      prev.value == "("  or prev.value == "[" or prev.value == "~"
    )
    if prev and not no_space_before and not no_space_after_prev then
      table.insert(parts, " ")
    end
    table.insert(parts, t.value)
  end
  return table.concat(parts)
end

return token
