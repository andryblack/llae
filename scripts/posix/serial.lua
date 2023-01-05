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
	[460800] = termios.B460800,
	[500000] = termios.B500000,
	[576000] = termios.B576000,
	[921600] = termios.B921600,
	[1000000] = termios.B1000000,
	[1152000] = termios.B1152000,
	[1500000] = termios.B1500000,
	[2000000] = termios.B2000000,
	[2500000] = termios.B2500000,
	[3000000] = termios.B3000000,
	[3500000] = termios.B3500000,
	[4000000] = termios.B4000000,
}

function serial:configure(conf)
	local options = self._options or assert(termios.tcgetattr(self._fd))
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

 	-- ee: http://unixwiz.net/techtips/termios-vmin-vtime.html
    options.c_cc[termios.VMIN]  = 1;            -- read doesn't block
    options.c_cc[termios.VTIME] = 0;            --

    options.c_iflag = options.c_iflag & ~(termios.IXON | termios.IXOFF | termios.IXANY); -- shut off xon/xoff ctrl

	assert(termios.tcsetattr(self._fd,termios.TCSANOW,options))
	self._options = options
end

function serial:set_baudrate(baudrate)
	local options = self._options or assert(termios.tcgetattr(self._fd))
	local baud = baudrates[baudrate] or error('unexpected baudrate')
	assert(termios.cfsetispeed(options,baud))
	assert(termios.cfsetospeed(options,baud))
	assert(termios.tcsetattr(self._fd,termios.TCSANOW,options))
	self._options = options
end

function serial:set_flowcontrol(flowcontrol)
	local options = self._options or assert(termios.tcgetattr(self._fd))
	if flowcontrol then
        --// with flow control
        options.c_cflag = toptions.c_cflag | CRTSCTS
    else
        --// no flow control
        options.c_cflag = options.c_cflag & ~termios.CRTSCTS
    end
    assert(termios.tcsetattr(self._fd,termios.TCSANOW,options))
	self._options = options
end

function serial:set_parity(parity)
	local options = self._options or assert(termios.tcgetattr(self._fd))
	if not parity or parity == 'none' then
		options.c_cflag = options.c_cflag & ~termios.PARENB
        options.c_cflag = options.c_cflag & ~termios.PARODD
    elseif parity == 'even' then
    	options.c_cflag = options.c_cflag | termios.PARENB
        options.c_cflag = options.c_cflag & ~termios.PARODD
    elseif parity == 'odd' then
    	options.c_cflag = options.c_cflag | termios.PARENB
        options.c_cflag = options.c_cflag | termios.PARODD
    else
    	error('unexpected parity')
    end
    assert(termios.tcsetattr(self._fd,termios.TCSANOW,options))
	self._options = options
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
	while true do
		local flags,err = self._poll:poll(uv.poll.WRITABLE)
		if not flags then
			return nil,err
		end
		--print('****',flags,err)
		if flags & uv.poll.WRITABLE then
			return self._fd:write(data)
		end
	end
end


return serial