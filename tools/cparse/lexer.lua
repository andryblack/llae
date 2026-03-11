local class = require 'llae.class'
local token = require 'cparse.token'
local log = require 'llae.log'

---@class lexer
---@field new fun(source: string): lexer
---@field _source string
---@field _pos    integer
---@field _len    integer
---@field _line   integer
---@field _peeked token|nil
local lexer = class(nil, 'lexer')

local KEYWORDS = {
  ["class"] = true, ["struct"] = true, ["union"] = true, ["enum"] = true,
  ["namespace"] = true, ["typedef"] = true, ["using"] = true, ["template"] = true,
  ["extern"] = true, ["inline"] = true, ["static"] = true, ["virtual"] = true,
  ["const"] = true, ["constexpr"] = true, ["consteval"] = true, ["constinit"] = true,
  ["override"] = true, ["final"] = true,
  ["public"] = true, ["protected"] = true, ["private"] = true,
  ["operator"] = true, ["friend"] = true, ["explicit"] = true, ["mutable"] = true,
  ["volatile"] = true, ["noexcept"] = true, ["typename"] = true,
  ["void"] = true, ["bool"] = true, ["auto"] = true,
  ["char"] = true, ["short"] = true, ["int"] = true, ["long"] = true,
  ["float"] = true, ["double"] = true,
  ["unsigned"] = true, ["signed"] = true,
  ["nullptr"] = true, ["true"] = true, ["false"] = true,
  ["delete"] = true, ["default"] = true, ["new"] = true, ["this"] = true,
  ["sizeof"] = true, ["alignof"] = true, ["decltype"] = true,
  ["static_assert"] = true, ["static_cast"] = true, ["dynamic_cast"] = true,
  ["reinterpret_cast"] = true, ["const_cast"] = true,
}

-- byte constants
local B_LF     = 10  -- '\n'
local B_CR     = 13  -- '\r'
local B_SPACE  = 32  -- ' '
local B_DQUOTE = 34  -- '"'
local B_HASH   = 35  -- '#'
local B_SQUOTE = 39  -- "'"
local B_STAR   = 42  -- '*'
local B_PLUS   = 43  -- '+'
local B_MINUS  = 45  -- '-'
local B_DOT    = 46  -- '.'
local B_SLASH  = 47  -- '/'
local B_0      = 48
local B_9      = 57
local B_LT     = 60  -- '<'
local B_EQ     = 61  -- '='
local B_GT     = 62  -- '>'
local B_A_UP   = 65  -- 'A'
local B_Z_UP   = 90  -- 'Z'
local B_LBRAK  = 91  -- '['
local B_BSLASH = 92  -- '\'
local B_RBRAK  = 93  -- ']'
local B_UNDER  = 95  -- '_'
local B_A_LO   = 97  -- 'a'
local B_F_LO   = 102 -- 'f'
local B_TAB    = 9   -- '\t'
local B_X_LO   = 120 -- 'x'
local B_X_UP   = 88  -- 'X'
local B_Z_LO   = 122 -- 'z'
local B_E_LO   = 101 -- 'e'
local B_E_UP   = 69  -- 'E'
local B_F_UP   = 70  -- 'F'

local function is_digit(b)
  return b ~= nil and b >= B_0 and b <= B_9
end

local function is_hex_digit(b)
  return (b >= B_0 and b <= B_9) or (b >= B_A_UP and b <= B_F_UP) or (b >= B_A_LO and b <= B_F_LO)
end

local function is_alpha(b)
  return (b >= B_A_UP and b <= B_Z_UP) or (b >= B_A_LO and b <= B_Z_LO) or b == B_UNDER
end

local function is_alnum(b)
  return is_alpha(b) or is_digit(b)
end

---@param source string
function lexer:_init(source)
  self._source = source
  self._pos    = 1
  self._len    = #source
  self._line   = 1
  self._peeked = nil
end

