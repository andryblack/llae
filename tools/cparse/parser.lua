local class = require 'llae.class'
local lexer_mod = require 'cparse.lexer'
local token = require 'cparse.token'
local ast = require 'cparse.ast'
local tags = require 'cparse.tags'
local log = require 'llae.log'

---@class parser
---@field new fun(source: string): parser
local parser = class(nil, 'parser')


function parser._yield() 
end

-- Keywords that are function/storage specifiers (extracted into qualifiers, not type string)
local FUNC_SPECS = {
  static=true, inline=true, virtual=true, explicit=true,
  mutable=true, friend=true, constexpr=true, consteval=true,
  constinit=true, ["extern"]=true,
}

---@param source string
function parser:_init(source, opts)
  self._lex = lexer_mod.new(source)
  self._pending_tag_text = nil  -- accumulated `///` / `/**` text before next decl
end

-- ── Utilities ────────────────────────────────────────────────────────────────

-- Consume tokens to skip a balanced brace block.
-- If already_open is false (default) the opening '{' is consumed here.
function parser:_skip_braces(already_open)
  if not already_open then
    if not self._lex:accept("punct", "{") then return end
  end
  local depth = 1
  while depth > 0 do
    local tok = self._lex:next()
    if tok:is_eof() then return end
    if     tok:is_punct("{") then depth = depth + 1
    elseif tok:is_punct("}") then depth = depth - 1
    end
    parser._yield()
  end
end

-- Consume tokens up to and including the next ';' at depth 0.
function parser:_skip_to_semi()
  local depth = 0
  while true do
    local tok = self._lex:next()
    if tok:is_eof() then return end
    if     tok:is_punct("(") or tok:is_punct("{") or tok:is_punct("[") then depth = depth + 1
    elseif tok:is_punct(")") or tok:is_punct("}") or tok:is_punct("]") then depth = depth - 1
    elseif tok:is_punct(";") and depth == 0 then return
    end
    parser._yield()
  end
end

-- Parse and clear pending doc comment text into a `tags` instance.
function parser:_take_tags()
  local d = self._pending_tag_text
  self._pending_tag_text = nil
  return tags.parse(d or "")
end

local toks_to_str = token.join

-- ── Attributes ───────────────────────────────────────────────────────────────

-- Consume and return any leading [[attr]] sequences as a string.
function parser:_parse_attributes()
  local parts = {}
  while self._lex:peek():is_punct("[[") do
    local attr = {"[["}
    self._lex:next()
    local depth = 1
    while depth > 0 do
      local tok = self._lex:next()
      if tok:is_eof() then break end
      if     tok:is_punct("[[") then depth = depth + 1; table.insert(attr, "[[")
      elseif tok:is_punct("]]") then depth = depth - 1; table.insert(attr, "]]")
      else   table.insert(attr, tok.value)
      end
      parser._yield()
    end
    table.insert(parts, table.concat(attr))
    parser._yield()
  end
  return #parts > 0 and table.concat(parts, " ") or nil
end

-- ── Template prefix ──────────────────────────────────────────────────────────

-- Consume template<...> and return it as a raw string e.g. "template<typename T>".
function parser:_parse_template_prefix()
  self._lex:expect("keyword", "template")
  if not self._lex:accept("punct", "<") then return "template" end
  local parts = {"template<"}
  local depth = 1
  while depth > 0 do
    local tok = self._lex:next()
    if tok:is_eof() then break end
    if     tok:is_punct("<")  then depth = depth + 1; table.insert(parts, "<")
    elseif tok:is_punct(">>") then
      depth = depth - 2; table.insert(parts, ">>")
      if depth <= 0 then break end
    elseif tok:is_punct(">") then
      depth = depth - 1
      if depth == 0 then table.insert(parts, ">"); break
      else               table.insert(parts, ">")
      end
    else table.insert(parts, tok.value)
    end
    parser._yield()
  end
  return table.concat(parts)
end

-- ── Base clause ──────────────────────────────────────────────────────────────

