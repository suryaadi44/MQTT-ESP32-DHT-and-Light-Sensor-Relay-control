#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 32
#define DHTTYPE DHT22
#define TEMT6000PIN 34
#define LEDPIN 2
#define RELAYPIN 25

#define SENSOR_UPDATE_INTERVAL 500
#define SEND_INTERVAL 1000
#define STATE_UPDATE_INTERVAL 500

const char* ssid = "Ngento";
const char* password = "Baruuuuu";

const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_output_topic = "esp32-iot-udayana/output";
const char* mqtt_setting_topic = "esp32-iot-udayana/setting";

WiFiClient espClient;
PubSubClient client(espClient);
DHT_Unified dht(DHTPIN, DHTTYPE);

sensors_event_t humidityEvent, tempEvent;
float lux = 0;

long lastMsg = 0;
long lastUpdate = 0;
long lastEvent = 0;
int relayState = LOW;
char msg[100];
int value = 0;

struct message_t {
    float temperature;
    float humidity;
    float light;
};

// Get config from MQTT
// MQTT message will be in custom format of:
// "%f,%f,%f,%f,%f,%f,%d,%d,%d"
// temperatureLessThan,temperatureGreaterThan,humidityLessThan,humidityGreaterThan,lightLessThan,lightGreaterThan,isTemperatureEnabled,isHumidityEnabled,isLightEnabled
// Example:
// 0,31.0,0,0,0,0,1,0,0 # Temperature less than 31, temperature enabled
struct config_t {
    float temperatureLessThan = 0;
    float temperatureGreaterThan = 100;
    float humidityLessThan = 0;
    float humidityGreaterThan = 100;
    float lightLessThan = 0;
    float lightGreaterThan = 900;

    bool isTemperatureEnabled = true;
    bool isHumidityEnabled = false;
    bool isLightEnabled = false;
} globalConfig;

void callback(char* topic, byte* message, unsigned int length) {
    Serial.print("Sub: Message arrived on topic:");
    Serial.print(topic);
    Serial.print(".Message: ");
    String messageTemp;

    for (int i = 0; i < length; i++) {
        Serial.print((char)message[i]);
        messageTemp += (char)message[i];
    }
    Serial.println();

    // Change config based on MQTT message
    int err = sscanf(messageTemp.c_str(), "%f,%f,%f,%f,%f,%f,%d,%d,%d", &globalConfig.temperatureLessThan, &globalConfig.temperatureGreaterThan, &globalConfig.humidityLessThan, &globalConfig.humidityGreaterThan, &globalConfig.lightLessThan, &globalConfig.lightGreaterThan, &globalConfig.isTemperatureEnabled, &globalConfig.isHumidityEnabled, &globalConfig.isLightEnabled);
    if (err != 9) {
        Serial.println("Sub: Error parsing MQTT message!");
        return;
    }

    Serial.printf("\ttemperatureLessThan: %f\n", globalConfig.temperatureLessThan);
    Serial.printf("\ttemperatureGreaterThan: %f\n", globalConfig.temperatureGreaterThan);
    Serial.printf("\thumidityLessThan: %f\n", globalConfig.humidityLessThan);
    Serial.printf("\thumidityGreaterThan: %f\n", globalConfig.humidityGreaterThan);
    Serial.printf("\tlightLessThan: %f\n", globalConfig.lightLessThan);
    Serial.printf("\tlightGreaterThan: %f\n", globalConfig.lightGreaterThan);
    Serial.printf("\tisTemperatureEnabled: %d\n", globalConfig.isTemperatureEnabled);
    Serial.printf("\tisHumidityEnabled: %d\n", globalConfig.isHumidityEnabled);
    Serial.printf("\tisLightEnabled: %d\n", globalConfig.isLightEnabled);
}

