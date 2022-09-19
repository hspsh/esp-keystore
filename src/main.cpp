#include "Button.h"
#include "RestClient.h"
#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <ESP8266WiFi.h>
#include <initializer_list>
#include <string>

RestClient client = RestClient("banana.at.hsp.net.pl", 8000);
AsyncMqttClient mqttClient;

class MqNode {
public:
  virtual void onInit(String topic) = 0;
};

class MqBranch : public MqNode {
  const char *name;
  MqNode *mqNode;
  MqBranch *next = NULL;

public:
  template <typename T, typename... Args>
  MqBranch(const char *name, T *mqNode, Args... mqNodes)
      : name(name), mqNode(mqNode), next(new MqBranch(name, mqNodes...)) {}

  template <typename T>
  MqBranch(const char *name, T *mqNode) : name(name), mqNode(mqNode) {}

  void onInit(String topic) {
    if (next != NULL)
      next->onInit(topic);

    topic.concat(topic.length() == 0 ? "" : "/");
    topic.concat(name);

    mqNode->onInit(topic);
  }
};

class MqButton : public MqNode {
  Button button;
  String name;

public:
  MqButton(String name, uint8 port) : button(port, INPUT), name(name) {}

  void onInit(String topic) {
    topic.concat(topic.length() == 0 ? "" : "/");
    topic.concat(name);

    mqttClient.publish(topic.c_str(), 0, true,
                       button.isPressed() ? "YES" : "NO");
    button.onChange([topic, this]() {
      mqttClient.publish(topic.c_str(), 0, true,
                         button.isPressed() ? "YES" : "NO");
    });
  }
};

String chipId = String(ESP.getChipId());
MqNode *buttons = 
    new MqBranch("keys", 
        new MqBranch(chipId.c_str(), 
            new MqButton("A", 14),
            new MqButton("B", 13), 
            new MqButton("C", 12)
        )
    );

void setup() {
  Serial.begin(9600);
  client.begin("eduram", "zarazcipodam");
  mqttClient.setServer("mqtt.hack", 1883);

  mqttClient.onConnect([](bool b) { buttons->onInit(""); });
  mqttClient.connect();
}

void loop() {}