---Read and return the next token from source. Updates self._pos and self._line.
---@return token
function lexer:_read_token()
  local src = self._source
  local pos = self._pos
  local len = self._len

  -- skip whitespace (spaces, tabs, CR, LF)
  while pos <= len do
    local b = src:byte(pos)
    if b == B_SPACE or b == B_TAB or b == B_CR then
      pos = pos + 1
    elseif b == B_LF then
      pos = pos + 1
      self._line = self._line + 1
      -- local next = src:find('\n',pos,true)
      -- if next then
      --   next = src:sub(pos,next-1)
      --   log.info('lexer: next line',self._line,next)
      -- end
    else
      break
    end
  end

  if pos > len then
    self._pos = pos
    return token.new("eof", "", self._line)
  end

  local line = self._line
  local b0 = src:byte(pos)

  -- comments  '/' ...
  if b0 == B_SLASH then
    local b1 = src:byte(pos + 1)

    -- line comment: // or ///
    if b1 == B_SLASH then
      local b2 = src:byte(pos + 2)
      if b2 == B_SLASH then
        -- doc line comment  ///
        local start = pos
        while pos <= len and src:byte(pos) ~= B_LF do
          pos = pos + 1
        end
        self._pos = pos
        return token.new("doc_comment", src:sub(start, pos - 1), line)
      else
        -- regular line comment, skip
        while pos <= len and src:byte(pos) ~= B_LF do
          pos = pos + 1
        end
        self._pos = pos
        return self:_read_token()
      end
    end

    -- block comment: /* or /**
    if b1 == B_STAR then
      local b2 = src:byte(pos + 2)
      -- /** starts a doc comment, but /**/ is just an empty regular comment
      local is_doc = (b2 == B_STAR) and (src:byte(pos + 3) ~= B_SLASH)
      local start = pos
      pos = pos + 2  -- skip '/*'
      while pos <= len do
        local bc = src:byte(pos)
        if bc == B_LF then
          self._line = self._line + 1
        end
        if bc == B_STAR and src:byte(pos + 1) == B_SLASH then
          pos = pos + 2  -- skip '*/'
          break
        end
        pos = pos + 1
      end
      self._pos = pos
      if is_doc then
        return token.new("doc_comment", src:sub(start, pos - 1), line)
      else
        return self:_read_token()
      end
    end
  end

  -- string literal  "..."
  -- also handles L"", u8"", u"", U"" prefixes (the prefix letter was already consumed as ident,
  -- but here we handle a raw '"' )
  if b0 == B_DQUOTE then
    local chars = {'"'}
    pos = pos + 1
    while pos <= len do
      local c = src:byte(pos)
      if c == B_BSLASH then
        table.insert(chars, src:sub(pos, pos + 1))
        pos = pos + 2
      elseif c == B_DQUOTE then
        table.insert(chars, '"')
        pos = pos + 1
        break
      elseif c == B_LF then
        self._line = self._line + 1
        table.insert(chars, '\n')
        pos = pos + 1
      else
        table.insert(chars, src:sub(pos, pos))
        pos = pos + 1
      end
    end
    self._pos = pos
    return token.new("string", table.concat(chars), line)
  end

  -- char literal  '...'
  if b0 == B_SQUOTE then
    local chars = {"'"}
    pos = pos + 1
    while pos <= len do
      local c = src:byte(pos)
      if c == B_BSLASH then
        table.insert(chars, src:sub(pos, pos + 1))
        pos = pos + 2
      elseif c == B_SQUOTE then
        table.insert(chars, "'")
        pos = pos + 1
        break
      else
        table.insert(chars, src:sub(pos, pos))
        pos = pos + 1
      end
    end
    self._pos = pos
    return token.new("char", table.concat(chars), line)
  end

  -- number literal  [0-9] or '.' followed by digit
  if is_digit(b0) or (b0 == B_DOT and is_digit(src:byte(pos + 1))) then
    local start = pos
    if b0 == B_0 then
      local b1 = src:byte(pos + 1)
      if b1 == B_X_LO or b1 == B_X_UP then
        -- hex  0x...
        pos = pos + 2
        while pos <= len and is_hex_digit(src:byte(pos)) do
          pos = pos + 1
        end
      else
        -- octal or decimal
        while pos <= len and (is_digit(src:byte(pos)) or src:byte(pos) == B_DOT) do
          pos = pos + 1
        end
      end
    else
      while pos <= len and (is_digit(src:byte(pos)) or src:byte(pos) == B_DOT) do
        pos = pos + 1
      end
    end
    -- optional exponent e/E ±digits
    local be = src:byte(pos)
    if be == B_E_LO or be == B_E_UP then
      pos = pos + 1
      local bs = src:byte(pos)
      if bs == B_PLUS or bs == B_MINUS then pos = pos + 1 end
      while pos <= len and is_digit(src:byte(pos)) do
        pos = pos + 1
      end
    end
    -- optional suffix: u, l, ul, ull, f, etc.
    while pos <= len and is_alpha(src:byte(pos)) do
      pos = pos + 1
    end
    self._pos = pos
    return token.new("number", src:sub(start, pos - 1), line)
  end

  -- identifier or keyword  [A-Za-z_][A-Za-z0-9_]*
  if is_alpha(b0) then
    local start = pos
    pos = pos + 1
    while pos <= len and is_alnum(src:byte(pos)) do
      pos = pos + 1
    end
    local word = src:sub(start, pos - 1)
    self._pos = pos
    if KEYWORDS[word] then
      return token.new("keyword", word, line)
    end
    return token.new("ident", word, line)
  end

  -- operators and punctuation — try longest match first

  -- 3-char
  local c3 = src:sub(pos, pos + 2)
  if c3 == "..." or c3 == "<<=" or c3 == ">>=" then
    self._pos = pos + 3
    return token.new("punct", c3, line)
  end

  -- 2-char
  local c2 = src:sub(pos, pos + 1)
  if c2 == "::" or c2 == "->" or c2 == ".*" or
     c2 == "[[" or c2 == "]]" or
     c2 == "==" or c2 == "!=" or c2 == "<=" or c2 == ">=" or
     c2 == "&&" or c2 == "||" or
     c2 == "++" or c2 == "--" or
     c2 == "+=" or c2 == "-=" or c2 == "*=" or c2 == "/=" or
     c2 == "%=" or c2 == "&=" or c2 == "|=" or c2 == "^=" or
     c2 == "<<" or c2 == ">>" then
    self._pos = pos + 2
    return token.new("punct", c2, line)
  end

  -- single char
  self._pos = pos + 1
  return token.new("punct", src:sub(pos, pos), line)
