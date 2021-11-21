

#include <Arduino.h>
#include <OpenTherm.h>
#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <Time.h>
#include <TimeLib.h>
#include <AsyncDelay.h>


// WiFi credentials
const char ssid[] = "";
const char pass[] = "";

//// The MQTT broker information
const char brokeraddr[] = "";
const int brokerport = ;

// Device info
// id used for mqtt and hostname
const char deviceid[] = "";
const char mqttuser[] = "";
const char mqttpass[] = "";
const char mqttch_speedcommand[] = "home/wtw/speed/set";
const char mqttch_speed[] = "home/wtw/speed";
const char mqttch_exhaustintemp[] = "home/wtw/exhaust/in/temp";
const char mqttch_supplyintemp[] = "home/wtw/supply/in/temp";

const char mqttch_ventmode[] = "home/wtw/ventmode";
const char mqttch_diagnostic[] = "home/wtw/diagnostic";
const char mqttch_fault[] = "home/wtw/fault";

const char mqttch_bypasstatus[] = "home/wtw/bypass/status";
const char mqttch_automaticstatus[] = "home/wtw/bypass/automaticstatus";

// WiFiClientSecure net;
WiFiClient net;
MQTTClient client;

unsigned long PUBLISH_INTERVAL = 5000;
unsigned long WIFI_CONNECT_INTERVAL = 1000;
AsyncDelay publish_delay;
AsyncDelay wifi_delay;
AsyncDelay motor_time;

// State management
int desiredNomVent = 25;

/* Single Shot trying to connect to MQTT, only single shot to prevent blocking
 */
void connect() {
    Serial.print("Checking wifi...");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("\nconnecting...");
        // deviceid, user, pass
        // Try to connect once
        if (client.connect(deviceid, mqttuser, mqttpass)){
            Serial.println("\nconnected!");
            client.subscribe(mqttch_speedcommand);
        }
    }
}

void messageReceived(String &topic, String &payload) {

    if (topic.equals(mqttch_speedcommand)) {
        Serial.println("incoming: " + topic + " - " + payload);
        int nomVentVal = payload.toInt();
        if (nomVentVal > 100) {
          nomVentVal = 100;
        } else if (nomVentVal < 0) {
          nomVentVal = 0;
        }
        desiredNomVent = nomVentVal;
        Serial.print("Setting desired nom Vent Val:");
        Serial.println(desiredNomVent);

    } else {
      Serial.println("Unknown topic: " + topic + " - " + payload);
    }
}


//Arduino Uno
//#define HVAC_IN 3
//#define HVAC_OUT 5

// Wemos D1 mini
const int inPin = D5;
const int outPin = D4;


//const int inPin = 3; //3//for Arduino, 4 for ESP8266
//const int outPin = 5; //5//for Arduino, 5 for ESP8266
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
    WiFi.mode(WIFI_STA);
    WiFi.hostname(deviceid);
    WiFi.begin(ssid, pass);
    wifi_delay.start(WIFI_CONNECT_INTERVAL, AsyncDelay::MILLIS);

    client.begin(brokeraddr, brokerport, net);
    client.onMessage(messageReceived);
    connect();
}


void get_temperatures() {
  Serial.println("Getting temperatures");
  float temperature1 = ot.getVentSupplyInTemperature();
  float temperature2 = ot.getVentSupplyOutTemperature();
  float temperature3 = ot.getVentExhaustInTemperature();
  float temperature4 = ot.getVentExhaustOutTemperature();
  Serial.print("T1:");
  Serial.println(temperature1);
  Serial.print("T2:");
  Serial.println(temperature2);
  Serial.print("T3:");
  Serial.println(temperature3);
  Serial.print("T4:");
  Serial.println(temperature4);
}

void get_pub_temps() {
  Serial.println("Publishing Temperatures Exhaust/Supply in");
  float temperature1 = ot.getVentSupplyInTemperature();
  String tempstr1 = String(temperature1);
  Serial.println("Supply In: " + tempstr1 + "C");
  client.publish(mqttch_supplyintemp, tempstr1);


  float temperature3 = ot.getVentExhaustInTemperature();
  String tempstr3 = String(temperature3);
  
  Serial.println("Exhaust In: " + tempstr3 + "C");
  client.publish(mqttch_exhaustintemp, tempstr3);
}

 void get_pub_diagnostic() {
   Serial.println("Geting and publishing diagnostic information");
   bool ventMode = ot.getVentilationMode();
   bool bypassStatus = ot.getBypassStatus();
   bool bypassAutomaticStatus = ot.getBypassAutomaticStatus();
   bool diagnostic = ot.getDiagnosticIndication();
   bool fault = ot.getFaultIndication();
   Serial.print("Vent mode: ");
   Serial.println(ventMode);
   Serial.print("Bypassstatus: ");
   Serial.println(bypassStatus);
   Serial.print("BypassAutomaticStatus: ");
   Serial.println(bypassAutomaticStatus);
   Serial.print("Fault: ");
   Serial.println(fault);
   Serial.print("Diagnostic: ");
   Serial.println(diagnostic);


   client.publish(mqttch_ventmode, String(ventMode));
   client.publish(mqttch_diagnostic, String(diagnostic));
   client.publish(mqttch_fault, String(fault));
   client.publish(mqttch_bypasstatus, String(bypassStatus));
   client.publish(mqttch_automaticstatus, String(bypassAutomaticStatus));
}

/**
 * If the current speed is not the desired speed, write it to
 * the Vents
 */
int set_speed(int cur_speed) {
  // Be some what lenient
  if (cur_speed <= (int) desiredNomVent + 4 && cur_speed >= (int) desiredNomVent - 4) {
    Serial.print("Already have desired speed of ");
    Serial.println(cur_speed);
    return 0;
  } else {
    Serial.print("Setting Nominal Ventilation to: ");
    Serial.println(desiredNomVent);
    unsigned int result = ot.setVentilation(desiredNomVent);
    Serial.print("Result: ");
    Serial.println(result);
    return 1;
  }
}

void loop()
{
    get_pub_temps();
    get_pub_diagnostic();

    // Now get the ventilation status
    unsigned long response2 = ot.OpenTherm::sendRequest(
            ot.OpenTherm::buildRequest(OpenThermMessageType::READ_DATA,
                      VentNomVent, 0));

    
    int nom_vent = ot.getVentilation();
    Serial.print("Current Nominal Ventilation: ");
    Serial.println(nom_vent);
    int didsetspeed = set_speed(nom_vent);
    int rpm =  nom_vent * 37.2;
    // 3370 -> 98
    // 930 -> 25
    // 1680 -> 45
    // 2800 -> 75
    
    Serial.print("RPM: ");
    Serial.println(rpm);
    Serial.println("Publishing RPM");
    String rpmstr = String(rpm);
    client.publish(mqttch_speed, rpmstr);
    client.loop();
    Serial.println();
    if (wifi_delay.isExpired()) {
        // retry the connection
        if (!client.connected()) {
            connect();
        }
        wifi_delay.repeat();
    }
    // skip sleep if speed has been set
    if (didsetspeed != 1){
      delay(5000);
    }
}
