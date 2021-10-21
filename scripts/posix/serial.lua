local posix = require 'posix'
local termios = require 'posix.termios'
local class = require 'llae.class'
local uv = require 'uv'

local serial = class(nil,'serial')

function serial.open(path,conf)
	local fd,err = posix.open(path,posix.O_RDWR  | posix.O_NOCTTY | posix.O_NONBLOCK)
	if not fd then
		return nil,err
	end
	local r = serial.new(fd)
	if conf then
		r:configure(conf)
	end
	return r
end

function serial:_init(fd)
	self._fd = fd
	self._poll = uv.poll.new(fd)
end

local baudrates = {
	[9600  ] = termios.B9600  ,
	[19200 ] = termios.B19200 ,
	[38400 ] = termios.B38400 ,
	[57600 ] = termios.B57600 ,
	[115200] = termios.B115200,
	[230400] = termios.B230400,
}

function serial:configure(conf)
	local options = assert(termios.tcgetattr(self._fd))
	local baud = baudrates[conf.baudrate] or error('unexpected baudrate')
	assert(termios.cfsetispeed(options,baud))
	assert(termios.cfsetospeed(options,baud))
	
	options.c_cflag = options.c_cflag | (termios.CLOCAL | termios.CREAD)
	options.c_cflag = options.c_cflag & ~termios.PARENB
	options.c_cflag = options.c_cflag & ~termios.CSTOPB --; // 8n1
	options.c_cflag = options.c_cflag & ~termios.CSIZE
	options.c_cflag = options.c_cflag | termios.CS8
	options.c_cflag = options.c_cflag & ~termios.CRTSCTS

	options.c_iflag = options.c_iflag & ~termios.IGNBRK;
	options.c_iflag = options.c_iflag & ~(termios.INLCR | termios.IGNCR | termios.ICRNL | (termios.IUCLC or 0)); -- no char processing
	options.c_lflag = 0;                -- no signaling chars, no echo,
                                        -- no canonical processing
 	options.c_oflag = 0;              -- no remapping, no delays

    options.c_cc[termios.VMIN]  = 0;            -- read doesn't block
    options.c_cc[termios.VTIME] = 0;            --

    options.c_iflag =options.c_iflag & ~(termios.IXON | termios.IXOFF | termios.IXANY); -- shut off xon/xoff ctrl

	assert(termios.tcsetattr(self._fd,termios.TCSANOW,options))
end

function serial:read(buf_or_len)
	while true do
		local flags,err = self._poll:poll(uv.poll.READABLE)
		if not flags then
			return nil,err
		end
		--print('****',flags,err)
		if flags & uv.poll.READABLE then
			return self._fd:read(buf_or_len or 1024)
		end
	end
end

function serial:write(data)
	return self._fd:write(data) -- @todo poll writable
end


return serial