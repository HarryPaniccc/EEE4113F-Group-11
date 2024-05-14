#define BLYNK_TEMPLATE_ID "TMPL2o36WEwZo"
#define BLYNK_TEMPLATE_NAME "Starling Spy"
#define BLYNK_AUTH_TOKEN "z6cZg4iCPlcfMbUl5z9txs-rgiPJ_pkD"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <FS.h>
#include "time.h"
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <floatToString.h>
#include "HX711.h"

// HX711 circuit wiring
//const int LOADCELL_DOUT_PIN = 32;
//const int LOADCELL_SCK_PIN = 12;

float total_weight = 0;
float weight;
//#define BLYNK_PRINT Serial
char auth[] = BLYNK_AUTH_TOKEN;

char ssid[] = "Realme GT Master Edition";
char pass[] = "Shaihan0720";
WebServer server(80);
//HX711 scale;
//File dataFile;
const int NUM_READINGS = 10;
String str = "";

const char* ntp_Server = "pool.ntp.org";
const long UTC_Offset = 3600;
const int DST_offset = 0;

String birdID = "";
String bID = "";
String ringType = "";
String rType = "";

String page = "<html>\
<head>\
  <title> Starling Spy Data Download Page</title>\
  <style>\
    body {\
      background-color: #305a36;\
      font-family: Arial;\
      color: #000000;\
    }\
    .container {\
      text-align: center;\
      background-color: rgb(189, 253, 253);\
      padding: 20px;\
      margin: 20%;\
      border-radius: 25px;\
    }\
    .download-button {\
      background-color: #4CAF50;\
      border: none;\
      color: white;\
      padding: 15px 32px;\
      text-align: center;\
      text-decoration: none;\
      display: inline-block;\
      font-size: 16px;\
      margin-top: 10px;\
      cursor: pointer;\
      border-radius: 12px;\
    }\
  </style>\
</head>\
<body>\
  <div class=\"container\">\
    <form method=\"post\" action=\"download\">\
      <input type=\"submit\" class=\"download-button\" value=\"Download Spy Data\">\
    </form>\
  </div>\
</body>\
</html>";

String createResponse() {
  return page;
}

void RootResponse() {
  server.send(200, "text/html", createResponse());
}

void notFound() {
  String response = "File not found\n";
  response += "URI:" + server.uri() + "\nMethod:" + (server.method() == HTTP_GET ? "GET" : "POST") + "\n";
  server.send(404, "text/plain", response);
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  Blynk.begin(auth, ssid, pass);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("*");
  }
  Serial.println();
  Serial.println("Connected...");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());

  MDNS.begin("esp8266");

  server.on("/", RootResponse);
  /*server.on("/download", updateFunction);
  server.onNotFound(notFound);
  server.begin();
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialization failed");
    return;
  }*/
}

String getTime() {
  configTime(UTC_Offset, DST_offset, ntp_Server);
  struct tm time_data;
  if (!getLocalTime(&time_data)) {
    Serial.println("Failure to obtain time");
    return "";
  }
  String time_current = String(time_data.tm_year + 1900) + "/" + String(time_data.tm_mon + 1) + "/" + String(time_data.tm_mday) + "\t" + String(time_data.tm_hour) + ":" + String(time_data.tm_min) + ":" + String(time_data.tm_sec);
  return time_current;
}

BLYNK_WRITE(V9) {
  birdID = param.asStr();
}

BLYNK_WRITE(V10) {
  ringType = param.asStr();
}

String saveWeightToCSV(float weight) {
  // Get current timestamp
  String timestamp = getTime();
  String floatStr = String(weight, 1);
  //Serial.println("Weight value is: \n");
  //Serial.println(floatStr);
  return birdID+"\t"+ringType+"\t"+timestamp + "\t" + floatStr;
}

void setStr(String str_out) {
  str = str_out;
}

void updateFunction() {
  File dataFile;
  SPIFFS.remove("/weight_data");
  dataFile = SPIFFS.open("/weight_data", "w");
  dataFile.println(str);
  dataFile.close();
  dataFile = SPIFFS.open("/weight_data", "r");
  server.sendHeader("Content-Type", "text/text");
  server.sendHeader("Content-Disposition", "attachment; filename=weight_data.txt");
  server.sendHeader("Connection", "close");
  server.streamFile(dataFile, "text/text");
  dataFile.close();
}

void captureWeight() {
  long calib_factor = 1045;
  weight = 0;
  total_weight = 0;
  for (int j = 0; j < NUM_READINGS; j++) {
    if (scale.is_ready()) {
    scale.set_scale();
    Serial.println("Remove weights.");
    delay(1000);
    scale.tare();
    Serial.println("Tare done.");
    delay(1000);  // Delay to stabilize reading
    Serial.println("Place the object.");
    delay(5000);
    float reading = scale.get_units(10);
    //float reading = 10045;
    weight = reading / calib_factor;
    Serial.print("Weight ");
    Serial.print(j + 1);
    Serial.print(": ");
    Serial.println(weight, 1);
    Blynk.virtualWrite(V0, total_weight);
    total_weight += weight;
    //} else {
    //  Serial.println("HX711 not found.");
    //}
    str += saveWeightToCSV(weight) + "\n";
    delay(2500);  // Delay between readings
  }

  float average = total_weight / NUM_READINGS;

  Serial.print("Average reading: ");
  Serial.print(average);
  Serial.print("\n");
  setStr(str);
  server.on("/download", updateFunction);
  server.onNotFound(notFound);
  server.begin();
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialization failed");
    return;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  Blynk.run();
  server.handleClient();
}

BLYNK_WRITE(V7) {
  if (param.asInt() == 1) {
    captureWeight();
  }
  else if (param.asInt()==0){
    Blynk.virtualWrite(V0, 0);
  }
}