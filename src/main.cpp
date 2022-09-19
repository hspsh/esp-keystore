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

class MqButton : public MqNode {
  Button button;
  String name;

public:
  MqButton(String name, uint8 port) : button(port, INPUT), name(name) {}

  void onInit(String topic) {
    topic.concat("/");
    topic.concat(name);

    mqttClient.publish(topic.c_str(), 0, true,
                       button.isPressed() ? "YES" : "NO");
    button.onChange([topic, this]() {
      mqttClient.publish(topic.c_str(), 0, true,
                         button.isPressed() ? "YES" : "NO");
    });
  }
};

template<int SIZE> class MqBranch : public MqNode {
  const char *name;
  std::array<MqNode*, SIZE> mqNodes;

public:
  MqBranch(const char *name, std::array<MqNode*, SIZE> mqNodes) : name(name), mqNodes(mqNodes) {}

  void onInit(String topic) {
    topic.concat("/");
    topic.concat(name);

    for(auto mqNode: mqNodes){
        mqNode->onInit(topic);
    }
  }
};

template<int SIZE> class MqRoot {
  String name;
  std::array<MqNode*, SIZE> mqNodes;

public:
  MqRoot(const char *name, std::array<MqNode*, SIZE> mqNodes) : name(name), mqNodes(mqNodes) {}

  void onInit() {
    for(auto mqNode: mqNodes){
        mqNode->onInit(name);
    }
  }
};

String chipId = String(ESP.getChipId());
MqRoot<1> *buttons = 
    new MqRoot<1>("keys", {
        new MqBranch<3>(chipId.c_str(), {
                new MqButton("A", 14),
                new MqButton("B", 13), 
                new MqButton("C", 12)
            })
        });

void setup() {
  Serial.begin(9600);
  client.begin("eduram", "zarazcipodam");
  mqttClient.setServer("mqtt.hack", 1883);

  mqttClient.onConnect([](bool b) { buttons->onInit(); });
  mqttClient.connect();
}

void loop() {}
