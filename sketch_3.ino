#include <WiFi.h>
#include <PZEM004Tv30.h>
#include <PubSubClient.h>

// Wi-Fi configuration
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// KiwisIoT MQTT configuration
const char* mqtt_server = "YOUR_MQTT_BROKER";
const int mqtt_port = YOUR_MQTT_PORT;
const char* mqtt_topic = "YOUR_DEVICE_TOPIC";

// ESP32 UART2 pins
#define PZEM_RX 16
#define PZEM_TX 17

// Example electricity tariff
const float TARIFF_PER_KWH = 8.0;

HardwareSerial pzemSerial(2);

PZEM004Tv30 pzem(
  pzemSerial,
  PZEM_RX,
  PZEM_TX
);

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastPublish = 0;

void connectWiFi() {

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting WiFi...");
  }

  Serial.println("WiFi Connected");
}

void reconnectMQTT() {

  while (!client.connected()) {

    if (client.connect("EnergyMonitorESP32")) {
      Serial.println("MQTT Connected");
    } else {
      Serial.println("MQTT connection failed");
      delay(2000);
    }
  }
}

void setup() {

  Serial.begin(115200);

  pzemSerial.begin(
    9600,
    SERIAL_8N1,
    PZEM_RX,
    PZEM_TX
  );

  connectWiFi();

  client.setServer(mqtt_server, mqtt_port);

  Serial.println("Energy Monitoring Started");
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  if (millis() - lastPublish >= 10000) {

    lastPublish = millis();

    float voltage = pzem.voltage();
    float current = pzem.current();
    float power = pzem.power();
    float energy = pzem.energy();
    float frequency = pzem.frequency();
    float pf = pzem.pf();

    if (isnan(voltage) ||
        isnan(current) ||
        isnan(power) ||
        isnan(energy)) {

      Serial.println("PZEM Reading Error");
      return;
    }

    float cost = energy * TARIFF_PER_KWH;

    Serial.print("Voltage: ");
    Serial.println(voltage);

    Serial.print("Current: ");
    Serial.println(current);

    Serial.print("Power: ");
    Serial.println(power);

    Serial.print("Energy: ");
    Serial.println(energy);

    Serial.print("Estimated Cost: Rs ");
    Serial.println(cost);

    char payload[256];

    snprintf(payload, sizeof(payload),
      "{\"voltage\":%.2f,"
      "\"current\":%.3f,"
      "\"power\":%.2f,"
      "\"energy\":%.3f,"
      "\"frequency\":%.2f,"
      "\"power_factor\":%.2f,"
      "\"estimated_cost\":%.2f}",
      voltage,
      current,
      power,
      energy,
      frequency,
      pf,
      cost
    );

    if (client.publish(mqtt_topic, payload)) {
      Serial.println("Energy Data Published");
    }
  }
}