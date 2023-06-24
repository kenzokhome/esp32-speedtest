#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "ledDevice.h"
#include "time.h"

const char* ssid = "";
const char* password = "";

const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
const char* ntpServer = "pool.ntp.org";

String sensorReadings;
unsigned long download, upload;

//Your Domain name with URL path or IP address with path
const char* serverName = "";

ledDevice* downloadLeds[7];
ledDevice* uploadLeds[7];
int downloadLedsSize = 7;
int uploadLedsSize = 7;

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 1800000;
unsigned long retryDelay = 10000;
//30 minutes = 1800000

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void get_network_info() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[*] Network information for ");
    Serial.println(ssid);

    Serial.println("[+] BSSID : " + WiFi.BSSIDstr());
    Serial.print("[+] Gateway IP : ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("[+] Subnet Mask : ");
    Serial.println(WiFi.subnetMask());
    Serial.println((String) "[+] RSSI : " + WiFi.RSSI() + " dB");
    Serial.print("[+] ESP32 IP : ");
    Serial.println(WiFi.localIP());
  }
}

JsonVariant findNestedKey(JsonObject obj, const char* key) {
  JsonVariant foundObject = obj[key];
  if (!foundObject.isNull())
    return foundObject;

  for (JsonPair pair : obj) {
    JsonVariant nestedObject = findNestedKey(pair.value(), key);
    if (!nestedObject.isNull())
      return nestedObject;
  }

  return JsonVariant();
}

JsonVariantConst findNestedKeyVariant(JsonObjectConst obj, const char* key) {
  JsonVariantConst foundObject = obj[key];
  if (!foundObject.isNull())
    return foundObject;

  for (JsonPairConst pair : obj) {
    JsonVariantConst nestedObject = findNestedKeyVariant(pair.value(), key);
    if (!nestedObject.isNull())
      return nestedObject;
  }

  return JsonVariantConst();
}

void GetData(String input) {
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, input);

  if (error) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    for (int i = 0; i < downloadLedsSize; i++) {
      downloadLeds[i]->TurnLEDOff();
      //TurnLEDOff(downloadLeds[i]->pin);
    }
    for (int i = 0; i < uploadLedsSize; i++) {
      uploadLeds[i]->TurnLEDOff();
      //TurnLEDOff(uploadLeds[i]->pin);
    }
    delay(retryDelay);
    lastTime = 0;
    return;
  }

  const char* message = doc["message"];  //Get sensor type value
  Serial.print("message is: ");
  Serial.println(message);
  Serial.println();
  JsonObject data = doc["data"];  //get lightstates obj

  // for (JsonPair p : data) {
  //   const char* key = p.key().c_str();
  //   JsonVariant value = p.value();
  //   Serial.println(key);
  // }
  download = findNestedKey(doc.as<JsonObject>(), "download");
  Serial.println(download);
  SetDownload();
  upload = findNestedKey(doc.as<JsonObject>(), "upload");
  Serial.println(upload);
  SetUpload();
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);

  // If you need Node-RED/server authentication, insert user and password below
  //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");

  // Send HTTP POST request
  int httpResponseCode = http.GET();

  String payload = "{}";
  boolean error = false;
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    error = true;
  }
  // Free resources
  http.end();
  if (error == true) {
    for (int i = 0; i < downloadLedsSize; i++) {
      downloadLeds[i]->TurnLEDOff();
      //TurnLEDOff(downloadLeds[i]->pin);
    }
    for (int i = 0; i < uploadLedsSize; i++) {
      uploadLeds[i]->TurnLEDOff();
      //TurnLEDOff(uploadLeds[i]->pin);
    }
    delay(retryDelay);
    lastTime = 0;
  }
  return payload;
}

void TurnLEDOn(int ledPin) {
  digitalWrite(ledPin, HIGH);  // turn on the LED
}

void TurnLEDOff(int ledPin) {
  digitalWrite(ledPin, LOW);  // turn on the LED
}

