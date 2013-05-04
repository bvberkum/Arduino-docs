#!/usr/bin/env python
"""
SERTest is a simple example using the SerialPort transport and the NMEA 0183
and Rockwell Zodiac SER protocols to display fix data as it is received from
the device.
"""
import sys
if sys.platform == 'win32':
    from twisted.internet import win32eventreactor
    win32eventreactor.install()

from twisted.python import log, usage
from twisted.internet import reactor
from twisted.protocols.basic import LineReceiver
import serial



class SER(LineReceiver):

    def processData(self, *args):
        data = list(args)
        if data:
        	key = data.pop(0)
        if data:
        	value = data.pop(0)
        if key == 'Humidity:':
        	open('/tmp/mpe_node1_temp', 'w+').write("%s" % value)
        	print 'hum', value
        elif key == 'Temperature:':
        	open('/tmp/mpe_node1_hum', 'w+').write("%s" % value)
        	print 'temp', value
        else:
        	print args

    def lineReceived(self, line):
        try:
            data = line.split()
            self.processData(*data)
        except ValueError:
            logging.error('Unable to parse data %s' % line)
            return

#class SERFixLogger:
#    def handle_fix(self, *args):
#      """
#      handle_fix gets called whenever either rockwell.Zodiac or nmea.NMEAReceiver
#      receives and decodes fix data.  Generally, SER receivers will report a
#      fix at 1hz. Implementing only this method is sufficient for most purposes
#      unless tracking of ground speed, course, utc date, or detailed satellite
#      information is necessary.
#
#      For example, plotting a map from MapQuest or a similar service only
#      requires longitude and latitude.
#      """
#      log.msg('fix:\n' + 
#      '\n'.join(map(lambda n: '  %s = %s' % tuple(n), zip(('utc', 'lon', 'lat', 'fix', 'sat', 'hdp', 'alt', 'geo', 'dgp'), map(repr, args)))))


class SEROptions(usage.Options):
    optFlags = [
    ]
    optParameters = [
        ['outfile', 'o', None, 'Logfile [default: sys.stdout]'],
        ['baudrate', 'b', 38400, 'Serial baudrate'],
        ['port', 'p', '/dev/ttyUSB0', 'Serial Port device'],
    ]


if __name__ == '__main__':

    from twisted.internet import reactor
    from twisted.internet.serialport import SerialPort


    o = SEROptions()
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

    serial.Serial(port=None, baudrate=38400, bytesize=8, parity='N', stopbits=1,
            timeout=None, xonxoff=0, rtscts=0, writeTimeout=None, dsrdtr=None)

    log.msg('Attempting to open %s at %dbps' % (o.opts['port'], o.opts['baudrate']))
    s = SerialPort(SER(), o.opts['port'], reactor, baudrate=int(o.opts['baudrate']))
    log.msg("Running")
    reactor.run()


