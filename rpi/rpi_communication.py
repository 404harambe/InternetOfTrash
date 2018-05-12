#!/usr/bin/env python2

import SyncOrderedList
import threading
from time import time
import configparser
import sys
import random
import json
import requests
from datetime import datetime
import mqtt_handler

# Config file name is expected as an arg
if len(sys.argv) < 2:
    print('Usage: %s <config.ini>' % (sys.argv[0]))
    sys.exit(1)

config = configparser.ConfigParser()
config.readfp(open(sys.argv[1]))

SIMULATE = True

cond = threading.Condition()
lock = threading.Lock()
tasks = SyncOrderedList.SyncOrderedList()


def post(url, data):
    res = requests.post(config['rpi']['remote_endpoint'] + url, json=data)
    if res.status_code != 200:
        raise Exception("HTTP Error " + res.status_code)
    resdata = res.json()
    if resdata["status"] == "ok":
        return resdata.get("contents")
    else:
        raise Exception("Error returned from server: " + resdata["error"])


if __name__ == "__main__":

    result = {}

    if not SIMULATE:

        # Load the NetworkProtocol module from the `out` directory
        sys.path.insert(0, "./out")
        from NetworkProtocol import wiringPiSetup, NetworkProtocol, MessageCallback


        # Accumulator of all the results
        class OnMessage(MessageCallback):
            def __init__(self):
                self.result = {}

            def Run(self, msg):
                self.result[msg.Source()] = {
                    "timestamp": datetime.utcnow().isoformat(),
                    "binId": "{0:024d}".format(msg.Source()),
                    "value": msg.Measurement()
                }

        # Initialize WiringPi
        if wiringPiSetup() < 0:
            print("Failed initializing WiringPI.\n")
            sys.exit(1)

        mqtt_handler = mqtt_handler.MQTThandler(tasks, cond, config['mqtt'])
        threading._start_new_thread(mqtt_handler.run,())

        proto = NetworkProtocol(42, 0, 2)
        while True:
            # every x minutes:
            #   send request to arduino
            #   receive answer from arduino
            #   send data to server

            # Wait for a new task
            while (tasks.wait_for_task()>0):
                cond.acquire()
                cond.wait(timeout=tasks.wait_for_task())

            # Request new measurements
            dest = tasks.pop()
            print("Requesting measurements...")
            cb = OnMessage()
            proto.RequestMeasurement(dest[0], cb)
            result = cb.result
            tasks.put(dest[0], config['rpi']['update_interval'])

    # Add some fake data if we are simulating
    else:
        result[1] = {
            "timestamp": datetime.utcnow().isoformat(),
            "binId": "1".zfill(24),
            "value": random.randint(5, 100)
        }
        result[2] = {
            "timestamp": datetime.utcnow().isoformat(),
            "binId": "2".zfill(24),
            "value": random.randint(5, 100)
        }

    print("Measurements:")
    print(result)

    # Send the updates to the server
    if len(result) > 0:
        post("/bulkmeasurements", result.values())
        print("Success!")
    else:
        print("No data to send.")
