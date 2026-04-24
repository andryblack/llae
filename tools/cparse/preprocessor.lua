local class = require 'llae.class'

---@class preprocessor
---@field new fun(inc_handler: fun(filename: string): string | nil): preprocessor
---@field _defines table<string, string>
local preprocessor = class(nil,'preprocessor')

---Constructor
---@param inc_handler fun(filename: string): string | nil Handler for #include directive
function preprocessor:_init(inc_handler)
    self._defines = {}
    self._func_defines = {}
    self._inc_handler = inc_handler or function() return nil end
    self._include_stack = {}
end

---Define a macro
---@param token string
---@param value string | nil
function preprocessor:define(token, value)
    local normalized = (token or ""):match("^%s*(.-)%s*$") or ""
    local macro_value = value or ""
    local rest = normalized
    if macro_value ~= "" then
        rest = rest .. " " .. macro_value
    end
    self:_define_from_rest(rest)
end

---Undefine a macro
---@param token string
function preprocessor:undef(token)
    self._defines[token] = nil
    self._func_defines[token] = nil
end

---Check if a macro is defined
---@param token string
---@return boolean
function preprocessor:is_defined(token)
    return self._defines[token] ~= nil or self._func_defines[token] ~= nil
end

---Get the value of a macro
---@param token string
---@return string | nil
function preprocessor:get_define(token)
    return self._defines[token]
end

local BYTE_SPACE  = string.byte(" ")
local BYTE_TAB    = string.byte("\t")
local BYTE_LPAREN = string.byte("(")
local BYTE_RPAREN = string.byte(")")
local BYTE_COMMA  = string.byte(",")
local BYTE_BANG   = string.byte("!")
local BYTE_AMP    = string.byte("&")
local BYTE_PIPE   = string.byte("|")
local BYTE_EQ     = string.byte("=")
local BYTE_LT     = string.byte("<")
local BYTE_GT     = string.byte(">")

local function trim_string(s)
    return s:match("^%s*(.-)%s*$") or ''
end

---@param rest string
function preprocessor:_define_from_rest(rest)
    rest = trim_string(rest or "")
    if rest == "" then
        return
    end
    -- function-like: NAME(params) body
    local name, params_str, body = rest:match("^([%w_]+)(%b())%s*(.*)")
    if name and params_str then
        local params = {}
        for param in params_str:sub(2, -2):gmatch("[%w_]+") do
            table.insert(params, param)
        end
        self._func_defines[name] = {
            params = params,
            body = trim_string(body),
        }
        self._defines[name] = nil
        return
    end
    -- object-like: NAME body
    local token, value = rest:match("^([%w_]+)%s*(.*)")
    if token then
        self._defines[token] = trim_string(value)
        self._func_defines[token] = nil
    end
end

