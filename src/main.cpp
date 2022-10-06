#include "Button.h"
// #include "RestClient.h"
#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <ESP8266WiFi.h>
// #include <WiFi.h>

#include <ArduinoOTA.h>
#include <initializer_list>
#include <string>



#include "secrets.h"

// RestClient client = RestClient("banana.at.hsp.net.pl", 8000);
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

class MqIoNotif : public MqNode {
  String name;
  uint8 io_num;

public:
  MqIoNotif(String name, uint8 port) : name(name) {
    io_num = port;
  }

  void onInit(String topic) {
    topic.concat("/");
    topic.concat(name);

    mqttClient.onMessage(
      [this](char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total){
        //do stuff at some point
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
    // root may subscribe to everything for debug purposes
    // String topicWildcard = name+"/#";
    // mqttClient.subscribe(topicWildcard.c_str(), 2);
  }

  void handleMessage(const char* topic, const char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    // Serial.println("Service %s received %s",name.c_str(),(char*)topic);
  }
};

// String deviceName = String(zESP.getChipId());
String deviceName = "triton one";

MqRoot<1> *mqTrittonService = 
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


void attachMessageHandler(AsyncMqttClient& client){
  mqttClient.onMessage(
    [](char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total){
      mqTrittonService->handleMessage(topic,payload,properties,len,index,total);
    });
}

void setup() {
  Serial.begin(115200);
  // client.begin(, IOT_WIFI_PASSWD);
  pinMode(PIN_LED, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(IOT_WIFI_NAME, IOT_WIFI_PASSWD);
  while (WiFi.waitForConnectResult(3000) != WL_CONNECTED) {
    static bool flag = false;
    digitalWrite(PIN_LED,flag);
    flag = !flag;
  }

  ArduinoOTA.setPassword(ARDUINO_OTA_PASSWD);

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  mqttClient.setServer("mqtt.hack", 1883);
  mqttClient.onConnect([](bool b) { mqTrittonService->onInit(); });
  // mqttClient.onMessage(
  //   [](char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total){
  //     mqTrittonService->handleMessage(topic,payload,properties,len,index,total);
  //   });
  mqttClient.connect();

}

void loop() {
  static int test = 0;
  static uint8_t heartbeat_pattern[] = {1,0,0,1,0,0,0,0,0,0,0,0,0};
  
  uint8_t pattern_index = (test++)% sizeof(heartbeat_pattern);
  digitalWrite(PIN_LED, heartbeat_pattern[pattern_index]);
  ArduinoOTA.handle();
  delay(100);
}