void setup_wifi() {
    delay(10);

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP addresSub: ");
    Serial.println(WiFi.localIP());
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");

        if (client.connect("ESP32Client")) {
            Serial.println("connected");
            client.subscribe(mqtt_setting_topic);
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");

            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);

    setup_wifi();
    dht.begin();

    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    pinMode(LEDPIN, OUTPUT);
    pinMode(RELAYPIN, OUTPUT);
    pinMode(TEMT6000PIN, INPUT);

    digitalWrite(RELAYPIN, LOW);
}

float readLightSensor() {
    analogReadResolution(10);

    float volts = analogRead(TEMT6000PIN) * 5 / 1024.0; // Convert reading to VOLTS
    float VoltPercent = analogRead(TEMT6000PIN) / 1024.0 * 100; //Reading to Percent of Voltage

    //Conversions from reading to LUX
    float amps = volts / 10000.0;  // 10,000 Ohms from datasheet
    float microamps = amps * 1000000; // Convert to Microamps
    return  microamps * 2.0; // retunr LUX
}

void handleRelayState() {
    int relayScore = 0;
    int minScore = 0;

    if (globalConfig.isHumidityEnabled) {
        minScore++;
        if (globalConfig.humidityLessThan != 0 && humidityEvent.relative_humidity < globalConfig.humidityLessThan) {
            relayScore++;
        } else if (globalConfig.humidityGreaterThan != 0 && humidityEvent.relative_humidity > globalConfig.humidityGreaterThan) {
            relayScore++;
        }
    }

    if (globalConfig.isTemperatureEnabled) {
        minScore++;
        if (globalConfig.temperatureLessThan != 0 && tempEvent.temperature < globalConfig.temperatureLessThan) {
            relayScore++;
        } else if (globalConfig.temperatureGreaterThan != 0 && tempEvent.temperature > globalConfig.temperatureGreaterThan) {
            relayScore++;
        }
    }

    if (globalConfig.isLightEnabled) {
        minScore++;
        if (globalConfig.lightLessThan != 0 && lux < globalConfig.lightLessThan) {
            relayScore++;
        } else if (globalConfig.lightGreaterThan != 0 && lux > globalConfig.lightGreaterThan) {
            relayScore++;
        }
    }

    Serial.printf("Sen: T: %f, H: %f, L: %f\n", tempEvent.temperature, humidityEvent.relative_humidity, lux);
    // Serial.printf("Min score: %d\n", minScore);
    // Serial.printf("Relay score: %d\n", relayScore);

    // Turn on relay if score is less than min score and min score is not 0 (means no condition is enabled)
    if (relayScore < minScore && minScore != 0) {
        if (relayState == LOW) {
            digitalWrite(RELAYPIN, HIGH);
            relayState = HIGH;
        }
    } else {
        if (relayState == HIGH) {
            digitalWrite(RELAYPIN, LOW);
            relayState = LOW;
        }
    }
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }

    client.loop();

    long now = millis();

    // Update sensor data
    if (now - lastEvent > SENSOR_UPDATE_INTERVAL) {
        // Read temperature and humidity
        dht.temperature().getEvent(&tempEvent);
        dht.humidity().getEvent(&humidityEvent);
        if (isnan(tempEvent.temperature) || isnan(humidityEvent.relative_humidity)) {
            Serial.println("Sen: Error reading dht sensor!");
            return;
        }

        lux = readLightSensor();
    }

    // Update relay state
    if (now - lastUpdate > STATE_UPDATE_INTERVAL) {
        lastUpdate = now;
        handleRelayState();
    }

    // Send message to MQTT topic
    if (now - lastMsg > SEND_INTERVAL) {
        lastMsg = now;
        message_t message = {
            .temperature = tempEvent.temperature,
            .humidity = humidityEvent.relative_humidity,
            .light = lux,
        };

        // Send message to MQTT topic
        String json = "{\"temperature\": " + String(message.temperature, 2) + ", \"humidity\": " + String(message.humidity, 2) + ", \"light\": " + String(message.light, 2) + "}";
        json.toCharArray(msg, 100);
        Serial.print("Pub: ");
        Serial.println(msg);
        client.publish(mqtt_output_topic, msg);
    }
}
