---@meta posix.termios

---The termios module provides terminal I/O control functionality.
---Supports terminal configuration, baud rate settings, and control flags.
---@class posix.termios
local termios = {}

---Terminal I/O control structure containing terminal configuration.
---@class posix.termios.termios
---@field c_iflag integer Input mode flags
---@field c_oflag integer Output mode flags  
---@field c_cflag integer Control mode flags
---@field c_lflag integer Local mode flags
---@field c_cc integer[] Control characters array

local termios_struct = {}


termios.termios = termios_struct

--- Gets terminal attributes for a file descriptor.
---@param fd posix.fd The file descriptor
---@return posix.termios.termios? The termios structure on success
---@return string? Error message if getting attributes fails
function termios.tcgetattr(fd) end

--- Sets terminal attributes for a file descriptor.
---@param fd posix.fd The file descriptor
---@param optional_actions integer When to apply changes (TCSANOW, TCSADRAIN, TCSAFLUSH)
---@param tios posix.termios.termios The termios structure
---@return integer? Status code on success
---@return string? Error message if setting attributes fails
function termios.tcsetattr(fd, optional_actions, tios) end

--- Sets input baud rate for a termios structure.
---@param tios posix.termios.termios The termios structure
---@param speed integer Baud rate constant (e.g., B9600, B115200)
---@return boolean? True on success
---@return string? Error message if setting speed fails
function termios.cfsetispeed(tios, speed) end

--- Sets output baud rate for a termios structure.
---@param tios posix.termios.termios The termios structure
---@param speed integer Baud rate constant (e.g., B9600, B115200)
---@return boolean? True on success
---@return string? Error message if setting speed fails
function termios.cfsetospeed(tios, speed) end

--- Sets custom baud rate for a file descriptor (macOS only).
---@param fd posix.fd The file descriptor
---@param speed integer Custom baud rate value
---@return boolean? True on success
---@return string? Error message if setting baud rate fails
function termios.set_baudrate(fd, speed) end

-- Terminal control action constants
---@type integer
termios.TCSANOW = 0    -- Apply changes immediately
---@type integer  
termios.TCSADRAIN = 0  -- Apply changes after output is drained
---@type integer
termios.TCSAFLUSH = 0  -- Apply changes after output is drained and input is flushed

-- Input mode flags (c_iflag)
---@type integer
termios.IGNBRK = 0     -- Ignore break condition
---@type integer
termios.BRKINT = 0     -- Break generates SIGINT
---@type integer
termios.IGNPAR = 0     -- Ignore characters with parity errors
---@type integer
termios.PARMRK = 0     -- Mark parity errors
---@type integer
termios.INPCK = 0      -- Enable input parity check
---@type integer
termios.ISTRIP = 0     -- Strip character
---@type integer
termios.INLCR = 0      -- Map NL to CR on input
---@type integer
termios.IGNCR = 0      -- Ignore CR
---@type integer
termios.ICRNL = 0      -- Map CR to NL on input
---@type integer
termios.IUCLC = 0      -- Map uppercase to lowercase on input
---@type integer
termios.IXON = 0       -- Enable XON/XOFF flow control on output
---@type integer
termios.IXANY = 0      -- Any character will restart stopped output
---@type integer
termios.IXOFF = 0      -- Enable XON/XOFF flow control on input
---@type integer
termios.IMAXBEL = 0    -- Ring bell when input queue is full

-- Output mode flags (c_oflag)
---@type integer
termios.OPOST = 0      -- Enable output processing
---@type integer
termios.OLCUC = 0      -- Map lowercase to uppercase on output
---@type integer
termios.ONLCR = 0      -- Map NL to CR-NL on output
---@type integer
termios.OCRNL = 0      -- Map CR to NL on output
---@type integer
termios.ONOCR = 0      -- No CR output at column 0
---@type integer
termios.ONLRET = 0     -- NL performs CR function
---@type integer
termios.OFILL = 0      -- Use fill characters for delay
---@type integer
termios.OFDEL = 0      -- Fill is DEL, else NUL
---@type integer
termios.NLDLY = 0      -- Newline delay mask
---@type integer
termios.CRDLY = 0      -- Carriage return delay mask
---@type integer
termios.TABDLY = 0     -- Horizontal tab delay mask
---@type integer
termios.BSDLY = 0      -- Backspace delay mask
---@type integer
termios.VTDLY = 0      -- Vertical tab delay mask
---@type integer
termios.FFDLY = 0      -- Form feed delay mask

