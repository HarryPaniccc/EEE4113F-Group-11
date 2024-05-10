/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-cam-take-photo-display-web-server/
  
  IMPORTANT!!! 
   - Select Board "AI Thinker ESP32-CAM"
   - GPIO 0 must be connected to GND to upload a sketch
   - After connecting GPIO 0 to GND, press the ESP32-CAM on-board RESET button to put your board in flashing mode
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

#include "WiFi.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "img_converters.h"
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <ESPAsyncWebServer.h>
#include <StringArray.h>
#include <SPIFFS.h>
// Calibrating the load cell
#include "HX711.h"

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 4;
const int LOADCELL_SCK_PIN = 3;
HX711 scale;

// // Replace with your network credentials
// const char* ssid = "CONNECT";
// const char* password = "spothot123";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

boolean takeNewPhoto = false;
int distance_value;

// Photo File Name to save in SPIFFS
#define FILE_PHOTO "/photo.jpg"

// OV2640 camera module pins (CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36 
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define TRIG_PIN 13
#define ECHO_PIN 12

# define DIST_TRIGGER 10

int max_value = 0;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { text-align:center; }
    .vert { margin-bottom: 10%; }
    .hori{ margin-bottom: 0%; }
  </style>
</head>
<body>
  <div id="container">
    <h2>ESP32-CAM Last Photo</h2>
    <p>It might take more than 5 seconds to capture a photo.</p>
    <p>
      <button onclick="rotatePhoto();">ROTATE</button>
      <button onclick="capturePhoto()">CAPTURE PHOTO</button>
      <button onclick="location.reload();">REFRESH PAGE</button>
    </p>
      <p id="distanceValue">Distance Value: </p> 

  </div>

  <div><img src="saved-photo" id="photo" width="70%"></div>
</body>
<script>
  var deg = 0;
  function capturePhoto() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', "/capture", true);
    xhr.send();
  }
  function rotatePhoto() {
    var img = document.getElementById("photo");
    deg += 90;
    if(isOdd(deg/90)){ document.getElementById("container").className = "vert"; }
    else{ document.getElementById("container").className = "hori"; }
    img.style.transform = "rotate(" + deg + "deg)";
  }
  function isOdd(n) { return Math.abs(n % 2) == 1; }

  // New function to get the variable value
  function getDistance() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("distanceValue").innerHTML = "Distance Value: " + this.responseText;
      }
    };
    xhr.open('GET', "/getDistance", true);
    xhr.send();
  }

  setInterval(getDistance, 500);

</script>
</html>)rawliteral";

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);



  const char* ssid = "WIN NOW!!! LIMITED OFFER!";
  const char* password = "12345678";
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  }
  else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }

  // Print ESP32 Local IP Address
  Serial.print("IP Address: http://");
  Serial.println(IP); // Use IP instead of WiFi.localIP()

  // Turn-off the 'brownout detector'
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // OV2640 camera module
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {

    // uncomment for better image quality
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
  });

  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest * request) {
    takeNewPhoto = true;
    request->send_P(200, "text/plain", "Taking Photo");
  });

  server.on("/saved-photo", HTTP_GET, [](AsyncWebServerRequest * request) {
    char image_file[32];
    sprintf(image_file, "/photo%d.jpg", max_value);
    request->send(SD_MMC, image_file, "image/jpg", false);
  });

  server.on("/getDistance", HTTP_GET, [](AsyncWebServerRequest * request) {
    char distance_str[32]; // Buffer to hold the string
    sprintf(distance_str, "%d", distance_value); // Convert the integer to a string
    request->send_P(200, "text/plain", distance_str);
  });

  // Start server
  server.begin();

  // pinMode(2, INPUT_PULLUP);
  initMicroSDCard();


  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

}


// Initialize the micro SD card
void initMicroSDCard(){
  // Start Micro SD card
  Serial.println("Starting SD Card");
  // bool begin(const char * mountpoint="/sdcard", bool mode1bit=true); // cf. SD_MMC.h
  if (!SD_MMC.begin("/sdcard", true))
  {
    Serial.println("SD Card Mount Failed");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD Card attached");
    return;
  }
  Serial.println("SD Card initialized");
}

void loop() {
    
  if (distance_value < DIST_TRIGGER) {

  // if (takeNewPhoto || distance_value < DIST_TRIGGER) {
    capturePhotoSaveSpiffs();
    measure_weight();

    // takeNewPhoto = false;

  } 
      // Send a 10 microsecond pulse to start the HC-SR04 sensor
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // Read the pulse from the HC-SR04 sensor, which is the time in microseconds
    // that it takes for the pulse to return to the sensor
    long duration = pulseIn(ECHO_PIN, HIGH);
    Serial.print("\nDistance: ");
    Serial.print(distance_value);

  // Check if pulseIn() returned 0 (no echo)
  if (duration != 0) {
    // Calculate the distance in cm
    distance_value = duration * 0.034 / 2;

    // print the distance in the Serial Monitor
    // Serial.print("\nDistance: ");
    // Serial.print(distance_value);
  }
  delay(1);
}

// Check if photo capture was successful
bool checkPhoto( fs::FS &fs ) {
  File f_pic = fs.open( FILE_PHOTO );
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}