-- Parse ': access Base, access Base2, ...'  (the ':' has already been consumed).
local function parse_base_clause(lex)
  local bases = {}
  while true do
    -- optional access specifier
    local access = "private"
    local t = lex:peek()
    if t:is_keyword("public") or t:is_keyword("protected") or t:is_keyword("private") then
      access = t.value; lex:next()
    end
    -- optional 'virtual'
    if lex:peek():is_keyword("virtual") then lex:next() end

    -- base name  (may be scoped, may have template args)
    local name_parts = {}
    t = lex:peek()
    if t:is_ident() or (t:is_keyword() and not t:is_keyword("public")
        and not t:is_keyword("protected") and not t:is_keyword("private")
        and not t:is_keyword("virtual")) then
      table.insert(name_parts, lex:next().value)
      while lex:peek():is_punct("::") do
        table.insert(name_parts, "::")
        lex:next()
        if lex:peek():is_ident() then table.insert(name_parts, lex:next().value) end
        parser._yield()
      end
      -- skip template args < ... >
      if lex:peek():is_punct("<") then
        lex:next()
        local targs = {"<"}; local depth = 1
        while depth > 0 do
          local tt = lex:next()
          if tt:is_eof() then break end
          if     tt:is_punct("<")  then depth = depth + 1; table.insert(targs, "<")
          elseif tt:is_punct(">>") then depth = depth - 2; table.insert(targs, ">>"); if depth <= 0 then break end
          elseif tt:is_punct(">")  then depth = depth - 1; table.insert(targs, ">")
          else   table.insert(targs, tt.value)
          end
          parser._yield()
        end
        table.insert(name_parts, table.concat(targs))
      end
    end
    if #name_parts > 0 then
      table.insert(bases, {name = table.concat(name_parts), access = access})
    else
      break
    end
    if not lex:peek():is_punct(",") then break end
    lex:next()
    parser._yield()
  end
  return bases
end

-- ── Namespace ────────────────────────────────────────────────────────────────

function parser:_parse_namespace()
  local node_tags = self:_take_tags()
  self._lex:expect("keyword", "namespace")
  local name = ""
  if self._lex:peek():is_ident() then name = self._lex:next().value end
  -- nested namespace: namespace A::B::C (C++17)
  while self._lex:accept("punct", "::") do
    if self._lex:peek():is_ident() then
      name = name .. "::" .. self._lex:next().value
    end
    parser._yield()
  end
  local children = {}
  if self._lex:accept("punct", "{") then
    children = self:_parse_decl_list()
    self._lex:accept("punct", "}")
  end
  self._lex:accept("punct", ";")
  local n = ast.namespace.new(name, children)
  n.tags = node_tags
  return n
end

-- ── Class / struct / union ───────────────────────────────────────────────────

function parser:_parse_class_struct()
  local node_tags = self:_take_tags()
  local struct_kind = self._lex:next().value  -- "class", "struct", or "union"

  local name = ""
  if self._lex:peek():is_ident() then name = self._lex:next().value end

  local bases = {}
  if self._lex:accept("punct", ":") then bases = parse_base_clause(self._lex) end

  -- forward declaration
  if self._lex:accept("punct", ";") then
    local n = ast.class_forward.new(name, struct_kind)
    n.tags = node_tags
    return n
  end

  local children = {}
  if self._lex:accept("punct", "{") then
    children = self:_parse_decl_list(name)
    self._lex:accept("punct", "}")
  end

  -- optional variable name after closing brace (e.g.  struct { int x; } obj;)
  local var_name = nil
  if self._lex:peek():is_ident() then var_name = self._lex:next().value end

  self._lex:accept("punct", ";")
  local n = ast["class"].new(name, struct_kind, bases, children, var_name)
  n.tags = node_tags
  return n
end

-- ── Enum ─────────────────────────────────────────────────────────────────────