-- Control mode flags (c_cflag)
---@type integer
termios.CBAUD = 0      -- Baud rate mask
---@type integer
termios.CBAUDEX = 0    -- Extra baud rate mask
---@type integer
termios.CSIZE = 0      -- Character size mask
---@type integer
termios.CSTOPB = 0     -- Send two stop bits, else one
---@type integer
termios.CREAD = 0      -- Enable receiver
---@type integer
termios.PARENB = 0     -- Parity enable
---@type integer
termios.PARODD = 0     -- Odd parity, else even
---@type integer
termios.HUPCL = 0      -- Hang up on last close
---@type integer
termios.CLOCAL = 0     -- Ignore modem status lines
---@type integer
termios.LOBLK = 0      -- Block output from a non-current layer
---@type integer
termios.CIBAUD = 0     -- Input baud rate mask
---@type integer
termios.CRTSCTS = 0    -- Enable RTS/CTS (hardware) flow control

-- Local mode flags (c_lflag)
---@type integer
termios.ISIG = 0       -- Enable signals
---@type integer
termios.ICANON = 0     -- Canonical input (erase and kill processing)
---@type integer
termios.XCASE = 0      -- Canonical upper/lower presentation
---@type integer
termios.ECHO = 0       -- Enable echo
---@type integer
termios.ECHOE = 0      -- Echo erase character as error-correcting backspace
---@type integer
termios.ECHOK = 0     -- Echo KILL
---@type integer
termios.ECHONL = 0     -- Echo NL
---@type integer
termios.ECHOCTL = 0    -- Echo control characters as ^char
---@type integer
termios.ECHOPRT = 0    -- Echo erased character
---@type integer
termios.ECHOKE = 0     -- Visual erase for KILL
---@type integer
termios.DEFECHO = 0    -- Echo only when a single process group is in the foreground
---@type integer
termios.FLUSHO = 0     -- Output is being flushed
---@type integer
termios.NOFLSH = 0     -- Don't flush after interrupt or quit
---@type integer
termios.TOSTOP = 0     -- Send SIGTTOU for background output
---@type integer
termios.PENDIN = 0     -- Retype pending input at next read or input character
---@type integer
termios.IEXTEN = 0     -- Enable implementation-defined input processing

-- Control character indices (c_cc array)
---@type integer
termios.VINTR = 0      -- Interrupt character
---@type integer
termios.VQUIT = 0      -- Quit character
---@type integer
termios.VERASE = 0     -- Erase character
---@type integer
termios.VKILL = 0      -- Kill character
---@type integer
termios.VEOF = 0       -- End-of-file character
---@type integer
termios.VMIN = 0       -- Minimum number of characters for noncanonical read
---@type integer
termios.VEOL = 0       -- End-of-line character
---@type integer
termios.VTIME = 0      -- Timeout in deciseconds for noncanonical read
---@type integer
termios.VEOL2 = 0      -- Second end-of-line character
---@type integer
termios.VSWTCH = 0     -- Switch character
---@type integer
termios.VSTART = 0     -- Start character
---@type integer
termios.VSTOP = 0      -- Stop character
---@type integer
termios.VSUSP = 0      -- Suspend character
---@type integer
termios.VDSUSP = 0     -- Delayed suspend character
---@type integer
termios.VLNEXT = 0     -- Literal next character
---@type integer
termios.VWERASE = 0    -- Word erase character
---@type integer
termios.VREPRINT = 0   -- Reprint character
---@type integer
termios.VDISCARD = 0   -- Discard character
---@type integer
termios.VSTATUS = 0    -- Status character

-- Baud rate constants
---@type integer
termios.B9600 = 0      -- 9600 baud
---@type integer
termios.B19200 = 0     -- 19200 baud
---@type integer
termios.B38400 = 0     -- 38400 baud
---@type integer
termios.B57600 = 0     -- 57600 baud
---@type integer
termios.B115200 = 0    -- 115200 baud
---@type integer
termios.B230400 = 0    -- 230400 baud
---@type integer
termios.B460800 = 0    -- 460800 baud
---@type integer
termios.B500000 = 0    -- 500000 baud
---@type integer
termios.B576000 = 0    -- 576000 baud
---@type integer
termios.B921600 = 0    -- 921600 baud
---@type integer
termios.B1000000 = 0   -- 1000000 baud
---@type integer
termios.B1152000 = 0   -- 1152000 baud
---@type integer
termios.B1500000 = 0   -- 1500000 baud
---@type integer
termios.B2000000 = 0   -- 2000000 baud
---@type integer
termios.B2500000 = 0   -- 2500000 baud
---@type integer
termios.B3000000 = 0   -- 3000000 baud
---@type integer
termios.B3500000 = 0   -- 3500000 baud
---@type integer
termios.B4000000 = 0   -- 4000000 baud

-- Character size constants
---@type integer
termios.CS8 = 0        -- 8 data bits

return termios
