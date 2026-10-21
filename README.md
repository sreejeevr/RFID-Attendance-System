# RFID Attendance System Using ESP32

An IoT-based RFID Attendance System developed as a mini project for the BCA program at Yenepoya (Deemed to be University), Bengaluru. The system uses an ESP32 microcontroller and an MFRC522 RFID reader to identify registered RFID cards and manage attendance records.

## Overview

The project demonstrates how RFID technology and the Internet of Things (IoT) can automate attendance tracking. When a student scans an RFID card, the ESP32 reads its unique identifier (UID), checks the registered student details, and displays the result using an LCD. A buzzer and LED provide additional feedback.

The system can also integrate with Google Sheets to store attendance records online.

## Features

- Contactless RFID card identification
- ESP32-based attendance processing
- LCD messages for user feedback
- Buzzer and LED status indicators
- Wi-Fi connectivity
- Google Sheets attendance integration
- Digital attendance record management

## Hardware Requirements

- ESP32 Development Board
- MFRC522 (RC522) RFID Reader
- RFID Cards or Tags
- 20×4 I2C LCD
- Buzzer
- LED
- Jumper Wires and Breadboard
- USB Cable or Power Supply

## Software and Technologies

- Arduino IDE
- Embedded C/C++
- ESP32
- MFRC522 RFID Library
- LiquidCrystal_I2C Library
- SPI and I2C Communication
- Wi-Fi
- Google Apps Script
- Google Sheets

## Working Principle

1. The ESP32 initializes the connected hardware modules.
2. The RFID reader waits for a card or tag.
3. The reader obtains the card's unique identifier.
4. The ESP32 checks the UID against registered student details.
5. The LCD displays the corresponding status.
6. The buzzer and LED provide feedback.
7. When cloud integration is configured, attendance information is sent to Google Sheets.


## Setup Instructions

1. Install Arduino IDE and ESP32 board support.
2. Install the required MFRC522 and LiquidCrystal_I2C libraries.
3. Connect the RFID reader, LCD, buzzer, and LED according to the actual circuit wiring.
4. Configure your Wi-Fi credentials and registered RFID UIDs.
5. Set up Google Apps Script and Google Sheets if cloud attendance recording is required.
5. Upload the program to the ESP32 and test it with registered and unregistered RFID cards.

**Important:** Verify all GPIO assignments against your hardware connections. Never publish Wi-Fi passwords or private credentials.

## Future Enhancements

- Web-based attendance dashboard
- Administrator authentication
- Duplicate attendance prevention
- Offline attendance storage and synchronization
- Attendance reports by date and semester
- Support for multiple classrooms

## Author

**Sreejeev R Chandran**

BCA – Artificial Intelligence, Machine Learning & Robotics  
Yenepoya (Deemed to be University), Bengaluru

## Acknowledgement

This project was developed as an academic mini project to demonstrate the practical application of IoT, RFID technology, and microcontroller-based automation.
