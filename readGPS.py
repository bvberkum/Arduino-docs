#!/usr/bin/env python
"""
TinyGPS++ 
reads from GPRMC, GPGGA
"""
import serial
from glob import glob


ports = glob('/dev/tty.usb*')
port = ports[0]


line_types = {
	'$PUBX':  {
		'00': ( "Time of Day and Clock Information", 
			""
		),
		'03': ( "Satellite Status", 
			"The PUBX,03 message contains satellite status information.", 
		),
		'04': ( "Lat/Long Position Data", 
			"This message contains position solution data."
		),
	},
	'$GPGSA': (
		"GSA", "GPS DOP and Active Satellites", "If less than 12 SVs are used "
		"for navigation, the remaining fields are left empty. If more "
		"than 12 SVs are used for navigation, only the IDs of the first 12 "
		"are output. "
		"- The SV Numbers (Fields 'Sv') are in the range of 1 to 32 for GPS "
		"satellites, and 33 to 64 "
		"for SBAS satellites (33 = SBAS PRN 120, 34 = SBAS PRN 121, and so "
		"on) ", 
		(), 20, []
	),
	'$GPGSV': (
		"GSV", "GPS Satellites in View", "The number of satellites in view, together "
		"with each PRN (SV ID), elevation and azimuth, "
		"and C/No (Signal/Noise Ratio) value. Only four satellite details are "
		"transmitted in one "
		"message. There are up to 4 messages used as indicated in the first "
		"field NoMsg. ",
		(), (7, 16), []
	),
	'$GPGLL': (
		"GLL", "Latitude and longitude, with time of position fix and status",
		"", (), (9, 10), [
			None,
			( 'Latitude', float, "" ),
			( 'N', str, "" ),
			( 'Longitude', float, "" ),
			( 'E', str, "" ),
			( 'hhmmss.ss', str, "" ),
			( 'A', str, "Valid" ),
			( '', 'A', str, "Mode" ),
			None
		]
	),
	'$GPGRS': (
		"GRS", "GNSS Range Residuals", "", 
		(), 17, []
	),
	'$GPGST': (
		"GST", "GNSS Pseudo Range Error Statistics", "", 
		(), 11, []
	),
	'$GPZDA': (
		"ZDA", "Time and Date", "", 
		(), 9, []
	),
	'$GPGBS': (
		"GBS", "GNSS Satellite Fault Detection", "",
		(), 11, [
			None,
			( 'hhmmss.ss', float, "UTC Time, Time to which this RAIM sentence belongs" ),
			( 'errlat', float, "Expected error in latitude" ),
			( 'errlong', float, "Expected error in longitude" ),
			( 'erralt', float, "Expected error in altitude" ),
			( 'svid', float, "Satellite ID of most likely failed satellite" ),
			( 'prob', float, "Probability of missed detection, no supported (empty)" ),
			( 'bias', float, "Estimate on most likely failed satellite (a priori residual)" ),
			( 'stddev', float, "Standard deviation of estimated bias" ),
		]
	),
	'$GPDTM': (
		"DTM", "Datum Reference", "This message gives the difference between the "
		"currently selected Datum, and the reference "
		"Datum. "
		"If the currently configured Datum is not WGS84 or WGS72, then the field "
		"LLL will be set to "
		"999, and the field LSD is set to a variable-lenght string, representing "
		"the Name of the "
		"Datum. The list of supported datums can be found in CFG-DAT. "
		"The reference Datum can not be changed and is always set to WGS84. ",
		(), 11, []
	),
	'$GPTXT': (
		"TXT", "Text Transmission", "This message outputs various information on "
		"the receiver, such as power-up screen",
		(0xF0, 0x41), 7, [
			None,
			('xx', int, "Total number of messages in this transmission, 01..99"),
			('yy', int, "Message number in this transmission, range 01..xx"),
			('zz', int, "Text identifier, u-blox GPS receivers specify the "
				"severity of the message with this number. "
				"- 00 = ERROR "
				"- 01 = WARNING "
				"- 02 = NOTICE "
				"- 07 = USER"
			),
			('string', str, "Any ASCII text"),
			None
		]
	),
	'$GPRMC': (
		"RMC", "Recommended Minimum data", "The Recommended Minimum sentence "
		"defined by NMEA for GPS/Transit system data.", 
		(), 15, [
			None,
			( 'hhmmss.ss', float, None, 'utc_time', "UTC Time, Time of position fix"),
			( 'status', str, None, 'status', "Status, V = Navigation receiver warning, A = Data valid"),
			( 'Latitude', float, None, 'latitude', "" ),
			( 'N', str, None, 'hemisphere', "" ),
			( 'Longitude', float, None, 'longitude', "" ),
			( 'E', str, None, 'hemisphere', "" ),
			( 'Spd', float, 'knots', 'spd', "Speed over ground" ),
			( 'Cog', float, 'degrees', 'cog', "Course over ground" ),
			( 'date', int, 'ddmmyy', 'date', "" ),
			( 'mv', int, 'degrees', 'mv', "Magnetic variation value, not being output by receiver" ),
			( 'mvE', str, None, '', "Magnetic variation E/W indicator, not being output by receiver" ),
			( 'mode', str, None, 'mode', "Mode Indicator, see Position Fix Flags description" ), 
			None
		]
	),
	'$GPVTG': (
		"VTG", "Course over ground and Ground speed",
		"Velocity is given as Course over Ground (COG) and Speed over Ground (SOG). ",
		( 0xF0, 0x05 ), 12, [
			None,
			('cogt', float, 'degrees', 'cogt', "Course over ground (true)"),
			None,#'T',
			('cogm', float, 'degrees', 'cogm', "Course over ground (magnatic)"),
			None,#'M',
			('sog', float, 'knots', 'sog', "Speed over ground"),
			None,#'N',
			('kph', float, 'km/h', 'kph', "Speed over ground"),
			None,#'K',
			('mode', str, None, 'mode', "Mode indicator"),
			None,
		]
	),
	'$GPGGA': (
		"GGA", "Global positioning system fix data", 
		"Time and position, together with GPS fixing related data (number of "
		"satellites in use, and "
		"the resulting HDOP, age of differential data if in use, etc.).",
		(), 17, [
		]
	),
}