---Evaluate a simple expression (supports defined(), identifiers, numbers, ==, !=, &&, ||, !, <, >, <=, >=)
---@param expr string
---@return boolean
function preprocessor:evaluate_expression(expr)
    expr = trim_string(expr or "")
    if expr == "" then return false end

    local pos = 1
    local len = #expr

    

    local function skip_ws()
        local b = expr:byte(pos)
        while b == BYTE_SPACE or b == BYTE_TAB do
            pos = pos + 1
            b = expr:byte(pos)
        end
    end

    -- forward declaration for mutual recursion
    local parse_or

    local function parse_primary()
        skip_ws()
        -- parenthesized expression
        if expr:byte(pos) == BYTE_LPAREN then
            pos = pos + 1
            local val = parse_or()
            skip_ws()
            if expr:byte(pos) == BYTE_RPAREN then pos = pos + 1 end
            return val
        end
        -- number literal
        local num = expr:match("^%d+", pos)
        if num then
            pos = pos + #num
            return tonumber(num)
        end
        -- identifier or keyword
        local id = expr:match("^[%a_][%w_]*", pos)
        if id then
            pos = pos + #id
            if id == "defined" then
                skip_ws()
                local has_paren = expr:byte(pos) == BYTE_LPAREN
                if has_paren then pos = pos + 1 end
                skip_ws()
                local name = expr:match("^[%a_][%w_]*", pos) or ""
                pos = pos + #name
                if has_paren then
                    skip_ws()
                    if expr:byte(pos) == BYTE_RPAREN then pos = pos + 1 end
                end
                return self:is_defined(name) and 1 or 0
            end
            if self:is_defined(id) then
                local val = self:get_define(id)
                if val == "" then return 1 end
                return tonumber(val) or 1
            end
            return 0
        end
        return 0
    end

    local function parse_unary()
        skip_ws()
        if expr:byte(pos) == BYTE_BANG then
            pos = pos + 1
            return parse_unary() == 0 and 1 or 0
        end
        return parse_primary()
    end

    local function parse_cmp()
        local left = parse_unary()
        while true do
            skip_ws()
            local b0 = expr:byte(pos)
            local b1 = expr:byte(pos + 1)
            local op
            if b1 == BYTE_EQ and (b0 == BYTE_EQ or b0 == BYTE_BANG or b0 == BYTE_LT or b0 == BYTE_GT) then
                op = expr:sub(pos, pos + 1); pos = pos + 2
            elseif b0 == BYTE_LT or b0 == BYTE_GT then
                op = expr:sub(pos, pos); pos = pos + 1
            else
                break
            end
            local right = parse_unary()
            if     op == "==" then left = left == right and 1 or 0
            elseif op == "!=" then left = left ~= right and 1 or 0
            elseif op == "<"  then left = left <  right and 1 or 0
            elseif op == ">"  then left = left >  right and 1 or 0
            elseif op == "<=" then left = left <= right and 1 or 0
            elseif op == ">=" then left = left >= right and 1 or 0
            end
        end
        return left
    end

    local function parse_and()
        local left = parse_cmp()
        while true do
            skip_ws()
            if expr:byte(pos) == BYTE_AMP and expr:byte(pos + 1) == BYTE_AMP then
                pos = pos + 2
                local right = parse_cmp()
                left = (left ~= 0 and right ~= 0) and 1 or 0
            else
                break
            end
        end
        return left
    end

    parse_or = function()
        local left = parse_and()
        while true do
            skip_ws()
            if expr:byte(pos) == BYTE_PIPE and expr:byte(pos + 1) == BYTE_PIPE then
                pos = pos + 2
                local right = parse_and()
                left = (left ~= 0 or right ~= 0) and 1 or 0
            else
                break
            end
        end
        return left
    end

    local result = parse_or()
    return result ~= 0
end

---Parse call arguments starting after '(' at paren_pos, return args table and position after ')'
---@param line string
---@param paren_pos integer position of '('
---@return table, integer
local function parse_call_args(line, paren_pos)
    local len = #line
    local i = paren_pos + 1
    local depth = 1
    local args = {}
    local cur = {}
    while i <= len and depth > 0 do
        local b = line:byte(i)
        if b == BYTE_LPAREN then
            depth = depth + 1
            table.insert(cur, "(")
        elseif b == BYTE_RPAREN then
            depth = depth - 1
            if depth == 0 then
                table.insert(args, trim_string(table.concat(cur)))
                cur = {}
            else
                table.insert(cur, ")")
            end
        elseif b == BYTE_COMMA and depth == 1 then
            table.insert(args, trim_string(table.concat(cur)))
            cur = {}
        else
            table.insert(cur, line:sub(i, i))
        end
        i = i + 1
    end
    return args, i
end

---Find next word-boundary occurrence of name followed by '(' in line, starting from pos
---@param line string
---@param name string
---@param nlen integer
---@param pos integer
---@return integer|nil, integer|nil  name_start, paren_pos
local function find_func_call(line, name, nlen, pos)
    local len = #line
    local search = pos
    while search <= len - nlen + 1 do
        local i = line:find(name, search, true)
        if not i then break end
        local before_ok = (i == 1) or not line:sub(i - 1, i - 1):match("[%w_]")
        if before_ok then
            local j = i + nlen
            while j <= len and (line:byte(j) == BYTE_SPACE or line:byte(j) == BYTE_TAB) do
                j = j + 1
            end
            if line:byte(j) == BYTE_LPAREN then
                return i, j
            end
        end
        search = i + 1
    end
    return nil, nil
end

