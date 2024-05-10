#include "HX711.h"

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 32;
const int LOADCELL_SCK_PIN = 12;

HX711 scale;
const int NUM_READINGS = 10; // Number of readings for each known weight

void setup() {
  Serial.begin(57600);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
}

void loop() {

  
  long calib_factor = 1045;

  Serial.println("Type 'start' to begin:");
    String userInput;

    while (userInput != "start") {
      while (!Serial.available()) {
          // Wait for user input
      }
      userInput = Serial.readStringUntil('\n');
    }
    long total_weight = 0;

    for (int j = 0; j < NUM_READINGS; j++) {
      if (scale.is_ready()) {
        scale.set_scale();
        Serial.println("Remove weights.");
        delay(1000);
        scale.tare();
        Serial.println("Tare done.");
        delay(1000); // Delay to stabilize reading
        Serial.println("Place the object.");
        delay(5000);
        float reading = scale.get_units(10);
        float weight = reading/calib_factor;
        Serial.print("Weight ");
        Serial.print(j + 1);
        Serial.print(": ");
        Serial.println(weight, 1);
        total_weight += weight;
      } else {
        Serial.println("HX711 not found.");
      }
      delay(5000); // Delay between readings
    }

    float average = total_weight / NUM_READINGS;

    Serial.print("Average reading: ");
    Serial.print(average);

  while (true) {
    // Stay in an infinite loop after calibration
  }
}