function parser:_parse_enum()
  local node_tags = self:_take_tags()
  self._lex:expect("keyword", "enum")

  local is_scoped = false
  if self._lex:peek():is_keyword("class") or self._lex:peek():is_keyword("struct") then
    is_scoped = true; self._lex:next()
  end

  local name = ""
  if self._lex:peek():is_ident() then name = self._lex:next().value end

  -- optional  : base_type
  local base_type = nil
  if self._lex:accept("punct", ":") then
    local tp = {}
    while true do
      local t = self._lex:peek()
      if t:is_eof() or t:is_punct("{") or t:is_punct(";") then break end
      table.insert(tp, self._lex:next())
      parser._yield()
    end
    base_type = toks_to_str(tp)
  end

  -- forward declaration
  if self._lex:accept("punct", ";") then
    local n = ast.enum.new(name, is_scoped, base_type, nil, true)
    n.tags = node_tags
    return n
  end

  -- body
  local values = {}
  if self._lex:accept("punct", "{") then
    while true do
      while self._lex:peek():is_doc() do self._lex:next() end
      local t = self._lex:peek()
      if t:is_eof() or t:is_punct("}") then break end

      local vname = nil
      if t:is_ident() or t:is_keyword() then
        vname = self._lex:next().value
      else
        self._lex:next(); break
      end

      local vval = nil
      if self._lex:accept("punct", "=") then
        local vp = {}; local depth = 0
        while true do
          local vt = self._lex:peek()
          if vt:is_eof() then break end
          if depth == 0 and (vt:is_punct(",") or vt:is_punct("}")) then break end
          if vt:is_punct("(") or vt:is_punct("{") then depth = depth + 1 end
          if vt:is_punct(")") or vt:is_punct("}") then
            if depth == 0 then break else depth = depth - 1 end
          end
          table.insert(vp, self._lex:next())
        end
        vval = toks_to_str(vp)
      end
      table.insert(values, {name = vname, value = vval})
      if not self._lex:accept("punct", ",") then break end
      parser._yield()
    end
    self._lex:accept("punct", "}")
  end
  self._lex:accept("punct", ";")
  local n = ast.enum.new(name, is_scoped, base_type, values)
  n.tags = node_tags
  return n
end

-- ── Typedef ──────────────────────────────────────────────────────────────────

function parser:_parse_typedef()
  local node_tags = self:_take_tags()
  self._lex:expect("keyword", "typedef")

  -- typedef struct/class/union/enum { ... } Name;
  local peek = self._lex:peek()
  if peek:is_keyword("struct") or peek:is_keyword("class") or peek:is_keyword("union") then
    local inner = self:_parse_class_struct()
    local n = ast.typedef.new(inner.var_name or "", nil, inner)
    n.tags = node_tags
    return n
  end
  if peek:is_keyword("enum") then
    local inner = self:_parse_enum()
    local n = ast.typedef.new(inner.var_name or "", nil, inner)
    n.tags = node_tags
    return n
  end

  -- general  typedef <type> <name> ;
  local toks = {}
  while true do
    local t = self._lex:peek()
    if t:is_eof() or t:is_punct(";") then break end
    table.insert(toks, self._lex:next())
    parser._yield()
  end
  self._lex:accept("punct", ";")

  -- last bare ident is the typedef name
  local name_idx = nil
  for i = #toks, 1, -1 do
    if toks[i]:is_ident() then name_idx = i; break end
  end
  if name_idx then
    local type_parts = {}
    for i = 1, name_idx - 1 do table.insert(type_parts, toks[i]) end
    local n = ast.typedef.new(toks[name_idx].value, toks_to_str(type_parts))
    n.tags = node_tags
    return n
  end
  local n = ast.typedef.new("", "")
  n.tags = node_tags
  return n
end

-- ── Using ─────────────────────────────────────────────────────────────────────

function parser:_parse_using()
  local node_tags = self:_take_tags()
  self._lex:expect("keyword", "using")

  -- using namespace Foo;
  if self._lex:peek():is_keyword("namespace") then
    self._lex:next()
    local np = {}
    while true do
      if self._lex:peek():is_ident() then
        table.insert(np, self._lex:next().value)
      elseif self._lex:peek():is_punct("::") then
        table.insert(np, "::"); self._lex:next()
      else break end
      parser._yield()
    end
    self._lex:accept("punct", ";")
    local n = ast.using_namespace.new(table.concat(np))
    n.tags = node_tags
    return n
  end

  local name = ""
  if self._lex:peek():is_ident() then name = self._lex:next().value end

  -- type alias:  using Name = Type;
  if self._lex:accept("punct", "=") then
    local tp = {}; local depth = 0
    while true do
      local t = self._lex:peek()
      if t:is_eof() then break end
      if depth == 0 and t:is_punct(";") then break end
      if t:is_punct("(") or t:is_punct("<") or t:is_punct("[") then depth = depth + 1 end
      if t:is_punct(")") or t:is_punct(">") or t:is_punct("]") then
        if depth == 0 then break else depth = depth - 1 end
      end
      table.insert(tp, self._lex:next())
      parser._yield()
    end
    self._lex:accept("punct", ";")
    local n = ast.using.new(name, toks_to_str(tp))
    n.tags = node_tags
    return n
  end

  -- using Base::member; or other forms
  self:_skip_to_semi()
  local n = ast.using.new(name, nil)
  n.tags = node_tags
  return n
