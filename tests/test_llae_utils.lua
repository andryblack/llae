local lu = require('luaunit')
local utils = require('llae.utils')

TestUtils = {}

function TestUtils:test_clear_table()
    local t = {1, 2, 3, a = "test", b = {1, 2}}
    utils.clear_table(t)
    lu.assertEquals(next(t), nil)
end

function TestUtils:test_merge()
    local t1 = {a = 1, b = 2, c = 3}
    local t2 = {b = 4, d = 5}
    local t3 = {d = 6, e = 7}
    
    local result = utils.merge(t1, t2)
    lu.assertEquals(result, {a = 1, b = 4, c = 3, d = 5})
    
    result = utils.merge(t1, t2, t3)
    lu.assertEquals(result, {a = 1, b = 4, c = 3, d = 6, e = 7})
    
    -- Original tables should not be modified
    lu.assertEquals(t1, {a = 1, b = 2, c = 3})
    lu.assertEquals(t2, {b = 4, d = 5})
    lu.assertEquals(t3, {d = 6, e = 7})
    
    -- Test with nil values
    result = utils.merge(t1, nil, t3)
    lu.assertEquals(result, {a = 1, b = 2, c = 3, d = 6, e = 7})
end

function TestUtils:test_list_concat()
    local t1 = {1, 2, 3}
    local t2 = {4, 5, 6}
    
    local result = utils.list_concat(t1, t2)
    lu.assertEquals(result, {1, 2, 3, 4, 5, 6})
    
    -- Original tables should not be modified
    lu.assertEquals(t1, {1, 2, 3})
    lu.assertEquals(t2, {4, 5, 6})
    
    -- Test with empty tables
    result = utils.list_concat({}, t2)
    lu.assertEquals(result, {4, 5, 6})
    
    result = utils.list_concat(t1, {})
    lu.assertEquals(result, {1, 2, 3})
    
    result = utils.list_concat({}, {})
    lu.assertEquals(result, {})
end

function TestUtils:test_reversedipairs()
    local t = {1, 2, 3, 4, 5}
    local expected = {5, 4, 3, 2, 1}
    local result = {}
    
    for _, v in utils.reversedipairs(t) do
        table.insert(result, v)
    end
    
    lu.assertEquals(result, expected)
    
    -- Test with empty table
    result = {}
    for _, v in utils.reversedipairs({}) do
        table.insert(result, v)
    end
    lu.assertEquals(result, {})
    
    -- Test with single element
    result = {}
    for _, v in utils.reversedipairs({1}) do
        table.insert(result, v)
    end
    lu.assertEquals(result, {1})
end

function TestUtils:test_replace_env()
    -- Test basic replacement using real environment variable
    local home = os.getenv("HOME")
    if home then
        local result = utils.replace_env("Path: ${HOME}/docs")
        lu.assertEquals(result, "Path: " .. home .. "/docs")
    end
    
    -- Test non-existent variable
    local result = utils.replace_env("${NON_EXISTENT}")
    lu.assertEquals(result, "${NON_EXISTENT}")
    
    -- Test no variables
    result = utils.replace_env("Plain text")
    lu.assertEquals(result, "Plain text")
    
    -- Test multiple variables if PATH is available
    local path = os.getenv("PATH")
    if path then
        result = utils.replace_env("HOME=${HOME} PATH=${PATH}")
        lu.assertEquals(result, "HOME=" .. home .. " PATH=" .. path)
    end
end

function TestUtils:test_replace_tokens()
    local tokens = {
        name = "John",
        age = "30",
        city = "New York"
    }
    
    -- Test basic replacement
    local result = utils.replace_tokens("Name: ${name}", tokens)
    lu.assertEquals(result, "Name: John")
    
    -- Test multiple replacements
    result = utils.replace_tokens("${name} is ${age} years old and lives in ${city}", tokens)
    lu.assertEquals(result, "John is 30 years old and lives in New York")
    
    -- Test non-existent token
    result = utils.replace_tokens("${unknown}", tokens)
    lu.assertEquals(result, "${unknown}")
    
    -- Test no tokens
    result = utils.replace_tokens("Plain text", tokens)
    lu.assertEquals(result, "Plain text")
end

function TestUtils:test_parse_args()
    -- Test basic argument parsing
    local args = {
        [0] = "script.lua",
        "--verbose",
        "input.txt",
        "--output=result.txt",
        "extra"
    }
    local result = utils.parse_args(args)
    lu.assertEquals(result[0], "script.lua")
    lu.assertEquals(result[1], "input.txt")
    lu.assertEquals(result[2], "extra")
    lu.assertTrue(result.verbose)
    lu.assertEquals(result.output, "result.txt")
    
    -- Test with no options
    args = {[0] = "script.lua", "file1", "file2"}
    result = utils.parse_args(args)
    lu.assertEquals(result[0], "script.lua")
    lu.assertEquals(result[1], "file1")
    lu.assertEquals(result[2], "file2")
    
    -- Test with only options
    args = {[0] = "script.lua", "--opt1=val1", "--opt2=val2", "--flag"}
    result = utils.parse_args(args)
    lu.assertEquals(result[0], "script.lua")
    lu.assertEquals(result.opt1, "val1")
    lu.assertEquals(result.opt2, "val2")
    lu.assertTrue(result.flag)
    
    -- Test empty args
    args = {[0] = "script.lua"}
    result = utils.parse_args(args)
    lu.assertEquals(result[0], "script.lua")
    lu.assertEquals(next(result, 0), nil)
end
