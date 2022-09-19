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
  virtual void onInit(String topic){
    Serial.println("C++ TO SHIT");
  }
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

String chipId = String(ESP.getChipId());
MqNode buttonA = MqBranch("keys", MqBranch(chipId.c_str(), MqButton(14)));
MqNode buttonB = MqBranch("keys", MqBranch(chipId.c_str(), MqButton(13)));
MqNode buttonC = MqBranch("keys", MqBranch(chipId.c_str(), MqButton(12)));



void setup() {
  Serial.begin(9600);
  client.begin("eduram", "zarazcipodam");
  mqttClient.setServer("mqtt.hack", 1883);

  Serial.println("XD");

  mqttClient.onConnect([](bool b) {
    Serial.println("CONNECTED");
    buttonA.onInit("");
    buttonB.onInit("");
    buttonC.onInit("");
    Serial.println("IT IS DONE");
  });
  mqttClient.connect();
}

void loop() {}
