"""

martador.linkread (TODO)
    - parse, possibly interactive with RadioLink sketch.
    - only emit valid values, validate against known types.
    - timestamp and emit messages through ZMQ.

martador.logger (TODO)
    - Write messages to plain text log.

martador.vftree (TODO)
    - A filesystem representing the WSN, using ZMQ for IPC to linkread process.
    - Possibly to configure and service remote nodes. 

martador.rrd (TODO)
    - Write to Munin files directly, possibly later to update from off-line logs.

"""

import bitstring


def decode_avr_struct(data, fmt):
	data_len = sum([f[1] for f in fmt])
	data_bin = [
			bin(int(d))[2:] for d in data
		]
	# First padd all bytes, according to exact payload_len
	for x, p in enumerate(data_bin):
		padd = 8
		remaining = data_len - ( x * 8 )
		if remaining < 8:
			padd = remaining
		data_bin[x] = p.rjust(padd, '0')
	# reverse this sequence
	data_bin.reverse()
	bits = ''.join(data_bin)
	assert len(bits) == data_len, (len(bits), data_len)
	#print 'Bits:', bits
	a = bitstring.ConstBitStream(bin='0b'+bits)
	types = [ "%s:%i" % (f[2].__name__, f[1]) for f in fmt ]
	values = [] 
	while types:
		# Now read fields in reverse
		tp = types.pop()
		if tp == 'bool:1': tp = 'bool'
		values.append(a.read(tp))
	values.reverse()
	return values


if __name__ == '__main__':
	print "ROOM-5-23 0 54 230 25 1623 0"
	pl = (0, 54, 115, 50, 184, 50, 0)
	print decode_avr_struct(pl, [
				('light', 8, int),
				('rhum', 7, int),
				('temp', 10, int),
				('ctemp', 10, int),
				('memfree', 16, int),
				('lobat', 1, bool),
		])