end

---Consume and return the next token.
---@return token
function lexer:next()
  if self._peeked then
    local tok = self._peeked
    self._peeked = nil
    return tok
  end
  return self:_read_token()
end

---Return the next token without consuming it.
---@return token
function lexer:peek()
  if not self._peeked then
    self._peeked = self:_read_token()
  end
  return self._peeked
end

---Consume the next token, asserting its kind (and optionally its value).
---Raises an error if the token does not match.
---@param kind string expected token kind
---@param value string|nil optional expected value
---@return token
function lexer:expect(kind, value)
  local tok = self:next()
  if tok.kind ~= kind then
    error(string.format(
      "lexer: expected [%s] but got %s", kind, tostring(tok)), 2)
  end
  if value ~= nil and tok.value ~= value then
    error(string.format(
      "lexer: expected [%s '%s'] but got %s", kind, value, tostring(tok)), 2)
  end
  return tok
end

---Consume the next token only if it matches kind (and optional value).
---Returns the token on match, nil otherwise.
---@param kind string
---@param value string|nil
---@return token|nil
function lexer:accept(kind, value)
  if self:peek():is(kind, value) then
    return self:next()
  end
  return nil
end

function lexer:dump()
  local log = require 'llae.log'
  log.info('lexer:dump',self._line,self._pos,'/',self._len)
end

return lexer
