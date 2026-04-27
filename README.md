# Multi-Temperature Quail Egg Hatcher (IoT-Enabled)

An automated, high-precision quail egg incubator designed for hobbyists. This project features a 3-phase climate control system and IoT integration via Telegram for remote monitoring and control.

## 🚀 Overview
The **Multi-Temperature Quail Egg Hatcher** optimizes the hatching process by automating temperature and humidity regulation. Unlike traditional incubators that require manual adjustments, this system uses pre-set profiles tailored to the different phases of quail egg development, significantly increasing the hatching success rate.

## ✨ Key Features
- **3-Phase Climate Profiling:** Automatically adjusts temperature and humidity settings based on the specific hatching phase.
- **IoT Integration (Telegram Bot):** - Real-time monitoring of temperature and humidity.
  - Status updates for the heater and humidifier.
  - Remote lighting control (On/Off).
  - Hatching start date tracking.
- **Precision Thermal Regulation:** Utilizes an N-Channel MOSFET (K4107) and Kanthal wire for efficient, space-saving heating.
- **Automated Humidification:** Ultrasonic water misting system to maintain optimal moisture levels.

## 🛠️ Technical Specifications
### Hardware
- **Microcontroller:** Arduino-compatible board with Wi-Fi capability.
- **Sensors:** DHT11 (Temperature & Humidity).
- **Heating Element:** Kanthal wire + DC Exhaust Fan.
- **Switching Component:** K4107 N-Channel MOSFET (High-current handling).
- **Power Supply:** 19V 3.2A.
- **Casing:** Custom-built Perspex (Acrylic).
- **Capacity:** Up to 12 quail eggs.

### Software
- **Programming Language:** C++ (Arduino IDE).
- **IoT Protocol:** Telegram Bot API for real-time telemetry.

## 📐 System Design
The system works by constantly polling the DHT11 sensor. 
1. **Temperature Control:** If the temperature falls below the phase profile, the K4107 MOSFET triggers the Kanthal wire heater. The DC fan ensures even heat distribution.
2. **Humidity Control:** If humidity is low, the ultrasonic humidifier creates fine water droplets to increase moisture. It stops automatically once the threshold is reached.

> <img width="940" height="529" alt="image" src="https://github.com/user-attachments/assets/bb0e9bde-ddef-4537-bfdd-0f360902f58f" />
[![Tajuk Video](https://img.youtube.com/vi/lFxzeUiik-c/0.jpg)](https://www.youtube.com/watch?v=lFxzeUiik-c)

## 📊 Results and Discussion
The project successfully hatched quail eggs within approximately **18 days**. 
- **Stability:** The PID-like control maintained temperature and humidity within the appropriate range throughout all 3 phases.
- **Connectivity:** The Telegram bot provided seamless data transmission without errors, allowing for 24/7 remote monitoring.

> <img width="444" height="333" alt="image" src="https://github.com/user-attachments/assets/318c3c23-9a2e-4e04-aa40-922fec9ef579" />

## 👥 Authors
- **Muhammad Hafizin bin Saibol Jahar** - *Main Developer* ([muhammadhafizin78@gmail.com](mailto:muhammadhafizin78@gmail.com))
- **Mr. Ts. Wan Rumaizi bin Wan Taib** - *Project Supervisor* ([rumaizi@pis.edu.my](mailto:rumaizi@pis.edu.my))

## 🙏 Acknowledgements
Special thanks to **Politeknik Ibrahim Sultan (PIS)** and our families for the continuous support throughout the development of this project.

---
*Keywords: Kanthal, MOSFET, Perspex, Incubator, IoT, Arduino*
