#include <Arduino.h>

// Additional Serial / UART
#include <SoftwareSerial.h>

// MH-Z19 sensor library
#include <MHZ19.h>

// NewPixel library
#include <Adafruit_NeoPixel.h>

// WIFi
#include <ESP8266WiFi.h>

// PubSub MQTT Lib
#include <PubSubClient.h>


// WiFi config
#include <wifi_config.h>

// MQTT config
#include <mqtt_config.h>

// CO2 sensor configuration
#define MHZ19_BAUDRATE  9600    // default baudrate of sensor
#define MHZ19_TX_PIN    D7      // TX pin MH-Z19(B) (SoftwareSerail RX)
#define MHZ10_RX_PIN    D8      // RX pin MH-Z19(B) (SoftwareSerial TX)

// CO2 thresholds
#define CO2_THRESHOLD_GOOD    500
#define CO2_THRESHOLD_MEDIUM  800
#define CO2_THRESHOLD_BAD     1100

// NeoPixel configuration
#define NEOPIXEL_COUNT      12
#define NEOPIXEL_PIN        D5
#define NEOPIXEL_BRIGHTNESS 255   // 255 max

// application configuration
#define CONF_WARMUP_TIME_MS             180000  // 180s
#define CONF_ZERO_CALIBRATION_TIME_MS  1260000 // 21m
#define CONF_MEASUREMENT_INTERVALL_MS   10000   // 10s
#define CONF_ZERO_CALIBRATION_PIN       D3


#ifndef WIFI_CONFIG_H
#error Include a valid wifi_config.h
#endif

#ifndef MQTT_CONFIG_H
#error Include a valid mqtt_config.h
#endif
// TODO: server / username / password handling


// SoftwareSerial to communicate with sensor
SoftwareSerial mhz19Serial(MHZ19_TX_PIN, MHZ10_RX_PIN);

// MHZ19 sensor
MHZ19 mhz19Sensor;