---Expand one function-like macro throughout a line.
---Creates a child preprocessor with params defined as args, then expands the body.
---@param line string
---@param name string
---@param params table
---@param body string
---@return string
function preprocessor:_expand_func_call(line, name, params, body)
    local out = {}
    local pos = 1
    local nlen = #name

    while pos <= #line do
        local name_start, paren_pos = find_func_call(line, name, nlen, pos)

        if not name_start then
            table.insert(out, line:sub(pos))
            break
        end

        table.insert(out, line:sub(pos, name_start - 1))

        local args, next_pos = parse_call_args(line, paren_pos)

        -- build param→arg lookup
        local param_map = {}
        for pi, param in ipairs(params) do
            param_map[param] = args[pi] or ""
        end

        -- apply ## (token concatenation) operator first
        local processed_body = body:gsub("([%w_]+)%s*##%s*([%w_]+)", function(left, right)
            return (param_map[left] or left) .. (param_map[right] or right)
        end)

        -- apply # (stringify) operator — only single #, ## is already gone
        processed_body = processed_body:gsub("#([%a_][%w_]*)", function(param)
            if param_map[param] then
                local arg = param_map[param]
                arg = arg:gsub('\\', '\\\\'):gsub('"', '\\"')
                return '"' .. arg .. '"'
            end
            return "#" .. param
        end)

        -- child preprocessor inherits all defines and adds param→arg bindings
        local child = preprocessor.new()
        for k, v in pairs(self._defines) do child._defines[k] = v end
        for k, v in pairs(self._func_defines) do child._func_defines[k] = v end
        for param, arg in pairs(param_map) do
            child._defines[param] = arg
        end

        table.insert(out, child:expand_macros(processed_body))
        pos = next_pos
    end

    return table.concat(out)
end

