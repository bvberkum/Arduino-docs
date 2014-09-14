#!/usr/bin/env python
"""download - retrieve flash/EEPROM using avrdude and sort, parse

Usage:
    download.py [options] (x|y|z)

Options:
    -s
    -h --help     Print program arguments help.
    --version     Print program/version.

TODO: build EEROM data for uC
"""
from docopt import docopt
__prog__ = 'avr-mpe'
__version__ = '0.0.0'


def h_s(opts):
    print 's'

handlers = dict([ ( name[2:].replace('_','-'), h )
    for name, h in locals().items() 
    if name.startswith('h_')
])

def c_x(opts):
    print 'x'
def c_y(opts):
    print 'y'
def c_z(opts):
    print 'z'

commands = dict([ ( name[2:].replace('_','-'), h )
    for name, h in locals().items() 
    if name.startswith('c_')
])

def get_version():
    return __prog__ +'/'+ __version__

if __name__ == '__main__':
    opts = docopt(__doc__, version=get_version())
    cmds = [ h for name, h in commands.items() 
            if name in opts and opts[name] ]
    opth = [ h for name, h in handlers.items() 
            if '-'+name in opts and opts['-'+name] 
            or '--'+name in opts and opts['--'+name] 
        ]
    
    for h in opth:
        h(opts)

    import sys
    for c in cmds:
        r = c(opts)
        if r: sys.exit(r)