// NeoPixel strip / ring
Adafruit_NeoPixel neoPixels(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// WiFi client
WiFiClient wifiClient;

// MQTT client
PubSubClient mqttClient(wifiClient);

static const char alphanum[] ="0123456789"
                              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                              "abcdefghijklmnopqrstuvwxyz";  // For random generation of client ID.

// values to start "zero calibration"
bool sendZeroCalibrationCmd = true;
unsigned long zeroCalibrationStartTimeMS;
// values to run initial "calibration"
bool initalCalibration = true;
unsigned long initalCalibrationStartTimeMS;
// Assume Wifi does not work, if connect set to true
bool wifiConnectionActive = false;
// Assume Mqtt to Work until proven wrong
bool attemptMqttConnect = true;
bool mqttConnectionActive = true;

// different operating modes
enum APPLICATION_MODE {
  MODE_INITIALIZATION,      // initialization
  MODE_ZERO_CALIBRATION,    // zero calibration
  MODE_MEASUREMENT          // default: do measurement
};

uint8_t currentApplicationMode = MODE_INITIALIZATION;

// change color of NeoPixels
void colorWipe(uint32_t color, int wait);
void loadingAnimation(uint8_t percent);

// interrupt for zero calibration
ICACHE_RAM_ATTR void detectZeroCalibrationButtonPush() {
  Serial.println("DEBUG: Interrupt");
  currentApplicationMode = MODE_ZERO_CALIBRATION;
  zeroCalibrationStartTimeMS = millis();
}

void checkSensorReturnCode() {
    if (mhz19Sensor.errorCode != RESULT_OK) {
        Serial.println("FAILED TO READ SENSOR!");
        Serial.printf("Error code: %d\n\r", mhz19Sensor.errorCode);
        for (uint8_t i=0; i<2; i++) {
            colorWipe(neoPixels.Color(0, 255, 255), 50);
            delay(500);
            colorWipe(neoPixels.Color(0,0,0), 50);
            delay(500);
        }
    }
}

void setup() {
  // Setup serial for output
  Serial.begin(9600);

  // Setup software serial for communication with sensor
  Serial.println("Setup: SoftwareSerial for MH-Z19 sensor");
  mhz19Serial.begin(MHZ19_BAUDRATE);

  // MHZ19 init
  Serial.println("Setup: Initializing MH-Z19 sensor");
  mhz19Sensor.begin(mhz19Serial);
  // enable auto calibration (lowest value in 24h as baseline for 400ppm)
  mhz19Sensor.autoCalibration();

  // NeoPixel init
  Serial.println("Setup: Initializing NeoPixels");
  neoPixels.begin();  // init
  neoPixels.show();   // off
  neoPixels.setBrightness(NEOPIXEL_BRIGHTNESS);

  // interrupt pin for zero calibration
  pinMode(CONF_ZERO_CALIBRATION_PIN, INPUT_PULLUP);
  attachInterrupt(
    digitalPinToInterrupt(CONF_ZERO_CALIBRATION_PIN), 
    detectZeroCalibrationButtonPush, 
    RISING
  );

  checkSensorReturnCode();

  // WiFi
#ifdef WIFI_CONFIG_SSID
    Serial.printf("Setup: WiFi connecting to %s\n\r", WIFI_CONFIG_SSID);
    #ifndef WIFI_CONFIG_PASSWORD
      WiFi.begin(WIFI_CONFIG_SSID);
    #else
      WiFi.begin(WIFI_CONFIG_SSID, WIFI_CONFIG_PASSWORD);
    #endif
  for (int i=0; i<20; i++) { //try 10s
      if (WiFi.status() == WL_CONNECTED) break;
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.print("Connected, IP: ");
      Serial.println(WiFi.localIP());
      wifiConnectionActive = true;
  } else {
      Serial.println("Could not Connect to Wifi");
      wifiConnectionActive = false;
  }
#else
  Serial.println("No Wifi Info Present, Light Only Mode");
  wifiConnectionActive = false;
#endif

  mqttClient.setServer(MQTT_CONFIG_SERVER, 1883);

  // finished time of setup, used for initial calibration
  initalCalibrationStartTimeMS = millis();
}

void loop() {
  unsigned long now = millis();

  if (wifiConnectionActive && attemptMqttConnect && !mqttClient.connected()) {
    char mqttClientID[9];
    //Attempt to Connect 10 times with growing intervals
    for (int retries = 0; retries < 10; retries++) {

      // Generate mqttClientID
      for (int i = 0; i < 8; i++) mqttClientID[i] = alphanum[random(51)];
      mqttClientID[8]='\0';

      if (mqttClient.connect(mqttClientID, MQTT_CONFIG_USER, MQTT_CONFIG_PASS)) {
        Serial.println("MQTT: connected");
        mqttConnectionActive = true;
        attemptMqttConnect = true;
        retries=10; //break the loop instantly
      } else {
        Serial.println(retries);
        // Print reason the connection failed.
        // See https://pubsubclient.knolleary.net/api.html#state for the failure code explanation.
        Serial.print("failed, rc="); Serial.println(mqttClient.state());
        mqttConnectionActive = false;
        attemptMqttConnect = false;
        delay(retries * 50);
      }      
    }
  }

  switch (currentApplicationMode) {
  case MODE_INITIALIZATION:
    // may move into function
    if (now - initalCalibrationStartTimeMS < CONF_WARMUP_TIME_MS) {
      if (now % 1000 == 0) {
        // NOTE: may use Serial.printf()
        Serial.print("Initial calibration in progress: ");
        Serial.print((now - initalCalibrationStartTimeMS)/1000);
        Serial.print("/");
        Serial.print(CONF_WARMUP_TIME_MS/1000);
        Serial.println("s");
        loadingAnimation(map((now - initalCalibrationStartTimeMS), 0, CONF_WARMUP_TIME_MS, 0, 100));
        delay(10);
      }
    }
    // change mode to measurement
    else {
      currentApplicationMode = MODE_MEASUREMENT;
    }
    break;
  case MODE_ZERO_CALIBRATION:
    // may move into function
    if (now - initalCalibrationStartTimeMS < CONF_WARMUP_TIME_MS) {
      // send zero calibration command to sensor
      if (sendZeroCalibrationCmd) {
        colorWipe(neoPixels.Color(0,0,0), 100);
        Serial.println("Start zero calibration progress.");
        mhz19Sensor.calibrateZero();
        sendZeroCalibrationCmd = false;
      }
      // send status to serial
      if (now % 10000 == 0) {
        // NOTE: may use Serial.printf()
        Serial.print("Zero calibration in progress: ");
        Serial.print((now - zeroCalibrationStartTimeMS)/1000);
        Serial.print("/");
        Serial.print(CONF_ZERO_CALIBRATION_TIME_MS/1000);
        Serial.println("s");
        delay(10);
      }
      // update neopixels every second
      if (now % 1000 == 0) {
        loadingAnimation(
          map((now - zeroCalibrationStartTimeMS), 0, CONF_ZERO_CALIBRATION_TIME_MS, 0, 100)
        );
      }
    }
    // zero calibration is done -> measurement mode
    else {
      currentApplicationMode = MODE_MEASUREMENT;
      // set send flag to true if we change again back to zero calibration mode
      sendZeroCalibrationCmd = true;
    }
    break;
  // default mode is measurement
  case MODE_MEASUREMENT:
  default:
    if (now % CONF_MEASUREMENT_INTERVALL_MS == 0) {
      int co2Value = mhz19Sensor.getCO2();
      float temperature = mhz19Sensor.getTemperature();

      checkSensorReturnCode();

      Serial.printf("CO2 [ppm]: %4i Temperature [C]: %.0f\n\r", co2Value, temperature);

      if (co2Value <= CO2_THRESHOLD_GOOD) {
        colorWipe(neoPixels.Color(  0, 255,   0), 200);
      }
      else if ((co2Value > CO2_THRESHOLD_GOOD) && (co2Value <= CO2_THRESHOLD_MEDIUM)) {
        colorWipe(neoPixels.Color(  255, 255,   0), 200);
      }
      else if ((co2Value > CO2_THRESHOLD_MEDIUM) && (co2Value <= CO2_THRESHOLD_BAD)) {
        colorWipe(neoPixels.Color(  255, 165,   0), 200);
      }
      else if (co2Value > CO2_THRESHOLD_BAD) {
        colorWipe(neoPixels.Color(  255, 0,   0), 200);
      }

      //If We have Wifi and MQTT, send it
      if (wifiConnectionActive && mqttConnectionActive) {
        String json = String("{ \"temperatur\": " + String(temperature, 0) + ", \"ppmCO2\": " + String(co2Value) + " }");
        //Serial.println(json);
        String topic = String("/co2ampel/" + String(ESP.getChipId()));
        //Serial.println(topic);
        if (mqttClient.publish(topic.c_str(), json.c_str())) {
          Serial.println("Send OK");
        }
      }

      delay(10);
    }
    break;
  }
}


void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<neoPixels.numPixels(); i++) {
    neoPixels.setPixelColor(i, color);
    neoPixels.show();
    delay(wait);
  }
}

void loadingAnimation(uint8_t percent) {
  int num_pixel_on = map(percent, 0, 100, 0, neoPixels.numPixels()-1);
  for (int i=0; i<num_pixel_on; i++) {
    neoPixels.setPixelColor(i, neoPixels.Color(255, 255, 255));
    neoPixels.show();
  }
  if (num_pixel_on < neoPixels.numPixels()) {
    for (uint8_t j=0; j<255; j++) {
      neoPixels.setPixelColor(num_pixel_on, neoPixels.Color(j, j, j));
      neoPixels.show();
    }
    for (uint8_t k=255; k>0; k--) {
      neoPixels.setPixelColor(num_pixel_on, neoPixels.Color(k, k, k));
      neoPixels.show();
    }
  }
}
