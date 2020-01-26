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

#define MOSFET_HEAT 5
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
    
    String action = doc["a"];
    if (action.equals("tgt")) {
      int newTargetTemperature = doc["tgt"];
      setAndReadTargetTemperature(newTargetTemperature, true);
    }
    if (action.equals("tgt_tmp")) {
      int newTargetTemperature = doc["tgt"];
      if (newTargetTemperature < MAX_TARGET_TEMP) {
         persistantTargetTemperature = newTargetTemperature;  
      }
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

  setAndReadTargetTemperature(TARGET_TEMP, false);

  int storedTargetTemperature = EEPROM.read(TARGET_TEMP_EEPROM_ADDRESS);
  if (storedTargetTemperature == 255) {// The EEPROM hasn't been set.
     EEPROM.write(TARGET_TEMP_EEPROM_ADDRESS, TARGET_TEMP);
  } else {
    if (storedTargetTemperature < 120) { //   Nothing I do needs to be above this temperature.
      persistantTargetTemperature = storedTargetTemperature;
    }
  }
  
  discoverOneWireDevices();
  sensors.begin();

    for( int i = 0; i < oneWireMaxIndex; i++) {
      sensors.setResolution(oneWireAddresses[i], 8);
    }

    pinMode(STATUS_LED_1, OUTPUT);
    pinMode(STATUS_LED_2, OUTPUT);
    pinMode(STATUS_LED_3, OUTPUT);

    digitalWrite(STATUS_LED_1, LOW);
    digitalWrite(STATUS_LED_2, LOW);
    digitalWrite(STATUS_LED_3, LOW);

    // The heater mosfet must be written low on startup.
    // Otherwise a power failure while it's above temperature will leave
    // heater element on and overheat the device.
    pinMode(MOSFET_HEAT, OUTPUT);
    analogWrite(MOSFET_HEAT, 0);


    // The fan mosfet should be written high on startup.
    pinMode(MOSFET_FAN_1, OUTPUT);
    analogWrite(MOSFET_FAN_1, 255);

    pinMode(MOSFET_FAN_2, OUTPUT);
    analogWrite(MOSFET_FAN_2, 255);

    pinMode(INTERRUPT_PIN_FAN_1, INPUT);
    digitalWrite(INTERRUPT_PIN_FAN_1, HIGH);

    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN_FAN_1), readRpm, RISING);

    blinkLED(STATUS_LED_1);
    blinkLED(STATUS_LED_2);
    blinkLED(STATUS_LED_3);

    delay(500); // Let the fan spin up.
    checkFanStatus(); // Preload the fan status so that it doesn't show an error on startup.
    
    blinkLED(STATUS_LED_1);
    blinkLED(STATUS_LED_2);
    blinkLED(STATUS_LED_3);
    
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

  sensors.requestTemperatures();
  double combinedTemperatures = 0.0;

  String jsonStringToPrint = "";
  String persistantTargetTemperatureString = getJsonPair("tgt", persistantTargetTemperature);
  jsonStringToPrint = addToJsonString(jsonStringToPrint, persistantTargetTemperatureString);
  
  for(int i = 0; i < oneWireMaxIndex; i++) {
    double temperature = readTemp(oneWireAddresses[i]);
    combinedTemperatures += temperature;

    String name = "t" + String(i);
    String probeTempString = getJsonPair(name, temperature);
    jsonStringToPrint = addToJsonString(jsonStringToPrint, probeTempString);
  }

  double average = combinedTemperatures / oneWireMaxIndex;
  String probeAverageString = getJsonPair("avg", average);
  jsonStringToPrint = addToJsonString(jsonStringToPrint, probeAverageString);

  if (fanIsRunning) {

    if (average > persistantTargetTemperature) {

      if (mosfetOn_heater) {
        rampDownMosfet(MOSFET_HEAT);
        rampDownMosfet(MOSFET_FAN_2);
        digitalWrite(STATUS_LED_3, LOW);
        mosfetOn_heater = false;
        mosfetDutyCycleOnCount = 0;
        mosfetDutyCycleOnCount = 0;
      }

    } else if (average <= persistantTargetTemperature) {
      if (!mosfetOn_heater) {
        rampUpMosfet(MOSFET_FAN_1);
        rampUpMosfet(MOSFET_HEAT);
        digitalWrite(STATUS_LED_3, HIGH);
        mosfetOn_heater = true;
      } 
      if (mosfetOn_heater) {
      
          if (mosfetDutyCycleOnCount < DUTY_CYCLE_ON) {
              mosfetDutyCycleOnCount += 1; 
          } else {
              rampDownMosfet(MOSFET_HEAT);
          }
         
         if (mosfetDutyCycleOnCount >= DUTY_CYCLE_ON) {
          if (mosfetDutyCycleOffCount < DUTY_CYCLE_OFF) {
             mosfetDutyCycleOffCount += 1;
          } else {
             rampUpMosfet(MOSFET_HEAT);
             mosfetDutyCycleOnCount = 0;
             mosfetDutyCycleOffCount = 0;
          }
         }
      }
    }
  } else {
    //Serial.println("Status: Fan is not running. Forcing heat off.");
    String errorPair = getJsonPair("err","fan");
    jsonStringToPrint = addToJsonString(jsonStringToPrint, errorPair);
    analogWrite(MOSFET_HEAT, 0);
    mosfetOn_heater = false;
    blinkLED(STATUS_LED_3);
    blinkLED(STATUS_LED_3);
    blinkLED(STATUS_LED_3);
  }

  String mosfetStatusPair = getJsonPair("ht", mosfetOn_heater ? 1 : 0 );
  jsonStringToPrint = addToJsonString(jsonStringToPrint, mosfetStatusPair);

  String fanPair = getJsonPair("fan", fanInteruptCount);
  jsonStringToPrint = addToJsonString(jsonStringToPrint, fanPair);

  checkFanStatus();

  // Indicate a loop with a blink.
  blinkLED(STATUS_LED_1);

  Serial.println(jsonStringToPrint);

  delay(800);
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
