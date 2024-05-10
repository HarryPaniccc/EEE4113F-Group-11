#include "HX711.h"

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 32;
const int LOADCELL_SCK_PIN = 12;

HX711 scale;

const int NUM_WEIGHTS = 3; // Number of different known weights
const int NUM_READINGS = 3; // Number of readings for each known weight

void setup() {
  Serial.begin(57600);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
}

void loop() {

  
  long total_calib_factor = 0;

  for (int i = 0; i < NUM_WEIGHTS; i++) {
    Serial.println("Type 'start' to begin:");
    String userInput;

    while (userInput != "start") {
      while (!Serial.available()) {
          // Wait for user input
      }
      userInput = Serial.readStringUntil('\n');
    }
    long total_reading = 0;

    Serial.print("Enter the known weight ");
    Serial.print(i + 1);
    Serial.println(":");
    float known_weight;

    while (!Serial.available()) {
      delay(5000);
    }
    known_weight = Serial.parseFloat();
    Serial.println("Weight = ");
    Serial.println(known_weight);

    for (int j = 0; j < NUM_READINGS; j++) {
      if (scale.is_ready()) {
        scale.set_scale();
        Serial.println("Tare: remove weights.");
        delay(10000);
        scale.tare();
        Serial.println("Tare done.");
        delay(1000); // Delay to stabilize reading
        Serial.print("Place the known weight ");
        Serial.println(i + 1);
        delay(5000);
        long reading = scale.get_units(10);
        Serial.print("Reading ");
        Serial.print(j + 1);
        Serial.print(" for weight ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(reading);
        total_reading += reading;
      } else {
        Serial.println("HX711 not found.");
      }
      delay(5000); // Delay between readings
    }

    float average_reading = total_reading / NUM_READINGS;
    float calib_factor = average_reading / known_weight;
    total_calib_factor += calib_factor;

    Serial.print("Average reading: ");
    Serial.print(average_reading);
    Serial.print("Calibration factor:");
    Serial.print(calib_factor);
  }

  float calibration_constant = total_calib_factor / NUM_WEIGHTS;

  Serial.print("Final calibration factor: ");
  Serial.println(calibration_constant);

  while (true) {
    // Stay in an infinite loop after calibration
  }
}