void SetDownload() {
  for (int i = 0; i < downloadLedsSize; i++) {
    if (downloadLeds[i]->speed < download) {
      downloadLeds[i]->TurnLEDOn();
      //TurnLEDOn(downloadLeds[i]->pin);
    } else {
      //TurnLEDOff(downloadLeds[i]->pin);
      downloadLeds[i]->TurnLEDOff();
    }
  }
}

void SetUpload() {
  for (int i = 0; i < uploadLedsSize; i++) {
    if (uploadLeds[i]->speed < upload) {
      //TurnLEDOn(uploadLeds[i]->pin);
      uploadLeds[i]->TurnLEDOn();
    } else {
      //TurnLEDOff(uploadLeds[i]->pin);
      uploadLeds[i]->TurnLEDOff();
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  downloadLeds[0] = new ledDevice(18, 950, 0);
  downloadLeds[1] = new ledDevice(5, 800, 1);
  downloadLeds[2] = new ledDevice(4, 700, 2);
  downloadLeds[3] = new ledDevice(26, 600, 3);
  downloadLeds[4] = new ledDevice(27, 500, 4);
  downloadLeds[5] = new ledDevice(14, 400, 5);
  downloadLeds[6] = new ledDevice(12, 0, 6);

  uploadLeds[0] = new ledDevice(19, 100, 7);
  uploadLeds[1] = new ledDevice(21, 90, 8);
  uploadLeds[2] = new ledDevice(22, 80, 9);
  uploadLeds[3] = new ledDevice(23, 70, 10);
  uploadLeds[4] = new ledDevice(25, 60, 11);
  uploadLeds[5] = new ledDevice(33, 50, 12);
  uploadLeds[6] = new ledDevice(32, 0, 13);

  // downloadLeds[0] = new ledDevice(18, 950);
  // downloadLeds[1] = new ledDevice(5, 800);
  // downloadLeds[2] = new ledDevice(4, 700);
  // downloadLeds[3] = new ledDevice(26, 600);
  // downloadLeds[4] = new ledDevice(27, 500);
  // downloadLeds[5] = new ledDevice(14, 400);
  // downloadLeds[6] = new ledDevice(12, 0);

  // uploadLeds[0] = new ledDevice(19, 100);
  // uploadLeds[1] = new ledDevice(21, 90);
  // uploadLeds[2] = new ledDevice(22, 80);
  // uploadLeds[3] = new ledDevice(23, 70);
  // uploadLeds[4] = new ledDevice(25, 60);
  // uploadLeds[5] = new ledDevice(33, 50);
  // uploadLeds[6] = new ledDevice(32, 0);

  Serial.println();
  Serial.print("connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int index = 0;
  bool downloadBool = true;
  while (WiFi.status() != WL_CONNECTED) {
    for (int i = 0; i < downloadLedsSize; i++) {
      downloadLeds[i]->TurnLEDOff();
      //TurnLEDOff(downloadLeds[i]->pin);
    }
    for (int i = 0; i < uploadLedsSize; i++) {
      uploadLeds[i]->TurnLEDOff();
      //TurnLEDOff(uploadLeds[i]->pin);
    }
    if (downloadBool == true) {
      downloadLeds[index]->TurnLEDOn(50);
      //TurnLEDOn(downloadLeds[index]->pin);
    } else {
      uploadLeds[index]->TurnLEDOn(50);
      //TurnLEDOn(uploadLeds[index]->pin);
    }
    index++;
    if (downloadBool == true && index == downloadLedsSize) {
      index = 0;
      downloadBool = false;
    }

    if (downloadBool == false && index == uploadLedsSize) {
      index = 0;
      downloadBool = true;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  get_network_info();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  sensorReadings = httpGETRequest(serverName);
  Serial.println(sensorReadings);
  GetData(sensorReadings);
}

void loop() {
  //Send an HTTP POST request every 10 minutes
  unsigned long current = millis();

  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {

      sensorReadings = httpGETRequest(serverName);
      Serial.println(sensorReadings);
      GetData(sensorReadings);
    } else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}
