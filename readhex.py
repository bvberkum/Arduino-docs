"""

"""
"""
import bitstring

# Payload bitfields: LDR, DHT11 rhum, DHT11 temp, ATmega temp, lowbat
payload_len = 8 + 10 + 10 + 10  + 1
print 'Packet length', payload_len

payloads = (
		"0 24 193 163 1", # 0 280 240 26 0
		"0 22 205 163 1", # 0 278 243 26 0
	)
print payloads

payloads_bin = [
		[ 
			bin(int(b))[2:] for b in p.split(' ') 
		] for p in payloads
	]
for i, p in enumerate(payloads_bin):
	# First padd all bytes, according to exact payload_len
	for x, p in enumerate(payloads_bin[i]):
		padd = 8
		remaining = payload_len - ( x * 8 )
		if remaining < 8:
			padd = remaining
		payloads_bin[i][x] = payloads_bin[i][x].rjust(padd, '0')
	# reverse this sequence
	payloads_bin[i].reverse()
print payloads_bin 


for b in payloads_bin:
	bits = ''.join(b)
	a = bitstring.ConstBitStream(bin='0b'+bits)
	# Now read payload in reverse
	print  \
		a.read("bool"), \
		a.read("int:10"),\
		a.read("int:10"),\
		a.read("int:10"),\
		a.read("int:8")
"""

import os, sys, time
if sys.platform == 'win32':
	from twisted.internet import win32eventreactor
	win32eventreactor.install()

from twisted.python import log, usage
from twisted.internet import reactor
from twisted.protocols.basic import LineReceiver
import serial


sketches = {
		'ROOM': 0,
		'MOIST': 0
	}
nodes = {}


class Martador(LineReceiver):

	def processData(self, *args):
		print args

	def lineReceived(self, line):
		print line
		return
		try:
			data = line.split()
			self.processData(*data)
		except ValueError:
			logging.error('Unable to parse data %s' % line)
			return


class MartadorOptions(usage.Options):
	optFlags = [
	]
	optParameters = [
		['outfile', 'o', None, 'Logfile [default: sys.stdout]'],
		['baudrate', 'b', 57600, 'Serial baudrate'],
		['port', 'p', '/dev/ttyUSB0', 'Serial Port device'],
	]


if __name__ == '__main__':

	from twisted.internet import reactor
	from twisted.internet.serialport import SerialPort

	o = MartadorOptions()
	try:
		o.parseOptions()
	except usage.UsageError, errortext:
		print '%s: %s' % (sys.argv[0], errortext)
		print '%s: Try --help for usage details.' % (sys.argv[0])
		raise SystemExit, 1

	logFile = o.opts['outfile']
	if logFile is None:
		logFile = sys.stdout
	log.startLogging(logFile)

	if o.opts['baudrate']:
		baudrate = int(o.opts['baudrate'])

	serial.Serial(port=None, baudrate=baudrate, bytesize=8, parity='N', stopbits=1,
			timeout=None, xonxoff=0, rtscts=0, writeTimeout=None, dsrdtr=None)

	log.msg('Attempting to open %s at %dbps' % (o.opts['port'], baudrate))
	s = SerialPort(Martador(), o.opts['port'], reactor, baudrate=baudrate)
	log.msg("Running")
	reactor.run()


