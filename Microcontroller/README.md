# ESP32 CAM Starling Weighing System

This repository contains a `.ino` file developed for the ESP32 CAM to weigh starlings at UCT.

## Overview

The system is designed to:

1. Sense when a bird lands on the scale using the ultrasonic sensor.
2. Measure the weight of the bird.
3. Take a photo of the bird.
4. Save the images and data to a CSV file on an SD card.
5. Upload this data to a web server.

The algorithmic flow of the system is illustrated in `Algorithmic Flow Diagram.drawio.png`


## Schematic Diagram

The schematic used for this system (with slight alterations) is found in `Schematic Diagram.drawio.png`

## Usage

To use this file with the Arduino IDE and upload it to an ESP32 CAM board, follow these steps:

1. Download and install the [Arduino IDE](https://www.arduino.cc/en/Main/Software).
2. Open the `.ino` file in the Arduino IDE.
3. Connect your ESP32 CAM board to your computer via a USB cable.
4. In the Arduino IDE, select the correct board and port under the "Tools" menu.
5. Click the "Upload" button to upload the code to your ESP32 CAM board.

Please ensure that the ESP32 CAM board is properly connected and powered during this process.