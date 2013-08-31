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







