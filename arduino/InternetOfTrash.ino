
#include <NewPing.h>
#include <SoftwareSerial.h>
#include <math.h>
#include "config.h"

SoftwareSerial bt(PIN_BT_SERIAL_RX, PIN_BT_SERIAL_TX);
NewPing sonar(PIN_SONIC_TRIGGER, PIN_SONIC_ECHO);

bool perform_measurement(unsigned long* val) {

    // Wait for the lid to close
    Serial.print("Waiting for lid closure...\n");
    bool timeout = true;
    unsigned long start = millis();
    while (millis() - start <= LID_CLOSED_TIMEOUT) {
        if (digitalRead(PIN_SWITCH) == HIGH) {
            timeout = false;
            break;
        }
    }
    
    if (timeout) {
        Serial.print("Timeout.\n");
        return false;
    } else {
    
        // Perform 10 measurements and try to get rid of the noise
        unsigned long measurements[10];
        float sum = 0, mean = 0, std = 0, c=0;
        int i = 0;
        for (i = 0; i < 10; i++){
            measurements[i] = sonar.ping_cm();
            Serial.print(measurements[i]);
            Serial.print(" <-> ");
            sum += measurements[i];
        }
        mean = sum/10;
        for (i = 0; i < 10; i++)
            std += pow(measurements[i]-mean, 2);
        std = sqrt(std/10);

        sum = 0;
        
        for (i = 0; i < 10; i++)
            if(measurements[i] <= mean+std && measurements[i] >= mean-std){
                sum += measurements[i];
                c += 1;
            }
        sum /= c;

        Serial.print("Measured value: ");
        Serial.print(sum);
        Serial.print("\n");
        if (sum>255)
            sum = 255;
        *val = sum;
        return true;
        
    }
}

void setup() {
    Serial.begin(9600);
    bt.begin(9600);
    Serial.print("Initialized.\n");
}

void loop() {
    if (bt.available() > 0) {
        int command = bt.read();

        unsigned long m = 0;
        switch (command) {

            case '0':
                Serial.print("Name request. Sending " THIS_ID ".\n");
                bt.print(THIS_ID);
                break;

            case '1':
                Serial.print("Measurement request.\n");
                if (perform_measurement(&m)) {
                    bt.write((unsigned char) m);
                } else {
                    bt.write((unsigned char) -1);
                }
                break;

            default:
                // Ignore any invalid command
                Serial.print("Invalid command.\n");
                break;

        }
    }
}
