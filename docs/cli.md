# LLAE CLI Reference

LLAE provides a command-line interface with several commands for managing projects, running scripts, and more. Here's a comprehensive guide to all available commands.

## Basic Usage

```bash
llae <command> [arguments]
```

To get help on any command:
```bash
llae help [command]
```

## Available Commands

### init

Initializes a new LLAE project.

```bash
llae init <project-name> [options]
```

**Options:**
- `--modules=<path>` - Additional modules location
- `--project-dir=<path>` - Project directory

**Example:**
```bash
# Create new project in current directory
llae init myproject

# Create project in specific directory
llae init myproject --project-dir=/path/to/dir
```

Creates a new project with the following structure:
```
myproject/
├── bin/
├── modules/
├── scripts/
├── build/
│   └── premake/
├── .gitignore
└── llae-project.lua
```

### install

Installs modules in a project.

```bash
llae install [module-name] [options]
```

**Options:**
- `--file=<path>` - Install module from file
- `--modules=<path>` - Additional modules location
- `--project-dir=<path>` - Project directory

**Examples:**
```bash
# Install all project dependencies
llae install

# Install specific module
llae install mymodule

# Install from file
llae install --file=./mymodule.zip
```

### run

Runs a Lua script or project command.

```bash
llae run <script> [arguments]
```

The command can execute:
- Lua script files
- Project-defined commands from llae-project.lua
- Project executables

**Examples:**
```bash
# Run a Lua script
llae run scripts/myscript.lua

# Run with arguments
llae run scripts/myscript.lua --arg1=value1

# Run project command
llae run test
```

### server

Starts a simple HTTP file server.

```bash
llae server [options]
```

**Options:**
- `--port=<number>` - TCP listening port (default: 8000)
- `--bind=<address>` - Bind address (default: 127.0.0.1)
- `--root=<path>` - Root folder (default: current directory)

**Examples:**
```bash
# Start server on default port
llae server

# Start server on specific port and address
llae server --port=3000 --bind=0.0.0.0

# Serve specific directory
llae server --root=/path/to/dir
```

### bootstrap

Bootstraps a new LLAE installation.

```bash
llae bootstrap
```

This command:
1. Creates LLAE directory structure
2. Installs core modules (premake, llae)
3. Runs module-specific bootstrap actions

### upgrade

Upgrades an existing LLAE installation.

```bash
llae upgrade
```

This command:
1. Updates core LLAE modules
2. Runs module-specific upgrade actions
3. Updates project files if needed

### help

Shows usage information for commands.

```bash
# Show all available commands
llae help

# Show help for specific command
llae help <command>
```

## Project Configuration

Projects are configured through `llae-project.lua` files. Example configuration:

```lua
-- Project configuration
project 'myproject'

-- Add dependencies
module 'llae'
module 'mymodule'

-- Define custom commands
command {
    name = 'test',
    script = 'scripts/test.lua',
    args = {'--verbose'}
}
```

## Environment Variables

The CLI respects the following environment variables:
- `LLAE_HOME` - LLAE installation directory (default: ~/.llae)
- `LLAE_PATH` - Additional module search paths

