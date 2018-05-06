#!/usr/bin/env python2

import sys

# Load the NetworkProtocol module from the `out` directory
sys.path.insert(0, "./out")
from NetworkProtocol import wiringPiSetup, NetworkProtocol, MessageCallback

# Initialize WiringPi
if wiringPiSetup() < 0:
    print("Failed initializing WiringPI.\n")
    sys.exit(1)

# Accumulator of all the results
class OnMessage(MessageCallback):
    def __init__(self):
        self.result = {}        
    def Run(self, msg):
        self.result[msg.Source()] = msg.Measurement()

proto = NetworkProtocol(42, 0, 2)
print("Requesting measurements...")
cb = OnMessage()
proto.RequestMeasurement(cb)

print("Result:")
print(cb.result)
