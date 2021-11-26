

#include <Arduino.h>
#include <OpenTherm.h>

#include <Time.h>
#include <TimeLib.h>

// Wemos D1 mini
const int inPin = 3;
const int outPin = 5;

OpenTherm ot(inPin, outPin);

void ICACHE_RAM_ATTR handleInterrupt() {
    ot.handleInterrupt();
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Start");

    ot.begin(handleInterrupt);

    Serial.begin(115200);
}



void try_id(int id) {
  
  unsigned long msg = ot.buildRequest(
    OpenThermRequestType::READ,
    (OpenThermMessageID)id,
    0
  );
  unsigned long response = ot.sendRequest(msg);
  if (ot.isValidResponse(response)){
    unsigned int data = ot.getUInt(response);
    Serial.println("========");
    Serial.print("Trying ID: ");
    Serial.println(id);
    Serial.println("Response:");
    Serial.println(data);
    Serial.println(data, BIN);
  } else{
    Serial.println("INVALID");
  }
  
}

void loop()
{
    for (int i = 0; i < 254; i++) {
      try_id(i);
      delay(10);
    }

    delay(5000);
}
