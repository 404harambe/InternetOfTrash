#!/usr/bin/env python2

import sys
import random
import json
import requests
from datetime import datetime

BASE_ENDPOINT = "https://internetoftrash.marcocameriero.net/api"
SIMULATE = True

def post(url, data):
    res = requests.post(BASE_ENDPOINT + url, json = data)
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
                
        # Request new measurements
        proto = NetworkProtocol(42, 0, 2)
        print("Requesting measurements...")
        proto.RequestMeasurement(cb)
        result = cb.result
        
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
