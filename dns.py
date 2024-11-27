#!/usr/bin/python2

# Python3 has some problems handling the client end.

import sys
import socket
import base64

class DNSQuery:
	def __init__(self, data):
		self.data = data
		self.data_text = ""

		tipo = (ord(data[2]) >> 3) & 15
		if tipo == 0:
			ini = 12
			lon = ord(data[ini])
		while lon != 0:
			self.data_text += data[ini+1 : ini+1+lon] + '.'
			ini += lon + 1
			lon = ord(data[ini])

	def request(self, ip):
		packet = ""
		if self.data_text:
			packet += self.data[:2] + "\x81\x80" + self.data[4:6] * 2 + "\x00" * 4 + self.data[12:]
			packet += "\xc0\x0c\x00\x01\x00\x01\x00\x00\x00\x3c\x00\x04"
			packet += str.join('', map(lambda x: chr(int(x)), ip.split('.')))

		return packet

def save(data):
	for name, value in data.iteritems():
		content = ""
		for block in value:
			content += block[:-1].replace('*', '+')

		content = base64.b64decode(content)
		open(name, "wb").write(content)


if __name__ == '__main__':
	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

	try: sock.bind(("0.0.0.0", 53))
	except:
		print("[-] Error: Can't bind to port 53.")
		sys.exit(1)

	print "[*] DNS Listening on 0.0.0.0:53"
	print "[*] Once files have sent, use CTRL+C to exit and save.\n"

	try:
		ddata = {}
		c = 0
		while 1:
			data, addr = sock.recvfrom(65565)
			dq = DNSQuery(data)
			sock.sendto(dq.request("0.0.0.0"), addr)

			rsplit = dq.data_text.split('.')
			rsplit.pop()

			dlen = len(rsplit)
			fname = ""
			tmp = []

			for n in range(dlen):
				if rsplit[n][len(rsplit[n])-1] == '-':
					tmp.append(rsplit[n])

				else:
					fname += rsplit[n] + '.'

			fname = fname[:-1]
			if fname not in ddata:
				ddata[fname] = []

			c += 1
			sys.stdout.write("[>] Packets Received: %d\r" % c)
			for d in tmp: ddata[fname].append(d)

	except:
		save(ddata)
		print "\n[*] Saved."
		print "[-] Exiting..."