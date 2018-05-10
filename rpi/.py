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

BASE_ENDPOINT = "https://internetoftrash.marcocameriero.net/api"
SIMULATE = True
UPDATE_TIME = 3600 # 1 hour

cond = threading.Condition()
lock = threading.Lock()
tasks = SyncOrderedList()


def post(url, data):
    res = requests.post(BASE_ENDPOINT + url, json=data)
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

        mqtt_thread = threading.Thread(target=mqtt_handler.MQTThandler().__init__(tasks, "myconfig.ini"))
        mqtt_thread.start()

        proto = NetworkProtocol(42, 0, 2)
        while True:
            # every x minutes:
            #   send request to arduino
            #   receive answer from arduino
            #   send data to server

            # Wait until receive request for measurement from server or x minutes have passed
            while tasks.empty() or tasks.getFirst()[1] > time():
                cond.wait(timeout=tasks.getFirst()[1])

            # Request new measurements
            dest = tasks.pop()
            print("Requesting measurements...")
            cb = OnMessage()
            proto.RequestMeasurement(dest[0], cb)
            result = cb.result
            tasks.put(dest[0], UPDATE_TIME)

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
