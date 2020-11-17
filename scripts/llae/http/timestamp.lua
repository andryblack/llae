

local months_s = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" 
}
local months = {}
for i,n in ipairs(months_s) do
	months[n]=i
end

local timestamp = {}

function timestamp.parse( str )
	local dn,d,mn,y,h,m,s = string.match(str,'^(...), (%d%d) (...) (%d%d%d%d) (%d%d):(%d%d):(%d%d) GMT$')
	if dn then
		local last_time = os.time{
			year = tonumber(y),
			month = months[mn],
			day = tonumber(d),
			hour = tonumber(h),
			min = tonumber(m),
			sec = tonumber(s)
		}
		return last_time
	end
end

function timestamp.format( ts )
	return os.date('%a, %d %b %Y %H:%M:%S GMT',ts)
end

return timestamp