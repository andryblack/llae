# Logging Module

The `log` module provides functionality for console output with colored logging levels and progress bar visualization.

## Color Constants

### Foreground Colors (`log.fg`)
Available colors for text:
- `black`, `red`, `green`, `yellow`, `blue`, `magenta`, `cyan`, `white`
- `bright_black`, `bright_red`, `bright_green`, `bright_yellow`, `bright_blue`, `bright_magenta`, `bright_cyan`, `bright_white`

### Background Colors (`log.bg`)
Available colors for background:
- `black`, `red`, `green`, `yellow`, `blue`, `magenta`, `cyan`, `white`
- `bright_black`, `bright_red`, `bright_green`, `bright_yellow`, `bright_blue`, `bright_magenta`, `bright_cyan`, `bright_white`

### Text Styles
- `log.bold`: Bold text style
- `log.italic`: Italic text style
- `log.reset`: Reset all styles and colors

## Logging Functions

### `log.set_verbose(v)`
Sets the verbosity level for debug messages.

**Parameters:**
- `v` (boolean): If true, debug messages will be shown

**Example:**
```lua
local log = require 'llae.log'
log.set_verbose(true)  -- Enable debug messages
```

### `log.info(...)`
Prints an info message with green [I] prefix.

**Parameters:**
- `...`: Values to print

**Example:**
```lua
local log = require 'llae.log'
log.info('Starting application')  -- Prints "[I] Starting application" in green
```

### `log.debug(...)`
Prints a debug message with blue [D] prefix. Only prints if verbose mode is enabled.

**Parameters:**
- `...`: Values to print

**Example:**
```lua
local log = require 'llae.log'
log.debug('Debug value:', 42)  -- Prints "[D] Debug value: 42" in blue if verbose is enabled
```

### `log.error(...)`
Prints an error message with red [E] prefix.

**Parameters:**
- `...`: Values to print

**Example:**
```lua
local log = require 'llae.log'
log.error('Failed to open file')  -- Prints "[E] Failed to open file" in red
```

### `log.warning(...)`
Prints a warning message with yellow [W] prefix.

**Parameters:**
- `...`: Values to print

**Example:**
```lua
local log = require 'llae.log'
log.warning('Resource usage high')  -- Prints "[W] Resource usage high" in yellow
```

## Progress Bar

### `log.progress(width)`
Creates and displays a new progress bar.

**Parameters:**
- `width` (number, optional): Width of the progress bar in characters. Defaults to 70.

**Returns:**
- (object): A progress bar object with the following methods:
  - `update(count, total)`: Updates the progress bar
  - `close()`: Removes the progress bar

**Example:**
```lua
local log = require 'llae.log'
local progress = log.progress(50)  -- Create a 50-character wide progress bar

-- Update progress with count and total
for i = 1, 100 do
    progress:update(i, 100)
    -- Do some work
end

-- Or update progress incrementally
for i = 1, 100 do
    progress:update()  -- Advances the progress bar by one step
end

progress:close()  -- Remove the progress bar
```

## Terminal Control

### `log.restart_line`
ANSI escape sequence to move cursor up one line and clear it. Useful for updating the current line.

**Example:**
```lua
local log = require 'llae.log'
log.info("Old text")
log.info(log.restart_line .. "New text")  -- Replaces "Old text" with "New text"
```

## Styling Examples

```lua
local log = require 'llae.log'

-- Colored text
log.info(log.fg.red .. "Red text" .. log.reset)
log.info(log.fg.bright_blue .. "Bright blue text" .. log.reset)

-- Background color
log.info(log.bg.yellow .. "Text with yellow background" .. log.reset)

-- Combined styles
log.info(log.bold .. log.fg.green .. "Bold green text" .. log.reset)
log.info(log.italic .. log.fg.cyan .. "Italic cyan text" .. log.reset)

-- Complex formatting
log.info(log.fg.white .. log.bg.blue .. log.bold .. "White bold text on blue background" .. log.reset)
``` 