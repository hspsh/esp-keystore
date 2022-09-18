#include <Arduino.h>
#include "Button.h"
#include "RestClient.h"
#include <AsyncMqttClient.h>
#include <ESP8266WiFi.h>
#include <string>

RestClient client = RestClient("banana.at.hsp.net.pl", 8000);

AsyncMqttClient mqttClient;
Button buttonA(14, INPUT);
Button buttonB(13, INPUT);
Button buttonC(12, INPUT);

bool connected = false;

String deviceTopic = "";
String topicA = "";
String topicB = "";
String topicC = "";

void setup() {
  Serial.begin(9600);
  client.begin("eduram", "zarazcipodam");
  mqttClient.setServer("mqtt.hack", 1883);

  deviceTopic.concat("keys");
  deviceTopic.concat("/");
  deviceTopic.concat(String(ESP.getChipId()));
  deviceTopic.concat("/");
  deviceTopic.end();

  topicA.concat(deviceTopic);
  topicA.concat("A");

  topicB.concat(deviceTopic);
  topicB.concat("B");

  topicC.concat(deviceTopic);
  topicC.concat("C");


  mqttClient.onConnect([&](bool b) {
    Serial.println("CONNECTED");
    mqttClient.publish(topicA.c_str(), 0, true, buttonA.isPressed() ? "YES" : "NO");
    mqttClient.publish(topicB.c_str(), 0, true, buttonB.isPressed() ? "YES" : "NO");
    mqttClient.publish(topicC.c_str(), 0, true, buttonC.isPressed() ? "YES" : "NO");
    connected = true;
  });
  mqttClient.connect();

  buttonA.onChange([]() {
    mqttClient.publish(topicA.c_str(), 0, true, buttonA.isPressed() ? "YES" : "NO");
  });
  buttonB.onChange([]() {
    mqttClient.publish(topicB.c_str(), 0, true, buttonB.isPressed() ? "YES" : "NO");
  });
  buttonC.onChange([]() {
    mqttClient.publish(topicC.c_str(), 0, true, buttonC.isPressed() ? "YES" : "NO");
  });
}

void loop() {
}
