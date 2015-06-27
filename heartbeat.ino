/*
>> Pulse Sensor Amped 1.1 <<
This code is for Pulse Sensor Amped by Joel Murphy and Yury Gitman
    www.pulsesensor.com
    >>> Pulse Sensor purple wire goes to Analog Pin 0 <<<
Pulse Sensor sample aquisition and processing happens in the background via Timer 2 interrupt. 2mS sample rate.
PWM on pins 3 and 11 will not work when using this code, because we are using Timer 2!
The following variables are automatically updated:
Signal :    int that holds the analog signal data straight from the sensor. updated every 2mS.
IBI  :      int that holds the time interval between beats. 2mS resolution.
BPM  :      int that holds the heart rate value, derived every beat, from averaging previous 10 IBI values.
QS  :       boolean that is made true whenever Pulse is found and BPM is updated. User must reset.
Pulse :     boolean that is true when a heartbeat is sensed then false in time with pin13 LED going out.

This code is designed with output serial data to Processing sketch "PulseSensorAmped_Processing-xx"
The Processing sketch is a simple data visualizer.
All the work to find the heartbeat and determine the heartrate happens in the code below.
Pin 13 LED will blink with heartbeat.
If you want to use pin 13 for something else, adjust the interrupt handler
It will also fade an LED on pin fadePin with every beat. Put an LED and series resistor from fadePin to GND.
Check here for detailed code walkthrough:
http://pulsesensor.myshopify.com/pages/pulse-sensor-amped-arduino-v1dot1

Code Version 02 by Joel Murphy & Yury Gitman  Fall 2012
This update changes the HRV variable name to IBI, which stands for Inter-Beat Interval, for clarity.
Switched the interrupt to Timer2.  500Hz sample rate, 2mS resolution IBI value.
Fade LED pin moved to pin 5 (use of Timer2 disables PWM on pins 3 & 11).
Tidied up inefficiencies since the last version.
*/

#include <Adafruit_NeoPixel.h>

#define TIMER_TO_USE_FOR_MILLIS 0   // Change millis() to use Timer0 instead of Timer1
#define NEOPIXELS_PIN 0             // NeoPixels data line connected to D0
#define BLINK_PIN 1                 // LED on D1
#define PULSE_PIN 1                 // Pulse Sensor purple wire connected to D2 (A1) - This was the biggest hiccup!
// When selecting a pin for INPUT a

//  VARIABLES
// these variables are volatile because they are used during the interrupt service routine!
volatile int BPM;                   // used to hold the pulse rate
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // holds the time between beats, the Inter-Beat Interval
volatile boolean Pulse = false;     // true when pulse wave is high, false when it's low
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.

Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, NEOPIXELS_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
    pinMode(BLINK_PIN, OUTPUT);        // pin that will blink to your heartbeat!
    //Serial.begin(115200);             // we agree to talk fast! - shut up, GEMMA can't talk over Serial!
    // UN-COMMENT THE NEXT LINE IF YOU ARE POWERING The Pulse Sensor AT LOW VOLTAGE,
    // AND APPLY THAT VOLTAGE TO THE A-REF PIN
    //analogReference(EXTERNAL); // we don't need this anymore since GEMMA runs at low voltage anyway
    interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS
}

void loop() {
    if (QS == true) {                      // Quantified Self flag is true when arduino finds a heartbeat
        showPulseFadeStop();
        QS = false;                        // reset the Quantified Self flag for next time
    }
}

void showPulseStart() {
    for (uint8_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(255, 0, 0));
    }
    strip.show();
}

void showPulseFadeStop() {
    for (int j = 255; j >= 0; j--) {
        for (int i = 0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, strip.Color(j, 0, 0));
        }
        strip.show();
        delayMicroseconds(500);
    }
}

void showPulseStop() {
    for (uint8_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, 0);
    }
    strip.show();
}

