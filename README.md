# Step

1. Install Requirements

    Install this library using PlatformIO>Libraries

    - PubSubClient
    - DHT sensor library

2. Setup your WiFi and MQTT Broker

    Set your WiFi SSID and Password in `src/main.cpp`

    ```cpp
    const char* ssid = "Ngento";
    const char* password = "Baruuuuu";
    ```

    Set your MQTT Broker in `src/main.cpp`

    ```cpp
    const char* mqtt_server = "broker.hivemq.com";
    const char* mqtt_output_topic = "esp32-iot-udayana/output";
    const char* mqtt_setting_topic = "esp32-iot-udayana/setting";
    ```

    mqtt_server is the IP address of your MQTT Broker.
    mqtt_output_topic is the topic where the ESP32 will publish the data. mqtt_setting_topic is the topic where the ESP32 will subscribe to get the setting.

3. Upload the code to your ESP32

    Upload the code using PlatformIO>Upload

4. Test

    Default setitng use public MQTT Broker in HiveMQ. To test:

    - Connect to `broker.hivemq.com` using MQTT Client (e.g. [HiveMQ Websocket Client](http://www.hivemq.com/demos/websocket-client/))
    - Subscribe to `esp32-iot-udayana/output` topic
    - Publish to `esp32-iot-udayana/setting` topic with payload `0,31.0,0,0,0,0,1,0,0` (this is the setting to activate the relay if the temperature is above 31.0 degree celcius). The format is explained below.

        ```cpp
        // MQTT message will be in custom format of:
        // "%f,%f,%f,%f,%f,%f,%d,%d,%d"
        // temperatureLessThan,temperatureGreaterThan,humidityLessThan,humidityGreaterThan,lightLessThan,lightGreaterThan,isTemperatureEnabled,isHumidityEnabled,isLightEnabled
        // Example:
        // 0,31.0,0,0,0,0,1,0,0 # Temperature less than 31, temperature enabled
        ```

    - You should see the relay is activated if the temperature is above 31.0 degree celcius
