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
		self.mqttc.username_pw_set(self.config['mqtt']['auth_user'],
		                               password=self.config['mqtt']['auth_psw'])


		# Register Event Handlers
		self.mqttc.on_message = self._on_message
		self.mqttc.on_connect = self._on_connect
		self.mqttc.on_subscribe = self._on_subscribe

		# Connect with MQTT Broker
		self.mqttc.connect(self.config['mqtt']['broker_ip'],
		                   int(self.config['mqtt']['broker_port']),
		                   int(self.config['mqtt']['keepalive_interval']))

	def run(self):
		# Continue the network loop
		self.mqttc.loop_forever()

	def subscribe_to_node(self, node):
		self.mqttc.subscribe(self.config['mqtt']['base_channel']+node+'/update')

	def update_node(self, node, msg):
		self.mqttc.publish(self.config['mqtt']['base_channel']+node+'/measurement', json.dumps(msg))

	def force_update_node(self, node, msg):
		self.mqttc.publish(self.config['mqtt']['base_channel']+node+'/update/response', json.dumps(msg))

	# Define on_connect event Handler
	def _on_connect(self, mosq, obj, flags, rc):
		pass

	# Define on_subscribe event Handler
	def _on_subscribe(self, mosq, obj, mid, granted_qos):
		print("Subscribed to a topic")

	# Define on_message event Handler
	def _on_message(self, mosq, obj, msg):
		bin_id = msg.topic.split('/')[1]
		self.nodes_queue.put(bin_id=bin_id, msg=json.loads(msg.payload.decode("UTF-8"), 'UTF-8'), force=True)
		try:
			self.cond.acquire()
			self.cond.notify()
		finally:
			self.cond.release()
