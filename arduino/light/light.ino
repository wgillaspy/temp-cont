/** 
 *  Commands currently accepted by this program.
 *  {"a":"tgt","tgt":91}  # Set the temperature and write the value to eeprom to persist across power offs.
 *  {"a":"tgt_tmp","tgt":100} # Set the temperature temporarily.  Value will revert to what is stored in eeprom if powered off.
 *  Use this to mimic temperature changes during the day and night for animals, plants, etc.
 *  Compatible with revision 4.5.*
**/
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>

#define ONE_WIRE_BUS 4

#define STATUS_LED_1 14  // RED LED
#define STATUS_LED_2 15  // BLUE LED
#define STATUS_LED_3 13  // BLUE LED

#define MOSFET_LIGHT 5
#define MOSFET_FAN_1 6
#define MOSFET_FAN_2 9

#define TARGET_TEMP_EEPROM_ADDRESS 0
#define TARGET_TEMP 85 // Target temperature cannot be a decimal at this time.
#define MAX_TARGET_TEMP 120 // Don't let the temperature be set above this amount.

#define READ_DELAY 200

#define INTERRUPT_PIN_FAN_1 2
#define INTERRUPT_PIN_FAN_2 3

#define DUTY_CYCLE_ON 4
#define DUTY_CYCLE_OFF 8

// Hold up to 5 temperature probes.  Add more if you need them.
byte oneWireAddresses[5][8];
// The index that we can count to.  Can't push arrays around these parts.
int  oneWireMaxIndex = 0;

boolean mosfetOn_heater = false;
int mosfetDutyCycleOnCount = 0;
int mosfetDutyCycleOffCount = 0;


boolean mosfetOn_fan = false;

boolean fanIsRunning = false;

int mosfetValue_heater = 0;
int mosfetValue_fan = 0;

int persistantTargetTemperature = TARGET_TEMP;

// Each DS18B20 has it's own one wire address.  Visit http://www.hacktronics.com/Tutorials/arduino-1-wire-address-finder.html
// Make sure you have your 4.7K resistor on data and +5V.
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

volatile int fanInteruptCount = 0;

void readRpm() {
  fanInteruptCount++;
}

void blinkLED(int PIN) {
  digitalWrite(PIN, HIGH);
  delay(READ_DELAY);
  digitalWrite(PIN, LOW);
}

void readJsonAndDoAction(String inputJson) {
    StaticJsonDocument<400> doc;
    DeserializationError error = deserializeJson(doc, inputJson);
    // Test if parsing succeeds.
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }
}

void setAndReadTargetTemperature(int temperature, boolean overwrite) {
  
  int storedTargetTemperature = EEPROM.read(TARGET_TEMP_EEPROM_ADDRESS);
  if (storedTargetTemperature == 255 || overwrite) { // The EEPROM hasn't been set.
    if (temperature > MAX_TARGET_TEMP) { // Don't allow the target temperature to exceed the maximum temperature.
      temperature = MAX_TARGET_TEMP;
    }
    EEPROM.write(TARGET_TEMP_EEPROM_ADDRESS, temperature);
    persistantTargetTemperature = temperature;
  } else {
    if (storedTargetTemperature < MAX_TARGET_TEMP) { //   Nothing I do needs to be above this temperature.
      persistantTargetTemperature = storedTargetTemperature;
    }
  }
}

void setup() {

  Serial.begin(9600);

    pinMode(STATUS_LED_1, OUTPUT);
    pinMode(STATUS_LED_2, OUTPUT);
    pinMode(STATUS_LED_3, OUTPUT);

    digitalWrite(STATUS_LED_1, LOW);
    digitalWrite(STATUS_LED_2, LOW);
    digitalWrite(STATUS_LED_3, LOW);

    pinMode(MOSFET_LIGHT, OUTPUT);
    analogWrite(MOSFET_LIGHT, 255);
}

String addToJsonString(String json, String add_json) {
  if (json.length() == 0) {
     return "{" + add_json + "}";
  } else {
    json.remove(json.length() - 1);
    return json + "," + add_json + "}";
  }
}

void checkFanStatus() {
   // Make sure the fan is running.  If it's not, turn off the heat.
  if (fanInteruptCount > 8) {
    blinkLED(STATUS_LED_2);
    fanIsRunning = true;
  } else {
    digitalWrite(STATUS_LED_2, LOW);
    fanIsRunning = false;
  }
 
  fanInteruptCount = 0;
}

void loop() {

  if (Serial.available() > 0) {
    // read the incoming byte:
    String incomingJson = Serial.readString();
    readJsonAndDoAction(incomingJson);
  }
  delay(800);
  digitalWrite(STATUS_LED_1, LOW);
  delay(800);
  digitalWrite(STATUS_LED_1, HIGH);
}

void rampUpMosfet(int MOSFET_PIN) {
//  for (int i = 0; i <= 255; i += 5) {
//    analogWrite(MOSFET_PIN, i);
//    delay(10);
//  }
  analogWrite(MOSFET_PIN, 255);
}

void rampDownMosfet(int MOSFET_PIN) {
//  for (int i = 255; i >= 0; i -= 5) {
//    analogWrite(MOSFET_PIN, i);
//    delay(10);
//  }
  analogWrite(MOSFET_PIN, 0);
}

void discoverOneWireDevices(void) {
  byte i;
  byte addr[8];

  while(oneWire.search(addr)) {

    for( i = 0; i < 8; i++) {
      oneWireAddresses[oneWireMaxIndex][i] = addr[i];
    }

    oneWireMaxIndex++;

    if ( OneWire::crc8( addr, 7) != addr[7]) {
      // CRC is not valid.
      return;
    }
  }

  oneWire.reset_search();

  return;
}

String doubleToString(double input,int decimalPlaces) {
  if(decimalPlaces!=0) {
    String string = String((int)(input*pow(10,decimalPlaces)));

    if(abs(input)<1) {
      if(input>0) {
       string = "0"+string;
      } else if(input<0) {
       string = string.substring(0,1)+"0"+string.substring(1);
      }
    }
    return string.substring(0,string.length()-decimalPlaces)+"."+string.substring(string.length()-decimalPlaces);
  } else {
    return String((int)input);
  }
}

String getJsonPair(String name, String value) {
  String json = "\"" + name + "\":\"";
  json.concat(value);
  json.concat("\"");
  return json;
}


String getJsonPair(String name, float value) {
  String json = "\"" + name + "\":\"";
  json.concat(value);
  json.concat("\"");
  return json;
}

String getJsonPair(String name, double value) {
  String json = "\"" + name + "\":\"";
  json.concat(value);
  json.concat("\"");
  return json;
}

String getJsonPair(String name, int value) {
  String json = "\"" + name + "\":\"";
  json.concat(value);
  json.concat("\"");
  return json;
}


float readTemp(DeviceAddress deviceAddress) {
  float tempC = (float)sensors.getTempF(deviceAddress);
  if (tempC == -127.00) {
    //Serial.print("Error getting temperature");
  } 
  return tempC;
}
