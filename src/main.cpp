#include "Button.h"
#include "RestClient.h"
#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <ESP8266WiFi.h>
#include <initializer_list>
#include <string>

RestClient client = RestClient("banana.at.hsp.net.pl", 8000);
AsyncMqttClient mqttClient;

const int PIN_LED = 4;

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


class MqStringProperty : public MqNode {
  String name, value;

public:
  MqStringProperty(String name, String value) : name(name), value(value) {}

  void onInit(String topic) {
    topic.concat("/");
    topic.concat(name);

    mqttClient.publish(topic.c_str(), 0, true, value.c_str());
  }
};

class MqStatsAggregate : public MqNode {
  MqStringProperty mqIpAddr;

public:
  MqStatsAggregate() : mqIpAddr("ipAddr","2137") {}

  void onInit(String topic) {
    topic.concat("/");
    topic.concat("stats");

    // mqttClient.publish(topic.c_str(), 0, true, "idk");
    mqIpAddr.onInit(topic);
  }
};


template<int SIZE> class MqBranch : public MqNode {
  String name;
  std::array<MqNode*, SIZE> mqNodes;

public:
  MqBranch(String name, std::array<MqNode*, SIZE> mqNodes) : name(name), mqNodes(mqNodes) {}

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
  MqRoot(String name, std::array<MqNode*, SIZE> mqNodes) : name(name), mqNodes(mqNodes) {}

  void onInit() {
    for(auto mqNode: mqNodes){
        mqNode->onInit(name);
    }
  }
};

// String deviceName = String(zESP.getChipId());
String deviceName = "triton one";

MqRoot<1> *buttons = 
    new MqRoot<1>("homie", {
      new MqBranch<2>(deviceName, {
        new MqBranch<3>("nuttons", {
          new MqButton("A", 14),
          new MqButton("B", 13), 
          new MqButton("C", 12)
        }),
        new MqStatsAggregate()
    })
  });


void setup() {
  Serial.begin(115200);
  client.begin("eduram", "zarazcipodam");
  mqttClient.setServer("mqtt.hack", 1883);

  pinMode(PIN_LED, OUTPUT);


  mqttClient.onConnect([](bool b) { buttons->onInit(); });
  mqttClient.connect();
}

void loop() {
  static int test = 0;
  static uint8_t heartbeat_pattern[] = {1,0,0,1,0,0,0,0,0,0,0,0,0};
  
  uint8_t pattern_index = (test++)% sizeof(heartbeat_pattern);
  digitalWrite(PIN_LED, heartbeat_pattern[pattern_index]);
  delay(100);
}
