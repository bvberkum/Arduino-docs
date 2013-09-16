"""
readhex?

Working
- This talks to a RadioLink JeeNode over a RS232/USB adapter.

ToDo
- receive announce w/ config
- serve vfs
- write/maintain log
- Would be nice to have local flash (card) based log, backup battery 
  and serial commands for dumping.

"""

import os, sys, time
if sys.platform == 'win32':
	from twisted.internet import win32eventreactor
	win32eventreactor.install()
import struct
import logging

from twisted.python import log, usage
from twisted.internet import reactor
from twisted.protocols.basic import LineReceiver
import serial

from tmp import decode_avr_struct


sketches = { }
values = { }
nodes = {
		'23': [
			('light', 8, int),
			('rhum', 7, int),
			('temp', 10, int),
			('ctemp', 10, int),
			('memfree', 16, int),
			('lobat', 1, bool),
			]
		}


class Martador(LineReceiver):

	def processReport(self, node_id, *payload):
		print payload, len(payload)
		report = []
		fmt = nodes[node_id]
		try:
			report = decode_avr_struct(payload, fmt)
		except Exception, e:
			print "processReport error:", e
			return
		for i, f in enumerate(fmt):
			if node_id not in values:
				values[node_id] = {}
			values[node_id][f[0]] = report[i]
			print node_id, f[0], report[i]

	def lineReceived(self, line):
		if line == ' A i1 g5 @ 868 MHz ':
			print line
			self.sendLine("1q")
			return
		elif line.startswith('>'):
			print line
			return
		node_id = None
		data = line.strip().split(' ')
		lkey = data.pop(0)
		if lkey == '?':
			return
		elif lkey == 'OKX':
			return
		elif lkey == 'OK':
			node_id = data.pop(0)
			print 'From: ', node_id
		else:
			logging.error('Unrecognized line %r' % line)
			return;
		try:
			self.processReport(node_id, *data)
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


