#include <Arduino.h>

// Additional Serial / UART
#include <SoftwareSerial.h>

// MH-Z19 sensor library
#include <MHZ19.h>

// NewPixel library
#include <Adafruit_NeoPixel.h>

// WIFi
#include <ESP8266WiFi.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

// PubSub MQTT Lib
#include <PubSubClient.h>

#include <DNSServer.h>

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
#define CONF_ZERO_CALIBRATION_TIME_MS   1260000 // 21m
#define CONF_MEASUREMENT_INTERVALL_MS   10000   // 10s
#define CONF_ZERO_CALIBRATION_PIN       D3


// raise error if values not set
#ifndef WIFI_CONFIG_H
#error Include a valid wifi_config.h
#endif
#ifndef WIFI_CONFIG_SSID
#error WIFI_CONFIG_SSID must be defined
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

// check sensor and force reboot
void checkSensorReturnCode();

// interrupt for zero calibration
ICACHE_RAM_ATTR void detectZeroCalibrationButtonPush() {
  Serial.println("DEBUG: Interrupt");
  currentApplicationMode = MODE_ZERO_CALIBRATION;
  zeroCalibrationStartTimeMS = millis();
}

DNSServer dnsServer;
AsyncWebServer server(80);

int co2Value;
float temperature;

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    //request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    unsigned long now = millis();
    long refreshtime;
    switch (currentApplicationMode) {
    case MODE_INITIALIZATION:
      refreshtime = 2000;
      break;
    case MODE_ZERO_CALIBRATION:
      refreshtime = 10000;
    default:
      refreshtime = 30000;
      break;
    }

    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print("<!doctype html><html lang=\"de\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>CO2-Ampel-");
    response->print(ESP.getChipId());
    response->print("</title><style>");
    response->print("body {font-family: Arial, Helvetica, sans-serif;color: #333;text-align: center;}");
    response->print("body > hr {opacity: 50%;}");
    response->print(".light-gray {color:#000;background-color:#f1f1f1;}");
    response->print(".gray {color:#000;background-color:#9e9e9e;}");
    response->print(".good {color: darkgreen;}");
    response->print(".medium {color: darkorange;font-weight: bold;}");
    response->print(".bad {color: darkred;font-weight: bolder;}</style></head>");
    response->print("<body onLoad=\"timeRefresh(");
    response->print(refreshtime);
    response->print(")\">");
    response->print("<h1>CO2-Ampel</h1><hr>");
    switch (currentApplicationMode) {
    case MODE_INITIALIZATION:
      response->print("<p>Initizalisiere Sensor</p><div class=\"light-gray\"><div class=\"gray\" style=\"height:24px;width:");
      response->print(map((now - initalCalibrationStartTimeMS), 0, CONF_WARMUP_TIME_MS, 0, 100));
      response->print("%\"></div></div>");
      break;
    case MODE_ZERO_CALIBRATION:
      response->print("<p>Zero calibration ... </p><div class=\"light-gray\"><div class=\"gray\" style=\"height:24px;width:");
      response->print(map((now - zeroCalibrationStartTimeMS), 0, CONF_ZERO_CALIBRATION_TIME_MS, 0, 100));
      response->print("%\"></div></div>");
      break;
    case MODE_MEASUREMENT:
    default:
      response->print("<p class=\"");
      if (co2Value <= CO2_THRESHOLD_GOOD || (co2Value > CO2_THRESHOLD_GOOD) && (co2Value <= CO2_THRESHOLD_MEDIUM)) {
        response->print("good");
      }
      else if ((co2Value > CO2_THRESHOLD_MEDIUM) && (co2Value <= CO2_THRESHOLD_BAD)) {
        response->print("medium");
      }
      else if (co2Value > CO2_THRESHOLD_BAD) {
        response->print("bad");
      }
      if (co2Value <= CO2_THRESHOLD_GOOD) {
        response->print("good");
      }
      response->print("\">CO2: ");
      response->print(co2Value);
      response->print("ppm</p><p>Temperature: ");
      response->print(temperature);
      response->print("°C</p>");
      break;
    }
    response->print("<script>function timeRefresh(time) {setTimeout(\"location.reload(true);\", time);}</script></body></html>");
    request->send(response);
  }
};

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
  // Serial.printf("Setup: WiFi connecting to %s\n\r", WIFI_CONFIG_SSID);
  // #ifndef WIFI_CONFIG_PASSWORD
  // WiFi.begin(WIFI_CONFIG_SSID);
  // #else
  // WiFi.begin(WIFI_CONFIG_SSID, WIFI_CONFIG_PASSWORD);
  // #endif
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println();
  // Serial.print("Connected, IP: ");
  // Serial.println(WiFi.localIP());
  String ap_name = String("CO2-Ampel-" + String(ESP.getChipId()));
  WiFi.softAP(ap_name.c_str());
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
  server.begin();

  mqttClient.setServer(MQTT_CONFIG_SERVER, 1883);

  // finished time of setup, used for initial calibration
  initalCalibrationStartTimeMS = millis();
}

void loop() {
  unsigned long now = millis();

  dnsServer.processNextRequest();
  // mqtt
  // c&p: https://de.mathworks.com/help/thingspeak/use-arduino-client-to-publish-to-a-channel.html
  if (!mqttClient.connected()) {
    char mqttClientID[9];
    while (!mqttClient.connected()) {
      // Generate mqttClientID
      for (int i = 0; i < 8; i++) {
          mqttClientID[i] = alphanum[random(51)];
      }
      mqttClientID[8]='\0';

      if (mqttClient.connect(mqttClientID, MQTT_CONFIG_USER, MQTT_CONFIG_PASS)) {
        Serial.println("MQTT: connected");
      }
      else {
        Serial.print("failed, rc=");
        // Print reason the connection failed.
        // See https://pubsubclient.knolleary.net/api.html#state for the failure code explanation.
        Serial.print(mqttClient.state());
        delay(100);
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
      co2Value = mhz19Sensor.getCO2();
      temperature = mhz19Sensor.getTemperature();

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

      String json = String(
        "{ \"temperatur\": " 
        + String(temperature, 0) 
        + ", \"ppmCO2\": " 
        + String(co2Value) + " }"
      );
      //Serial.println(json);
      String topic = String("/co2ampel/" + String(ESP.getChipId()));
      //Serial.println(topic);

      if (mqttClient.publish(topic.c_str(), json.c_str())) {
        Serial.println("Send OK");
      }

      delay(10);
    }
    break;
  }
}


void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<neoPixels.numPixels(); i++) { // For each pixel in strip...
    neoPixels.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    neoPixels.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
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
    ESP.reset();
  }
}