---Expand macros in a string
---@param line string
---@return string
function preprocessor:expand_macros(line)
    if line:match("^%s*//") then return line end

    local result = line

    -- expand function-like macros first (sorted longest name first)
    local func_defs = {}
    for k, v in pairs(self._func_defines) do
        table.insert(func_defs, {key = k, params = v.params, body = v.body})
    end
    table.sort(func_defs, function(a, b) return #a.key > #b.key end)
    for _, def in ipairs(func_defs) do
        result = self:_expand_func_call(result, def.key, def.params, def.body)
    end

    -- expand object-like macros (sorted longest name first)
    local obj_defs = {}
    for k, v in pairs(self._defines) do
        table.insert(obj_defs, {key = k, value = v})
    end
    table.sort(obj_defs, function(a, b) return #a.key > #b.key end)
    for _, def in ipairs(obj_defs) do
        result = result:gsub("%f[%w_]" .. def.key .. "%f[^%w_]", def.value)
    end

    return result
end


---Process source code
---@param data string
---@return string
function preprocessor:process(data)
    local lines = {}
    local output = {}
    
    -- Split into lines
    for line in data:gmatch("[^\r\n]+") do
        table.insert(lines, line)
    end

    -- Phase 2: splice backslash-continued lines
    local spliced = {}
    local j = 1
    while j <= #lines do
        local line = lines[j]
        while line:sub(-1) == "\\" and j < #lines do
            j = j + 1
            line = line:sub(1, -2) .. "\n" .. lines[j]
        end
        table.insert(spliced, line)
        j = j + 1
    end
    lines = spliced
    
    local skip = false
    local conditional_stack = {}

    local i = 1
    while i <= #lines do
        local line = lines[i]
        local trimmed = line:match("^%s*(.-)%s*$")

        -- Check for preprocessor directive
        local directive = trimmed:match("^#(%w+)")

        -- For non-directive lines, join subsequent lines while parens are unbalanced
        -- (multiline function-like macro calls)
        if not directive then
            local depth = 0
            for ci = 1, #trimmed do
                local b = trimmed:byte(ci)
                if b == BYTE_LPAREN then depth = depth + 1
                elseif b == BYTE_RPAREN then depth = depth - 1
                end
            end
            while depth > 0 and i < #lines do
                i = i + 1
                local next = lines[i]
                line = line .. "\n" .. next
                for ci = 1, #next do
                    local b = next:byte(ci)
                    if b == BYTE_LPAREN then depth = depth + 1
                    elseif b == BYTE_RPAREN then depth = depth - 1
                    end
                end
            end
            trimmed = trim_string(line)
        end
        
        if directive then
            directive = directive:lower()
            
            if directive == "define" then
                if not skip then
                    local rest = trimmed:match("^#define%s+(.+)")
                    if rest then
                        self:_define_from_rest(rest)
                    end
                end
                
            elseif directive == "undef" then
                if not skip then
                    local token = trimmed:match("^#undef%s+([%w_]+)")
                    if token then
                        self:undef(token)
                    end
                end
                
            elseif directive == "ifdef" then
                local token = trimmed:match("^#ifdef%s+([%w_]+)")
                if token then
                    local cond_skip = not self:is_defined(token)
                    table.insert(conditional_stack, {prev_skip = skip, cond_skip = cond_skip, branch_taken = not cond_skip, else_found = false})
                    skip = skip or cond_skip
                end

            elseif directive == "ifndef" then
                local token = trimmed:match("^#ifndef%s+([%w_]+)")
                if token then
                    local cond_skip = self:is_defined(token)
                    table.insert(conditional_stack, {prev_skip = skip, cond_skip = cond_skip, branch_taken = not cond_skip, else_found = false})
                    skip = skip or cond_skip
                end

            elseif directive == "if" then
                local expr = trimmed:match("^#if%s+(.+)")
                if expr then
                    local cond_skip = not self:evaluate_expression(expr)
                    table.insert(conditional_stack, {prev_skip = skip, cond_skip = cond_skip, branch_taken = not cond_skip, else_found = false})
                    skip = skip or cond_skip
                end

            elseif directive == "elif" then
                if #conditional_stack > 0 then
                    local top = conditional_stack[#conditional_stack]
                    if not top.else_found then
                        local expr = trimmed:match("^#elif%s+(.+)")
                        if expr then
                            if top.branch_taken or top.prev_skip then
                                -- ветка уже взята или родитель пропускает — пропустить
                                skip = true
                            else
                                local cond_skip = not self:evaluate_expression(expr)
                                top.cond_skip = cond_skip
                                if not cond_skip then top.branch_taken = true end
                                skip = cond_skip
                            end
                        end
                    end
                end

            elseif directive == "else" then
                if #conditional_stack > 0 then
                    local top = conditional_stack[#conditional_stack]
                    top.else_found = true
                    if top.branch_taken or top.prev_skip then
                        skip = true
                    else
                        top.branch_taken = true
                        skip = false
                    end
                end

            elseif directive == "endif" then
                if #conditional_stack > 0 then
                    local top = table.remove(conditional_stack)
                    skip = top.prev_skip
                end
                
            elseif directive == "include" then
                if not skip then
                    local filename = trimmed:match('^#include%s*"<([^">]+)">')
                    if not filename then
                        filename = trimmed:match("^#include%s*<([^>]+)>")
                    end
                    if not filename then
                        filename = trimmed:match('^#include%s*"([^"]+)"')
                    end
                    
                    if filename and self._inc_handler then
                        -- Check for circular includes
                        for _, f in ipairs(self._include_stack) do
                            if f == filename then
                                table.insert(output, "// Circular include detected: " .. filename)
                                goto continue
                            end
                        end
                        
                        table.insert(self._include_stack, filename)
                        local content = self._inc_handler(filename)
                        if content then
                            table.insert(output, "// Begin include: " .. filename)
                            table.insert(output, self:process(content))
                            table.insert(output, "// End include: " .. filename)
                        else
                            table.insert(output, "// Include not found: " .. filename)
                        end
                        table.remove(self._include_stack)
                    end
                end
                ::continue::
                
            elseif directive == "pragma" then
                if not skip then
                    local pragma_content = trimmed:match("^#pragma%s+(.+)")
                    if pragma_content then
                        table.insert(output, "/// @pragma( " .. pragma_content .. " )")
                    end
                end
                
            elseif directive == "error" then
                if not skip then
                    local error_msg = trimmed:match("^#error%s+(.+)")
                    if error_msg then
                        table.insert(output, "// #error: " .. error_msg)
                    end
                end
                
            elseif directive == "warning" then
                if not skip then
                    local warning_msg = trimmed:match("^#warning%s+(.+)")
                    if warning_msg then
                        table.insert(output, "// #warning: " .. warning_msg)
                    end
                end
                
            elseif directive == "line" then
                -- #line directive - ignore for now
                
            else
                -- Unknown directive, keep as comment
                if not skip then
                    table.insert(output, "// Unknown directive: " .. trimmed)
                end
            end
        else
            -- Not a directive
            if not skip then
                table.insert(output, self:expand_macros(trimmed))
            end
        end

        i = i + 1
    end
    
    return table.concat(output, "\n")
end

return preprocessor
