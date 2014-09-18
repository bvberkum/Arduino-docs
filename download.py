#!/usr/bin/env python
"""download - retrieve AVR EEPROM using avrdude and sort, parse.

For non-global avr install use bin-path, ie.:
    /Applications/Arduino.app/Contents/Resources/Java/

Usage:
    download.py [options] (get|read|erase)
    download.py [options] --bin-path PATH get

Options:
    --hex-file PATH 
                   [default: download.hex]
    --baud RATE    [default: 57600]
    --port DEVICE  [default: /dev/ttyUSB* /dev/tty.usbserial-*]
    --chip PART    [default: m328p]
    --protocol NAME 
                   [default: arduino]
    --method NAME  To select a preset chip/device/rate.
    --bin AVRDUDE  [default: avrdude]
    --bin-path AVRPATH 
                   Used to determine avrdude, config path from Arduino install.
    -h --help      Print program arguments help.
    --version      Print program/version.

TODO: see prototype EEPROM for struct: Node, SensorNode
    - char node[4] Report payload type.
    - int node_id Instance number.
    - int version Sketch code version.
    - char config_id[4] Unique sketch ID.

TODO: build EEROM data for uC
"""
import os, re, struct
from shutil import move
from glob import glob
from subprocess import Popen, call
try:
    import syck
    yaml_load = syck.load
    yaml_dump = syck.dump
except ImportError, e:
    try:
        import yaml
        yaml_load = yaml.load
        yaml_dump = yaml.dump
    except ImportError, e:
        print >>sys.stderr, "confparse.py: no YAML parser"

from docopt import docopt

__prog__ = 'avr-mpe'
__version__ = '0.0.0'


def h_method(opts):
    opts['flags']['method'] = opts['--method']
def h_baud(opts):
    opts['flags']['baud'] = opts['--baud']
def h_port(opts):
    opts['flags']['port'] = opts['--port']
def h_protocol(opts):
    opts['flags']['protocol'] = opts['--protocol']
def h_chip(opts):
    opts['flags']['chip'] = opts['--chip']
def h_bin_path(opts):
    path = opts['--bin-path']
    opts['flags']['bin'] = os.path.join(path, 'bin/avrdude')
    opts['flags']['config'] = os.path.join(path, 'etc/avrdude.conf')

handlers = dict([ ( name[2:].replace('_','-'), h )
    for name, h in locals().items() 
    if name.startswith('h_')
])


def run(argstr, flags):
    args = re.sub('\s+', ' ',  flags['bin'] + argstr ).split(' ')
    call(args)

def run_eeprom(filename, mode, outtype, flags):
# outtype:
# 'r': raw LE
# 'i': intel hex
# 'd': decimal
# 'h': hex
    argstr = ( ' -C %(config)s -c %(protocol)s -P %(port)s -b %(baud)s '
            ' -p %(chip)s' ) % flags
    argstr += ' -U eeprom:%s:%s:%s ' % ( mode, filename, outtype )
    run(argstr, flags)

def c_erase(flags, **opts):
    argstr = ( ' -C %(config)s -c %(protocol)s -P %(port)s -b %(baud)s '
            ' -p %(chip)s' ) % flags
    argstr += ' -e '
    run(argstr, flags)

def c_get(flags, **opts):
    hexfile = opts['--hex-file']
    print "Reading to", hexfile
    run_eeprom(hexfile, 'r', 'r', flags)

def c_read(flags, **opts):
    hexfile = opts['--hex-file']
    data = open(hexfile).read()
    node, nid, version, config_id = struct.unpack('<4sHH4s', data[:12])
    node = node.strip('\0')
    nodes = yaml_load(open('avr-nodes.yml').read())
    node_id = node+str(nid)
    if node_id not in nodes:
        nodes[node_id] = dict(
                    node=node,
                    chip=flags['chip'],
                    protocol=flags['protocol'],
                    baudrate=int(flags['baud']),
                    version=version,
                    eeprom=dict(
                        file='firmware/eeprom/'+node_id+'.hex',
                        size=len(data)
                    )
                )
        yaml_dump(nodes, open( 'avr-nodes.yml', 'w+' ))
    if hexfile != nodes[node_id]['eeprom']['file']:
        move('download.hex', nodes[node_id]['eeprom']['file'])
    print 'Node:', node
    print 'Node ID:', node_id
    print 'Version:', version
    print 'Config ID:', config_id
    print 'Firmware:', nodes[node_id]['eeprom']['file']


commands = dict([ ( name[2:].replace('_','-'), h )
    for name, h in locals().items() 
    if name.startswith('c_')
])


def get_version():
    return __prog__ +'/'+ __version__

if __name__ == '__main__':
    opts = docopt(__doc__, version=get_version())
    opts['flags'] = {}
    cmds = [ h for name, h in commands.items() 
            if name in opts and opts[name] ]
    opth = [ h for name, h in handlers.items() 
            if '-'+name in opts and opts['-'+name] 
            or '--'+name in opts and opts['--'+name] 
        ]
    for h in opth:
        h(opts)
    for p in opts['--port'].split(' '):
        match = glob(p)
        if match:
            opts['flags']['port'] = match[0]
            break
    import sys
    assert sys.byteorder == 'little'
    for c in cmds:
        r = c(**opts)
        if r: sys.exit(r)