class GPSInfo(object):

	def __init__(self, config):
		self._line_types = config
		self._line = ''
		self._read_lines = 0
		self._decoded_lines = 0
		self.fields = []

	def set_value(self, spec, msg):
		if '*' not in msg[-1]:
			return
		count = spec[4]
		if isinstance(count, tuple):
			s, e = count
			l = len(msg)-2 # ignore <cr><lf> field, and checksum
			if s < l and l < e:
				count = l
		else:
			count -= 2
			assert len(msg) == count, "Expected %s, got %i" %( count, len(msg), )
		lastf, chk = msg[-1].split('*')
		msg[-1] =lastf

		opt = []	
		for i, f in enumerate(spec[5]):
			if not f: continue
			if len(f) == 1:
				assert msg[i] == f[0], (f, msg)
				continue
			if not f[0]:
				f = f[1:]
				opt.append(i)
			if len(f) == 3:
				name, decode, descr = f
			else:
				name, decode, unit, varname, descr = f
			data = msg[i]
			if data:
				v = decode(data)
				setattr(self, name, v)
				if name not in self.fields:
					self.fields.append(name)

	def get_spec(self, msg):
		spec = self._line_types[msg[0]]
		if not spec:
			return
		assert len(spec) == 6
		fields = spec[5]
		if not fields:
			return
		return spec

	def decode_line(self, line):
		self._read_lines += 1
		msg = line.split(',')
		line_type = msg[0]

		if line_type not in self._line_types:
			print '---', 'Missing message type:', msg[0]

		elif self._line_types[line_type]:
			d = self._line_types[line_type]
			if isinstance(d, dict):
				if msg[1] in d:
					pass
				else:
					print 'UBX,'+msg[1], msg[2:]
			else:
				spec = self.get_spec(msg)
				if spec:
					self.set_value(spec, msg)
					return msg[0]
				#else:
				#	print 'missing', msg
		else:
			assert False

	def decode(self, c):
		if not c:
			return
		if c == '\r':
			return
		if c == '\n':
			m = self.decode_line(self._line)
			self._line = ''
			return m
		self._line += c


if __name__ == '__main__':

	baudrate = 9600
	s = serial.Serial(port=port, baudrate=baudrate, bytesize=8, parity='N', stopbits=1,
			timeout=None, xonxoff=0, rtscts=0, writeTimeout=None, dsrdtr=None)
	print 'open', port, baudrate

	#s.write('UBX,41,1,0007,0003,38400,0,*25\r\n')

	#baudrate = 38400
	#s = serial.Serial(port=port, baudrate=baudrate, bytesize=8, parity='N', stopbits=1,
	#		timeout=None, xonxoff=0, rtscts=0, writeTimeout=None, dsrdtr=None)
	#print 'open', port, baudrate

	info = GPSInfo( line_types )
	while True:
		x = info.decode(s.read())
		if x:
			print x, 
			for n in info.fields:
				if n in ['mode', 'status']:
					continue
				print n, getattr(info, n),
			if info.fields:
				print
				info.fields=[]

	print 'Done'

