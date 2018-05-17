import bluetooth
import os

class BThandler:

	def __init__(self, mqtt_handler, nodes_queue, conf, cond):
		self.mqtt_handler = mqtt_handler
		self.nodes_queue = nodes_queue
		self.client_socks = []
		self.conf = conf
		self.cond = cond

	def recv_full(self, sock, n):
	# Helper function to recv n bytes or return None if EOF is hit
		data = b''
		while len(data) < n:
			packet = sock.recv(n - len(data))
			if not packet:
				return None
			data += packet
		return data

	def request_measurement(self, bin_id):
		for sock in self.client_socks:
			if sock[0]==bin_id:
				try:
					sock[1].send(self.conf['measurement_request'])
					resp = self.recv_full(sock[1], 1)

					print("resp: ", int.from_bytes(resp, byteorder='little',signed=False), sep="\t")

					return int.from_bytes(resp, byteorder='little', signed=False)
				except Exception as e:
					print('Failed with:', e)


	def get_response(self, bin_id):
		for sock in self.socks:
			if sock[0]==bin_id:
				try:
					resp=sock.recv(24)
					return resp
				except TimeoutException:
					return self.conf['timeout']


	def run(self):

		while True:
			try:
				clients = bluetooth.discover_devices()
				for client in clients:
					name = bluetooth.lookup_name(client)
					if name  is not None:
						if name.startswith(self.conf['bt_prefix']):
							print(name)
							#TODO pair
							#Open a connection to the node
							sock = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
							sock.connect((client,1))
							sock.settimeout(50)
							sock.sendall(self.conf['name_request'])
							name = str(self.recv_full(sock, 24), 'utf-8')
							self.client_socks.append((name, sock))
							print('connected to %s' % str(name))
							# Subscribe to the new node's channel to receive its updates
							self.mqtt_handler.subscribe_to_node(name)
							self.nodes_queue.put(bin_id=name, msg={'reqId':-1})
							try:
								self.cond.acquire()
								self.cond.notify()
							finally:
								self.cond.release()
			except OSError as e:
					if e.errno==19:
						print("Seems like bluetooth is not properly configured. Exiting..")
						os._exit(23)
					else:
						print('Failed with:',e)	
						pass


