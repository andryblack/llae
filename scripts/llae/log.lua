---The log module provides functionality for console output with colored logging levels and progress bar visualization.
---@class llae.log
local _M = {}

local _red = '\x1b[31m'
local _white = '\x1b[37m'
local _green = '\x1b[32m'
local _blue = '\x1b[34m'
local _yellow = '\x1b[33m'
local _reset = '\x1b[0m'
local _bold = '\x1b[1m'
local _italic = '\x1b[3m'

local _prev_line = '\x1b[1A'
local _clear_line = '\x1b[K'

local colors = {'black','red','green','yellow','blue','magenta','cyan','white'}

---@type table<string,string>
---Foreground colors for text styling.
_M.fg = {}
---@type table<string,string>
---Background colors for text styling.
_M.bg = {}

for i,v in ipairs(colors) do
	_M.fg[v] = '\x1b[' .. (30+i-1) .. 'm'
	_M.bg[v] = '\x1b[' .. (40+i-1) .. 'm'
	_M.fg['bright_'..v] = '\x1b[' .. (90+i-1) .. 'm'
	_M.bg['bright_'..v] = '\x1b[' .. (100+i-1) .. 'm'
end

--- Bold text style.
_M.bold = _bold
--- Italic text style.
_M.italic = _italic
--- Reset all styles and colors.
_M.reset = _reset


--- Sets the verbosity level for debug messages.
---@param v boolean If true, debug messages will be shown
function _M.set_verbose( v )
	_M._verbose = v
end

local llae = require 'llae'

local function set_prefixes()
	if llae.log.set_console_prefix then
		llae.log.set_console_prefix(llae.log.level.debug,_blue .. '[D]' .. _reset)
		llae.log.set_console_prefix(llae.log.level.info,_green .. '[I]' .. _reset)
		llae.log.set_console_prefix(llae.log.level.warning,_yellow .. '[W]' .. _reset)
		llae.log.set_console_prefix(llae.log.level.error,_red .. '[E]' .. _reset)
		llae.log.set_console_prefix(llae.log.level.fatal,_red .. '[F]' .. _reset)
	end
end

set_prefixes()
if llae.log.print then
	print = llae.log.print
end
if not llae.log.print_level then
	llae.log.print_level = {
		debug = '[D]',
		info = '[I]',
		warning = '[W]',
		error = '[E]',
		fatal = '[F]'
	}
end

function _M.decolorize()
	_red = ''
	_white = ''
	_green = ''
	_blue = ''
	_yellow = ''
	_reset = ''
	_bold = ''
	_italic = ''
	_M.bold = ''
	_M.italic = ''
	_M.reset = ''
	for _,v in ipairs(colors) do
		_M.fg[v] = ''
		_M.bg[v] = ''
		_M.fg['bright_'..v] = ''
		_M.bg['bright_'..v] = ''
	end

	set_prefixes()
end


--- Prints an info message with green [I] prefix.
---@param ... any Values to print
function _M.info( ... )
	print(llae.log.print_level.info,...)
end

--- Prints a debug message with blue [D] prefix. Only prints if verbose mode is enabled.
---@param ... any Values to print
function _M.debug( ... )
	if _M._verbose then
		print(llae.log.print_level.debug,...)
	end
end

--- Prints an error message with red [E] prefix.
---@param ... any Values to print
function _M.error( ... )
	print(llae.log.print_level.error,...)
end

--- Prints a warning message with yellow [W] prefix.
---@param ... any Values to print
function _M.warning( ... )
	print(llae.log.print_level.warning,...)
end

local progress = nil

local function get_progress_class()
	if progress then
		return progress
	end
	local class = require 'llae.class'
	---@class llae.log.progress
	---Progress bar for visualizing task completion.
	---@field new fun(width:integer?) : llae.log.progress
	progress = class(nil,'llae.log.progress')

	function progress:_init( width )
		self._width = width or 70
		self._fill = 0
	end

	--- Shows the initial progress bar.
	function progress:show(  )
		print(_white..'[  0%]'.._blue..'['..string.rep(' ',self._width)..']'.._reset)
	end

	--- Updates the progress bar with count and total, or advances by one step.
	---@param count integer? Current count (if provided with total)
	---@param total integer? Total count (if provided with count)
	function progress:update( count, total )
		local fill = 0
		if count and total and total > 0 and  (count<=total) then
			fill =  math.ceil(count*self._width/total)
		else
			fill = self._fill + 1
			if fill > self._width then
				fill = 0
			end
		end
		print(_prev_line.._clear_line..
			_white..'['..string.format('%3d%%',math.ceil(fill*100/self._width))..']'..
			_blue..'['..string.rep('=',fill)..
			string.rep(' ',(self._width-fill))..']'.._reset)
		self._fill = fill
	end

	--- Removes the progress bar from display.
	function progress:close(  )
		print(_prev_line.._clear_line)
	end

	return progress
end

--- Creates and displays a new progress bar.
---@param width integer? Width of the progress bar in characters (default: 70)
---@return llae.log.progress A progress bar object with update and close methods
function _M.progress( width )
	local progress = get_progress_class()
	local p = progress.new(width)
	p:show()
	return p
end

--- ANSI escape sequence to move cursor up one line and clear it. Useful for updating the current line.
_M.restart_line = _prev_line.._clear_line


_M._log_file_handler = nil

function _M.redirect(filename,with_time)
	_M.decolorize()
	local llae = require 'llae'
	if _M._log_file_handler then
		llae.log.remove_handler(_M._log_file_handler)
	else
		llae.log.remove_stdout_handler()
	end
	local fs = require 'llae.fs'
	local file = fs.open(filename,fs.O_WRONLY|fs.O_CREAT|fs.O_APPEND)
	if not file then
		error('failed to open file: ' .. filename)
	end

	_M._log_file_handler = llae.log.add_file_handler(file,with_time)
end


return _M