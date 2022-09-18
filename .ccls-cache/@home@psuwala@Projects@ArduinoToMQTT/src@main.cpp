#include "Button.h"
#include "RestClient.h"
#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <ESP8266WiFi.h>
#include <string>

RestClient client = RestClient("banana.at.hsp.net.pl", 8000);
AsyncMqttClient mqttClient;

class MqNode {
public:
  virtual void onInit(String topic);
};

class MqBranch : public MqNode {
  const char *name;
  MqNode mqNode;
  String currentTopic = "";

public:
  MqBranch(const char *name, MqNode mqNode) : name(name), mqNode(mqNode) {}

  void onInit(String topic) {
    topic.concat("/");
    topic.concat(name);
    mqNode.onInit(currentTopic.c_str());
  }
};

class MqButton : public MqNode {
  Button button;

public:
  MqButton(uint8 port) : button(port, INPUT) {}

  void onInit(String topic) {
    mqttClient.publish(topic.c_str(), 0, true,
                       button.isPressed() ? "YES" : "NO");
    button.onChange([topic, this]() {
      mqttClient.publish(topic.c_str(), 0, true,
                         button.isPressed() ? "YES" : "NO");
    });
  }
};


MqNode buttonA = MqButton(14);
MqNode buttonB = MqButton(13);
MqNode buttonC = MqButton(12);

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

  Serial.println("XD");

  mqttClient.onConnect([](bool b) {
    Serial.println("CONNECTED");
    buttonA.onInit(topicA);
    buttonB.onInit(topicB);
    buttonC.onInit(topicC);
    Serial.println("IT IS DONE");
  });
  mqttClient.connect();
}

void loop() {}
