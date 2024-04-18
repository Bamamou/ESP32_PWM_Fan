#include <Arduino.h>

// Fan_Temp_Control.ino ##############################################
/* This code was written to control the temperature inside an old big and uggly Desktop 
by measuring the temperature with a DHT22 Temperature probe and
outputting a PWM signal with and ESP32 to control a 4-Pin fan.
Unlike the cheap control boards from taobao or Pinduoduo this code switches
the fan off when the temperature is low enough. */

/*

The following constants should be changed according to the use case:

constant (default value) - description

tempLow (35) - Below this temperature (minus half hysteresis) the fan shuts off.
               It shuts on again at this temperature plus half hysteresis

tempHigh (50) - At and above this temperature the fan is at maximum speed

hyteresis (5) - Hysteresis to prevent frequent on/off switching at the threshold

minDuty (10) - Minimum fan speed to prevent stalling
maxDuty (255) - Maximum fan speed to limit noise

 */



#include <Wire.h>
#include "DHTesp.h" // Click here to get the library: http://librarymanager/All#DHTesp


DHTesp dht;             // instantiate a DHT object

// PWM output pin
const byte DHT_Pin = 5;

// PWM_Fan output pin
const byte PWM_Fan = 4;
const int DELAY_MS = 4;  // delay between fade increments

// how frequently the main loop runs
const int tempSetInterval = 5000;
const int PWM_CHANNEL = 0;    // ESP32 has 16 channels which can generate 16 independent waveforms
const int PWM_FREQ = 500;     // Recall that Arduino Uno is ~490 Hz. Official ESP32 example uses 5,000Hz
const int PWM_RESOLUTION = 8; // We'll use same resolution as Uno (8 bits, 0-255) but ESP32 can go up to 16 bits 

// temperatur settings
const float tempLow = 26;
const float tempHigh = 30.3;
const float hyteresis = 5;
const int minDuty = 10;
// The max duty cycle value based on PWM resolution (will be 255 if resolution is 8 bits)
const int maxDuty = (int)(pow(2, PWM_RESOLUTION) - 1); 

// state on/off of Fan
bool fanState = HIGH;

// current duty cycle
byte duty = 100;

// new duty cycle
byte newDuty = 100;


void setup() {
	// start serial port 
	Serial.begin(115200); 
	
	// Start up the temperature library 
  dht.setup(DHT_Pin, DHTesp::DHT11);   // If you are using the DHT22, you just need to change the value 11 to 22
	delay(dht.getMinimumSamplingPeriod());
  Serial.println(dht.getStatusString());
  
  // Sets up a channel (0-15), a PWM duty cycle frequency, and a PWM resolution (1 - 16 bits) 
  // ledcSetup(uint8_t channel, double freq, uint8_t resolution_bits);
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);

  // ledcAttachPin(uint8_t pin, uint8_t channel);
  ledcAttachPin(PWM_Fan, PWM_CHANNEL);

}


// setting PWM ############################################
void setPwmDuty() {
	if (duty == 0) {
		fanState = LOW;
	} else if (duty > 0) {
		fanState = HIGH;
	}
	ledcWrite(PWM_CHANNEL, duty);
}


// calculate new PWM ######################################
void tempToPwmDuty() {

  float temp = dht.getTemperature();       // To store the values of tempreature
  float humidity = dht.getHumidity();            // To store the values of Humidity

	Serial.print(temp);
	Serial.print("°C, ");

	if (temp < tempLow) {
		// distinguish two cases to consider hyteresis
		if (fanState == HIGH) {
			if (temp < tempLow - (hyteresis / 2) ) {
				// fan is on, temp below threshold minus hysteresis -> switch off
				Serial.print("a, ");
				newDuty = 0;
			} else {
				// fan is on, temp not below threshold minus hysteresis -> keep minimum speed
				Serial.print("b, ");
				newDuty = minDuty;
			}
		} else if (fanState == LOW) {
			// fan is off, temp below threshold -> keep off
			Serial.print("c, ");
			newDuty = 0;
		}

	} else if (temp < tempHigh) {
		// distinguish two cases to consider hyteresis
		if (fanState == HIGH) {
			// fan is on, temp above threshold > control fan speed
			Serial.print("d, ");
			newDuty = map(temp, tempLow, tempHigh, minDuty, maxDuty);
		} else if (fanState == LOW) {
			if (temp > tempLow + (hyteresis / 2) ) {
				// fan is off, temp above threshold plus hysteresis -> switch on
				Serial.print("e, ");
				newDuty = minDuty;
			} else {
				// fan is on, temp not above threshold plus hysteresis -> keep off
				Serial.print("f, ");
				newDuty = 0;
			}
		}
	} else if (temp >= tempHigh) {
		// fan is on, temp above maximum temperature -> maximum speed
		Serial.print("g, ");
		newDuty = maxDuty;
	} else {
		// any other temperature -> maximum speed (this case should never occur)
		Serial.print("h, ");
		newDuty = maxDuty;
	}

 	//set new duty
 	duty = newDuty;

	Serial.print(duty);
  	Serial.print("%, ");

 	if (fanState==0) {Serial.println("OFF");} else {Serial.println("ON");}
	setPwmDuty();
}


// main loop ##############################################
void loop() {
	// measure temperature, calculate Duty cycle, set PWM
// fade up PWM on given channel
 // measure temperature, calculate Duty cycle, set PWM
	tempToPwmDuty();
  delay(DELAY_MS);
}