// Capture Photo and Save it to SPIFFS
void capturePhotoSaveSpiffs( void ) {
  camera_fb_t * fb = NULL; // pointer
  // bool ok = 0; // Boolean indicating if the picture has been taken correctly

  // do {
    // Take a photo with the camera
    Serial.println("\nTaking a photo...");

    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }
    // -----------------------------------------------------------------------------------------------------------------
    // SAVE PHOTO INTO SPIFFS:

    // save name
    Serial.printf("Picture file name: %s\n", FILE_PHOTO);
    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);

    // Insert the data in the photo file
    if (!file) {
      Serial.println("Failed to open file in writing mode");
    }
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length
      Serial.print("The picture has been saved in ");
      Serial.print(FILE_PHOTO);
      Serial.print(" - Size: ");
      Serial.print(file.size());
      Serial.println(" bytes");
    }
    // ok = checkPhoto(SPIFFS);

    // Close the file
    file.close();

    // -----------------------------------------------------------------------------------------------------------------
    // SAVE PHOTO INTO MICROSD:

    // get the path of the photos
    char path[32];
    fs::FS &fs = SD_MMC; 

    if (max_value == 0) {
      sprintf(path, "/photo%d.jpg", getNextPhotoNumber(fs));
    } else {
      sprintf(path, "/photo%d.jpg", max_value + 1);
      max_value++;
    }
    
    Serial.printf("Picture file name: %s\n", path);

    // save the photo to microsd
    
    File sd_file = fs.open(path,FILE_WRITE);
    if(!sd_file){
      Serial.printf("Failed to open file in writing mode");
    } 
    else {
      sd_file.write(fb->buf, fb->len); // payload (image), payload length
      Serial.printf("Saved: %s\n", path);
    }
    sd_file.close();
    esp_camera_fb_return(fb); 

    // -----------------------------------------------------------------------------------------------------------------
    // APPEND THE DATA TO THE CSV FILE:
    char message[64]; // Buffer to hold the message
    sprintf(message, "\n%s,%d", path, distance_value);
    appendFile(fs, "/data.csv", message);


    // check if file has been correctly saved in SPIFFS
    
  // } while ( !ok );
}

// Take photo and save to microSD card
// void takeSavePhoto(){
//   // Take Picture with Camera
//   camera_fb_t * fb = esp_camera_fb_get();

//   if(!fb) {
//     Serial.println("Camera capture failed");
//     delay(1000);
//     ESP.restart();
//   }

//   // Path where new picture will be saved in SD Card
//   String path = getPhotoNumber();
//   Serial.printf("Picture file name: %s\n", path.c_str());

//   // Save picture to microSD card
//   fs::FS &fs = SD_MMC; 
//   File file = fs.open(path.c_str(),FILE_WRITE);
//   if(!file){
//     Serial.printf("Failed to open file in writing mode");
//   } 
//   else {
//     file.write(fb->buf, fb->len); // payload (image), payload length
//     Serial.printf("Saved: %s\n", path.c_str());
//   }
//   file.close();
//   esp_camera_fb_return(fb); 

//   // Save the data values associated with the capture to a CSV file
//   File dataFile = fs.open("/data.csv", FILE_APPEND);
//   if (!dataFile) {
//     Serial.println("Failed to open data file in append mode");
//   }
//   else {
//     dataFile.printf("%s, %d\n", path.c_str(), distance_value);
//     dataFile.close();
//   }
// }

// void saveFile()
// {
//     // Open the file from SPIFFS
//   File spiffsFile = SPIFFS.open(spiffsPath, FILE_READ);
//   if (!spiffsFile) {
//     Serial.printf("Failed to open file %s for reading", spiffsPath.c_str());
//     return;
//   }

//   // Path where new picture will be saved in SD Card
//   String sdPath = "/sd" + spiffsPath; // Change this to your desired SD card path
//   Serial.printf("SD card file path: %s\n", sdPath.c_str());

//   // Open a new file on the SD card
//   fs::FS &fs = SD_MMC; 
//   File sdFile = fs.open(sdPath.c_str(), FILE_WRITE);
//   if (!sdFile) {
//     Serial.printf("Failed to open file %s for writing", sdPath.c_str());
//     return;
//   }

//   // Read the contents of the file from SPIFFS and write them to the SD card
//   byte buf[512];
//   while (spiffsFile.available()) {
//     size_t len = spiffsFile.read(buf, sizeof(buf));
//     sdFile.write(buf, len);
//   }

//   Serial.printf("Copied file %s to %s\n", spiffsPath.c_str(), sdPath.c_str());

//   // Close the files
//   spiffsFile.close();
//   sdFile.close();
  
//   File file = SPIFFS.open("/data.csv", FILE_APPEND);
//   if (!file) {
//     Serial.println("There was an error opening the file for writing");
//     return;
//   }
// }

void appendFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

int getNextPhotoNumber(fs::FS &fs){


  const char * dirname = "/";

  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return -1;
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return -1;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      // comment out later
      // Serial.print("  DIR : ");
      // Serial.println(file.name());
      // if(levels){
      //   listDir(fs, file.name(), levels -1);
      // }
    } else {
      String filename = file.name();
      filename.replace("photo", ""); // Remove "photo" prefix
      filename.replace(".jpg", ""); // Remove ".jpg" suffix
      int value = filename.toInt(); // Convert the remaining string to an integer
      if (value > max_value) {
        max_value = value;
      }
      // comment out later
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
     file = root.openNextFile();
  }

  max_value++;
  return max_value;
}

void measure_weight()
{
    // if (scale.is_ready()) {
    // scale.set_scale();    
    // Serial.println("Tare... remove any weights from the scale.");
    // delay(10000);
    // scale.tare();
    // Serial.println("Tare done...");
    // Serial.print("Place a known weight on the scale...");
    // delay(10000);
    // long reading = scale.get_units(10);
    // Serial.print("Result: ");
    // Serial.println(reading);
    // } 
    // else {
    //   Serial.println("HX711 not found.");
    // }
    // delay(10000);
  

}
