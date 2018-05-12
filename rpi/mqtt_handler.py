import paho.mqtt.client as mqtt
import re
import configparser

'''
MQTT module for the sink communication with the remote server. Upon connecting, the sink subscribes to 
the JOIN channel on which the server will notify the presence of new nodes to handle.
When a new node joins the local network, the sink will also subscribe to the node's specific channel on which
the server might request an instant update of the node's status.
'''

class MQTThandler:

    def __init__(self, nodes_queue, cond, config):
        self.nodes_queue = nodes_queue
        self.cond = cond
        self.config = config

        # Initiate MQTT Client
        self.mqttc = mqtt.Client()
        self.mqttc.username_pw_set(config['AUTH_USER'],
                                   password=config['AUTH_PSW'])

        # Register Event Handlers
        self.mqttc.on_message = self.on_message
        self.mqttc.on_connect = self.on_connect
        self.mqttc.on_subscribe = self.on_subscribe

    def run(self):
        # Connect with MQTT Broker
        self.mqttc.connect(self.config['BROKER_IP'],
                           int(self.config['BROKER_PORT']),
                           int(self.config['KEEPALIVE_INTERVAL']))

        # Continue the network loop
        self.mqttc.loop_forever()


    # Define on_connect event Handler
    def on_connect(self, mosq, obj, flags, rc):
        # Subscribe to the topic from which we will get the arduinos ids
        self.mqttc.subscribe(self.config['JOIN_CHANNEL'], 0)


    # Define on_subscribe event Handler
    def on_subscribe(self, mosq, obj, mid, granted_qos):
        print("Subscribed to a topic")
	
    # Define on_message event Handler
    def on_message(self, mosq, obj, msg):
        if msg.topic == self.config['JOIN_CHANNEL']:
            #Subscribe to the new node's channel to receive its updates
            self.mqttc.subscribe(self.config['BASE_CHANNEL']+msg.payload.decode('UTF-8'))
            self.nodes_queue.put(msg.payload.decode('UTF-8'))
            try:
                self.cond.acquire()
                self.cond.notify()
            finally: 
                self.cond.release()   
        elif re.search("^" + self.config['BASE_CHANNEL'] + "\d+$", msg.topic) != None:
            self.nodes_queue.put(msg.payload.decode('UTF-8'))
            try:
                self.cond.acquire()
                self.cond.notify()
            finally: 
                self.cond.release()    
        else:
            raise Exception("Unrecognized topic")
