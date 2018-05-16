import paho.mqtt.client as mqtt
import re
import json
import configparser

'''
MQTT handler
'''

class MQTThandler:

	def __init__(self, nodes_queue, cond, config):
		self.nodes_queue = nodes_queue
		self.cond = cond
		self.config = config
	
		# Initiate MQTT Client
		self.mqttc = mqtt.Client()
		self.mqttc.username_pw_set(config['mqtt']['auth_user'],
		                               password=config['mqtt']['auth_psw'])


		# Register Event Handlers
		self.mqttc.on_message = self._on_message
		self.mqttc.on_connect = self._on_connect
		self.mqttc.on_subscribe = self._on_subscribe

		# Connect with MQTT Broker
		self.mqttc.connect(config['mqtt']['broker_ip'],
		                   int(config['mqtt']['broker_port']),
		                   int(config['mqtt']['keepalive_interval']))

	def run(self):
		# Continue the network loop
		self.mqttc.loop_forever()
	
	def subscribe_to_node(self, node):
		self.mqttc.subscribe(config['mqtt']['base_channel']+'/'+node+'/updates')

	def update_node(self, node, msg):
		self.mqttc.publish(config['mqtt']['base_channel']+'/'+node+'/measurement', msg)

	def force_update_node(self, node, msg):
		self.mqttc.publish(config['mqtt']['base_channel']+'/'+node+'/update/response', msg)

	# Define on_connect event Handler
	def _on_connect(self, mosq, obj, flags, rc):
		# Subscribe to the topic from which we will get the arduinos ids
		pass
		self.mqttc.subscribe(self.config['mqtt']['join_channel'], 0)

	# Define on_subscribe event Handler
	def _on_subscribe(self, mosq, obj, mid, granted_qos):
		print("Subscribed to a topic")
	
	# Define on_message event Handler
	def _on_message(self, mosq, obj, msg):
		if msg.topic == self.config['join_channel']:
		    pass   
		elif re.search("^" + self.config['base_channel'] + "\d+$", msg.topic) != None:
			bin_id = msg.topic.split('/')[1]
			self.nodes_queue.put(bin_id=bin_id, msg=json.loads(msg.payload, 'UTF-8'))
			try:
				self.cond.acquire()
				self.cond.notify()
			finally: 
				self.cond.release()    
		else:
			raise Exception("Unrecognized topic")
