/** 
 *  Commands currently accepted by this program.
 *  {"a":"tgt","tgt":91}  # Set the temperature and write the value to eeprom to persist across power offs.
 *  {"a":"tgt_tmp","tgt":100} # Set the temperature temporarily.  Value will revert to what is stored in eeprom if powered off.
 *  Use this to mimic temperature changes during the day and night for animals, plants, etc.
 *  Compatible with revision 4.5.*
**/
#include <ArduinoJson.h>

#define STATUS_LED_1 14  // RED LED
#define STATUS_LED_2 15  // BLUE LED
#define STATUS_LED_3 13  // BLUE LED

#define MOSFET_LIGHT 5

#define READ_DELAY 200

int persistentLightTargetTemperature = 0;

void setup() {

  Serial.begin(9600);

    pinMode(STATUS_LED_1, OUTPUT);
    pinMode(STATUS_LED_2, OUTPUT);
    pinMode(STATUS_LED_3, OUTPUT);

    digitalWrite(STATUS_LED_1, LOW);
    digitalWrite(STATUS_LED_2, LOW);
    digitalWrite(STATUS_LED_3, LOW);

    pinMode(MOSFET_LIGHT, OUTPUT);
    analogWrite(MOSFET_LIGHT, persistentLightTargetTemperature);
}

String addToJsonString(String json, String add_json) {
  if (json.length() == 0) {
     return "{" + add_json + "}";
  } else {
    json.remove(json.length() - 1);
    return json + "," + add_json + "}";
  }
}

void loop() {

  if (Serial.available() > 0) {
    // read the incoming byte:
    String incomingJson = Serial.readString();
    readJsonAndDoAction(incomingJson);
  }

  analogWrite(MOSFET_LIGHT, persistentLightTargetTemperature);

  String jsonStringToPrint = "";
  String persistentTargetTemperatureString = getJsonPair("tgt", persistentLightTargetTemperature);
  jsonStringToPrint = addToJsonString(jsonStringToPrint, persistentTargetTemperatureString);

  blinkLED(STATUS_LED_1);

  Serial.println(jsonStringToPrint);
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
    if (action.equals("tgt_light")) {
      int newTargetTemperature = doc["tgt"];
      persistentLightTargetTemperature = newTargetTemperature;
    }
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