end

-- ── Parameter parsing ────────────────────────────────────────────────────────

-- Parse one parameter from the token stream.
-- Returns {type=string, name=string|nil, default=string|nil}
local function parse_one_param(lex)
  local toks  = {}
  local depth_p = 0  -- parens
  local depth_a = 0  -- angle brackets
  local depth_b = 0  -- square brackets

  while true do
    local t = lex:peek()
    if t:is_eof() then break end
    -- stop at top-level , or )
    if depth_p == 0 and depth_a == 0 and depth_b == 0 then
      if t:is_punct(",") or t:is_punct(")") then break end
    end
    -- track depth
    if     t:is_punct("(")  then depth_p = depth_p + 1
    elseif t:is_punct(")")  then if depth_p > 0 then depth_p = depth_p - 1 end
    elseif t:is_punct("<")  then depth_a = depth_a + 1
    elseif t:is_punct(">>") then depth_a = math.max(0, depth_a - 2)
    elseif t:is_punct(">")  then if depth_a > 0 then depth_a = depth_a - 1 end
    elseif t:is_punct("[")  then depth_b = depth_b + 1
    elseif t:is_punct("]")  then if depth_b > 0 then depth_b = depth_b - 1 end
    end
    -- handle default value  name = expr
    if depth_p == 0 and depth_a == 0 and depth_b == 0 and t:is_punct("=") then
      lex:next()  -- consume '='
      local dp = {}; local dd = 0
      while true do
        local dt = lex:peek()
        if dt:is_eof() then break end
        if dd == 0 and (dt:is_punct(",") or dt:is_punct(")")) then break end
        if dt:is_punct("(") or dt:is_punct("<") or dt:is_punct("[") then dd = dd + 1 end
        if dt:is_punct(")") or dt:is_punct(">") or dt:is_punct("]") then
          if dd == 0 then break else dd = dd - 1 end
        end
        table.insert(dp, lex:next().value)
        parser._yield()
      end
      -- find bare name: last ident in toks at depth 0
      local ni = nil
      for i = #toks, 1, -1 do
        if toks[i]:is_ident() and toks[i]._depth_a == 0 and toks[i]._depth_p == 0 then
          ni = i; break
        end
      end
      local nm = nil
      if ni then
        nm = toks[ni].value
        local left = toks_to_str(toks, 1, ni - 1)
        local right = toks_to_str(toks, ni + 1, #toks)
        local joined = (left ~= "" and right ~= "") and (left .. " " .. right) or (left ~= "" and left or right)
        return {type = joined, name = nm, default = table.concat(dp, " ")}
      end
      return {type = toks_to_str(toks), name = nm, default = table.concat(dp, " ")}
    end
    -- tag current depth onto token for later name detection
    local tt = lex:next()
    tt._depth_a = depth_a; tt._depth_p = depth_p
    table.insert(toks, tt)
    parser._yield()
  end

  -- no default — find bare name (last ident at depth 0)
  local ni = nil
  for i = #toks, 1, -1 do
    if toks[i]:is_ident() and toks[i]._depth_a == 0 and toks[i]._depth_p == 0 then
      ni = i; break
    end
  end
  -- only treat it as name if it's a plausible declarator identifier.
  -- do not treat single-token or scoped-type tails (e.g. std::string) as a parameter name.
  local prev = ni and toks[ni - 1] or nil
  if ni and ni == #toks and ni > 1 and not (prev and prev:is_punct("::")) then
    return {type = toks_to_str(toks, 1, ni - 1), name = toks[ni].value}
  end
  return {type = toks_to_str(toks), name = nil}
end

-- Parse a full parameter list (the '(' has already been consumed).
function parser:_parse_params()
  local params = {}
  if self._lex:peek():is_punct(")") then
    return params  -- empty ()
  end
  -- (void) → treat as empty
  if self._lex:peek():is_keyword("void") then
    self._lex:next()
    if self._lex:peek():is_punct(")") then
      return params
    end
    -- actually "void*" or similar — accumulate the rest and prepend "void"
    local remaining_toks = {}
    local depth_p = 0; local depth_a = 0
    while true do
      local t = self._lex:peek()
      if t:is_eof() then break end
      if depth_p == 0 and depth_a == 0 and (t:is_punct(",") or t:is_punct(")")) then break end
      if     t:is_punct("(") then depth_p = depth_p + 1
      elseif t:is_punct(")") then if depth_p > 0 then depth_p = depth_p - 1 end
      elseif t:is_punct("<") then depth_a = depth_a + 1
      elseif t:is_punct(">") then if depth_a > 0 then depth_a = depth_a - 1 end
      end
      table.insert(remaining_toks, self._lex:next())
      parser._yield()
    end
    local ni = nil
    if #remaining_toks > 0 and remaining_toks[#remaining_toks]:is_ident() then
      ni = #remaining_toks
    end
    local nm = nil
    local type_str = nil
    if ni then
      nm = remaining_toks[ni].value
      local left = "void"
      local right = toks_to_str(remaining_toks, 1, ni - 1)
      type_str = right ~= "" and (left .. " " .. right) or left
    else
      local right = toks_to_str(remaining_toks)
      type_str = right ~= "" and ("void " .. right) or "void"
    end
    table.insert(params, {type = type_str, name = nm})
    if not self._lex:accept("punct", ",") then return params end
  end

  while true do
    if self._lex:peek():is_eof() then break end
    if self._lex:peek():is_punct(")") then break end
    if self._lex:peek():is_punct("...") then
      self._lex:next()
      table.insert(params, {type = "...", name = nil})
      break
    end
    local p = parse_one_param(self._lex)
    if p then table.insert(params, p) end
    if not self._lex:accept("punct", ",") then break end
    parser._yield()
  end
  return params
end

-- ── Core: type + name declaration ────────────────────────────────────────────

-- Parse everything from the current position that constitutes a field or function decl.
-- extra_specs: list of specifier strings already consumed (e.g. {"extern"})
function parser:_parse_type_and_name_decl(extra_specs,class_name)
  local node_tags = self:_take_tags()

  -- Collect leading specifier keywords
  local qualifiers = {}
  for _, s in ipairs(extra_specs or {}) do qualifiers[s] = true end
  while true do
    local t = self._lex:peek()
    if t:is_keyword() and FUNC_SPECS[t.value] then
      qualifiers[t.value] = true; self._lex:next()
    else 
      break 
    end
    parser._yield()
  end

  -- Scan tokens that make up "type name" (stop before '(', ';', '=', '{' at depth 0)
  local pre           = {}
  local depth_a       = 0
  local depth_p       = 0
  local depth_b       = 0
  local found_paren   = false
  local last_bare_idx = nil  -- index in `pre` of last bare identifier

  while true do
    local t = self._lex:peek()
    if t:is_eof() then break end

    local at_top = (depth_a == 0 and depth_p == 0 and depth_b == 0)
    if at_top then
      if     t:is_punct("(") then found_paren = true; break
      elseif t:is_punct(";") or t:is_punct("=") or t:is_punct("{") or t:is_punct(",") then break
      end
    end

    local tok = self._lex:next()
    table.insert(pre, tok)

    if     tok:is_punct("<")  then depth_a = depth_a + 1
    elseif tok:is_punct(">>") then depth_a = math.max(0, depth_a - 2)
    elseif tok:is_punct(">")  then if depth_a > 0 then depth_a = depth_a - 1 end
    elseif tok:is_punct("(")  then depth_p = depth_p + 1
    elseif tok:is_punct(")")  then if depth_p > 0 then depth_p = depth_p - 1 end
    elseif tok:is_punct("[")  then depth_b = depth_b + 1
    elseif tok:is_punct("]")  then if depth_b > 0 then depth_b = depth_b - 1 end
    end

    -- Track last bare identifier (at top-level depth)
    if depth_a == 0 and depth_p == 0 and depth_b == 0 then
      if tok:is_ident() or tok:is_keyword("operator") then
        last_bare_idx = #pre
      end
    end
    parser._yield()
  end

  -- Determine name and type tokens
  local name      = nil
  local type_toks = pre

  if last_bare_idx then
    local name_tok = pre[last_bare_idx]
    if name_tok.value == "operator" then
      -- name = "operator <symbol>" — collect remaining pre tokens after 'operator'
      local op_parts = {"operator"}
      for i = last_bare_idx + 1, #pre do
        table.insert(op_parts, pre[i].value)
      end
      name      = table.concat(op_parts)
      type_toks = {}
      for i = 1, last_bare_idx - 1 do table.insert(type_toks, pre[i]) end
    else
      name      = name_tok.value
      type_toks = {}
      for i = 1, last_bare_idx - 1 do table.insert(type_toks, pre[i]) end
    end
  end

  -- Handle destructor: ~ immediately before the name
  if last_bare_idx and last_bare_idx > 1 and pre[last_bare_idx - 1].value == "~" then
    name      = "~" .. (name or "")
    type_toks = {}
    for i = 1, last_bare_idx - 2 do table.insert(type_toks, pre[i]) end
  end

  -- Build type string, moving any stray specifiers into qualifiers
  local type_tokens = {}
  for _, t in ipairs(type_toks) do
    if t:is_keyword() and FUNC_SPECS[t.value] then
      qualifiers[t.value] = true
    else
      table.insert(type_tokens, t)
    end
  end
  local type_str = toks_to_str(type_tokens)

  -- ── FUNCTION ──────────────────────────────────────────────────────────────
  if found_paren then
    self._lex:expect("punct", "(")
    local params = self:_parse_params()
    self._lex:expect("punct", ")")

    -- post-qualifiers: const volatile noexcept override final
    while true do
      local t = self._lex:peek()
      if t:is_keyword("const") or t:is_keyword("volatile") or t:is_keyword("override") or
         t:is_keyword("final") or t:is_keyword("noexcept") then
        qualifiers[t.value] = true; self._lex:next()
        if t:is_keyword("noexcept") and self._lex:peek():is_punct("(") then
          -- noexcept(expr) — skip the parens
          self._lex:next()
          local d = 1
          while d > 0 do
            local nt = self._lex:next()
            if nt:is_eof() then break end
            if     nt:is_punct("(") then d = d + 1
            elseif nt:is_punct(")") then d = d - 1
            end
          end
        end
      elseif t:is_punct("->") then
        -- trailing return type  -> Type
        self._lex:next()
        local tr = {}; local d = 0
        while true do
          local tt = self._lex:peek()
          if tt:is_eof() then break end
          if d == 0 and (tt:is_punct(";") or tt:is_punct("{")) then break end
          if tt:is_punct("(") or tt:is_punct("<") then d = d + 1 end
          if tt:is_punct(")") or tt:is_punct(">") then
            if d == 0 then break else d = d - 1 end
          end
          table.insert(tr, self._lex:next())
        end
        if type_str == "" or type_str == "auto" then
          type_str = toks_to_str(tr)
        end
      elseif t:is_punct("=") then
        -- = 0 / = delete / = default
        self._lex:next()
        local nt = self._lex:next()
        if nt.value == "0"               then qualifiers.pure      = true
        elseif nt:is_keyword("delete")   then qualifiers.deleted   = true
        elseif nt:is_keyword("default")  then qualifiers.defaulted = true
        end
        break
      else
        break
      end
      parser._yield()
    end

    -- skip constructor initializer list:  Foo() : base(a), m(b) { }
    if self._lex:peek():is_punct(":") then
      self._lex:next()
      local d = 0
      while true do
        local t = self._lex:peek()
        if t:is_eof() then break end
        if d == 0 and (t:is_punct("{") or t:is_punct(";")) then break end
        if t:is_punct("(") then d = d + 1 end
        if t:is_punct(")") then if d > 0 then d = d - 1 end end
        self._lex:next()
      end
    end

    -- skip inline body { ... }
    if self._lex:peek():is_punct("{") then self:_skip_braces(false) end
    self._lex:accept("punct", ";")

    -- if class_name and name and class_name == name then
    --   local n = ast.constructor.new(class_name, type_str, params, qualifiers)
    --   n.tags = node_tags
    --   return n
    -- end
    local n = ast.func.new(name or "?", type_str, params, qualifiers)
    n.tags = node_tags
    return n
  end

  -- ── FIELD / VARIABLE ──────────────────────────────────────────────────────
  if self._lex:accept("punct", "=") then
    local depth = 0
    while true do
      local t = self._lex:peek()
      if t:is_eof() then break end
      if depth == 0 and t:is_punct(";") then break end
      if t:is_punct("(") or t:is_punct("{") or t:is_punct("[") then depth = depth + 1 end
      if t:is_punct(")") or t:is_punct("}") or t:is_punct("]") then
        if depth == 0 then break else depth = depth - 1 end
      end
      self._lex:next()
      parser._yield()
    end
  end
  if self._lex:peek():is_punct("{") then self:_skip_braces(false) end
  self._lex:accept("punct", ";")

  local n = ast.field.new(name or "?", type_str, qualifiers)
  n.tags = node_tags
  return n
end

-- ── Declaration dispatcher ───────────────────────────────────────────────────

function parser:_parse_decl(class_name)
  local tok = self._lex:peek()

  -- template<...> prefix
  local template_params = nil
  if tok:is_keyword("template") then
    template_params = self:_parse_template_prefix()
    tok = self._lex:peek()
  end

  -- [[attributes]]
  local attributes = self:_parse_attributes()
  tok = self._lex:peek()

  if tok:is_keyword("namespace") then
    local n = self:_parse_namespace()
    n.template_params = template_params; n.attributes = attributes
    return n
  end

  if tok:is_keyword("class") or tok:is_keyword("struct") or tok:is_keyword("union") then
    local n = self:_parse_class_struct()
    n.template_params = template_params; n.attributes = attributes
    return n
  end

  if tok:is_keyword("enum") then
    local n = self:_parse_enum()
    n.template_params = template_params; n.attributes = attributes
    return n
  end

  if tok:is_keyword("typedef") then
    local n = self:_parse_typedef()
    n.template_params = template_params
    return n
  end

  if tok:is_keyword("using") then
    local n = self:_parse_using()
    n.template_params = template_params
    return n
  end

  -- access specifier  public: / protected: / private:
  if tok:is_keyword("public") or tok:is_keyword("protected") or tok:is_keyword("private") then
    local access = tok.value; self._lex:next()
    self._lex:accept("punct", ":")
    return ast.access.new(access)
  end

  -- extern "C" { ... }  or  extern "C" single-decl
  if tok:is_keyword("extern") then
    self._lex:next()
    local lt = self._lex:peek()
    if lt:is("string") then
      local linkage = self._lex:next().value
      if self._lex:peek():is_punct("{") then
        self._lex:next()
        local children = self:_parse_decl_list()
        self._lex:accept("punct", "}")
        self._lex:accept("punct", ";")
        local n = ast.extern_block.new(linkage, children)
        n.tags = self:_take_tags()
        n.template_params = template_params
        return n
      else
        local n = self:_parse_type_and_name_decl({})
        if n then n.extern_linkage = linkage; n.template_params = template_params end
        return n
      end
    else
      local n = self:_parse_type_and_name_decl({"extern"})
      if n then n.template_params = template_params; n.attributes = attributes end
      return n
    end
  end

  if tok:is_keyword("friend") then
    self._lex:next()
    local n = self:_parse_type_and_name_decl({})
    if n then n.is_friend = true; n.template_params = template_params end
    return n
  end

  if tok:is_keyword("static_assert") then
    self:_skip_to_semi(); return nil
  end

  if tok:is_punct(";") then
    self._lex:next(); return nil
  end

  -- general declaration
  local n = self:_parse_type_and_name_decl({},class_name)
  if n then n.template_params = template_params; n.attributes = attributes end
  return n
end

-- ── Declaration list ─────────────────────────────────────────────────────────

function parser:_parse_decl_list(class_name)
  local decls = {}
  while true do
    local tok = self._lex:peek()
    if tok:is_eof() then break end
    if tok:is_punct("}") then break end

    if tok:is_doc() then
      local v = self._lex:next().value
      self._pending_tag_text = self._pending_tag_text and (self._pending_tag_text .. "\n" .. v) or v
    else
      local stale = self._lex:peek()
      local result = self:_parse_decl(class_name)
      if result then
        table.insert(decls, result)
      elseif self._lex:peek() == stale then
        -- _parse_decl consumed nothing: skip the unknown token to avoid infinite spin
        error(string.format('parser: unexpected token %s at line %d', tostring(stale), stale.line))
      end
    end
    parser._yield()
  end
  return decls
end

-- ── Public API ────────────────────────────────────────────────────────────────

function parser:dump()
  self._lex:dump()
end

function parser:_parse_impl()
  local children = self:_parse_decl_list()
  return ast.file.new(children)
end

---Parse C++ source text and return a file-level AST node.
---@param source string preprocessed C++ source
---@return ast_file
---@param source string
function parser.parse(source, opts)
  local p = parser.new(source, opts)
  return p:_parse_impl()
end

return parser
