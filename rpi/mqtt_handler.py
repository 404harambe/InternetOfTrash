import paho.mqtt.client as mqtt
import re
import configparser


class MQTThandler:
    # List of arduinos to handle

    def __init__(self, nodes_queue, config):
        self.nodes_queue = nodes_queue

        self.config = configparser.ConfigParser()
        self.config.read('myconfig.ini')

        # Initiate MQTT Client
        self.mqttc = mqtt.Client()
        self.mqttc.username_pw_set(config['MQTT']['AUTH']['USER'],
                                   password=config['MQTT']['AUTH']['PASSWORD'])

        # Register Event Handlers
        self.mqttc.on_message = self.on_message
        self.mqttc.on_connect = self.on_connect
        self.mqttc.on_subscribe = self.on_subscribe

        # Connect with MQTT Broker
        self.mqttc.connect(config['MQTT']['BROKER_IP'],
                           config['MQTT']['BROKER_PORT'],
                           config['MQTT']['KEEP_ALIVE_INTERVAL'])

        # Continue the network loop
        self.mqttc.loop_forever()


    # Define on_connect event Handler
    def on_connect(self, mosq, obj, flags, rc):
        # Subscribe to the topic from which we will get the arduinos ids
        self.mqttc.subscribe(self.config['MQTT']['JOIN_CHANNEL'], 0)


    # Define on_subscribe event Handler
    def on_subscribe(self, mosq, obj, mid, granted_qos):
        # TODO topic name
        print("Subscribed to %s" % ("#TODO topic name"))


    # Define on_message event Handler
    def on_message(self, mosq, obj, msg):
        if msg.topic == self.config['MQTT']['JOIN_CHANNEL']:
            self.nodes_queue.put(msg)
            print("Now listening for arduino %s:" % (msg))
        elif re.search("^" + self.config['MQTT']['BASE_CHANNEL'] + "/\d+$", msg.topic) != None:
            self.nodes_queue.put(msg)
        else:
            raise Exception("Unrecognized topic")
