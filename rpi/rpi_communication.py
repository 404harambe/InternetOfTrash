#!/usr/bin/env python2
import paho.mqtt.client as mqtt
from datetime import datetime
import SyncOrderedList
from time import time
import configparser
import mqtt_handler
import bt_handler
import bluetooth
import threading
import requests
import datetime
import random
import json
import sys

config = configparser.ConfigParser()

# Construct reply message
def create_msg(dest, resp):
	if dest[3]!=-1:
		return {'reqId': dest[3], 'status': 'ok' if resp<255 else 'error', 'value': resp, 
	'error': ('Lid not closed' if str(resp)==config['comm']['open_lid'] else '') if str(resp)!=config['comm']['timeout'] else 'Timed out'}
	else:
		return {'timestamp': datetime.datetime.utcnow().isoformat(), 'binId': dest[0], 'value': resp }

if __name__ == "__main__":

	# Config file name is expected as an arg
	if len(sys.argv) < 2:
	    print('Usage: %s <config.ini>' % (sys.argv[0]))
	    sys.exit(1)

	# Read the configuration
	config = configparser.ConfigParser()
	config.readfp(open(sys.argv[1]))


	cond = threading.Condition()
	lock = threading.Lock()
	tasks = SyncOrderedList.SyncOrderedList()


	# Launch the MQTT thread listening for updates
	mqtt_handler = mqtt_handler.MQTThandler(tasks, cond, config)
	threading._start_new_thread(mqtt_handler.run,())

	# Launch the thread scanning the network for new devices
	bt_handler = bt_handler.BThandler(mqtt_handler, tasks, config['comm'], cond)
	threading._start_new_thread(bt_handler.run,())

	result = {}

	while True:
        # every x minutes:
        #   send request to arduino
        #   receive answer from arduino
        #   send data to server

        # Wait for a new task
		while(tasks.wait_for_task()>time()):
			cond.acquire()
			cond.wait(timeout=tasks.wait_for_task()-time())

        # Request new measurements
		dest = tasks.pop()
		response = bt_handler.request_measurement(dest[0])
		msg = create_msg(dest, response)

		if dest[2]==True: # Instant update
			print('Insta update:',msg)
			mqtt_handler.force_update_node(dest[0], msg)
			tasks.put(dest[0], time=float(config['rpi']['update_interval']))
		else: # Regular update
			if response > 0:
				mqtt_handler.update_node(dest[0], msg)
				tasks.put(dest[0], time=float(config['rpi']['update_interval']))
			else:
				tasks.put(dest[0], time=float(config['rpi']['retry_interval']))


