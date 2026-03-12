# Smart-Home-Ecosystem---ESP-32-Android-Control
This project represents a complete home automation system, integrating ESP32-based hardware with a native mobile application developed in Kotlin. The system allows real-time monitoring of environmental parameters and control of electrical devices via Firebase Realtime Database.

<div align="center">

# 🏠 SmartHome Ecosystem
### ESP32 & Android Integrated IoT Solution

[![Firebase](https://img.shields.io/badge/Firebase-Realtime_Database-ffca28?style=for-the-badge&logo=firebase&logoColor=white)](https://firebase.google.com/)
[![Android](https://img.shields.io/badge/Android-Kotlin-3ddc84?style=for-the-badge&logo=android&logoColor=white)](https://kotlinlang.org/)
[![ESP32](https://img.shields.io/badge/Hardware-ESP32-e67e22?style=for-the-badge&logo=espressif&logoColor=white)](https://www.espressif.com/)

---

<p align="left">
A sophisticated home automation system that bridges the gap between environmental monitoring and smart control. This project utilizes an <b>ESP32</b> microcontroller to fetch data from a <b>Bosch BME680</b> sensor, syncing everything in real-time with a custom <b>Kotlin-based Android application</b>.
</p>



</div>

## 🌟 Key Features

### 📱 Android Application
- **Dynamic Dashboard:** Real-time data visualization with instant updates.
- **UI Customization:** Users can toggle visibility for specific sensor widgets (Temp, Humidity, AQI, Power).
- **Intelligent Feedback:** The UI dynamically changes colors based on the **AQI (Air Quality Index)** levels.
- **Smart Control:** Remote toggles for lighting and appliance simulation.
- **Adaptive Design:** Full support for **Dark Mode** and responsive grid layouts.

### 🔌 Hardware & Firmware
- **BME680 Integration:** Precise measurement of Temperature, Humidity, and VOC Gases.
- **Advanced Calibration:** Custom software offsets for sensor self-heating and dynamic gas resistance mapping.
- **I2C Communication:** Efficient data transfer between the sensor and ESP32.

---

### Hardware Setup
<p align="center">
  <img src="screenshots/montaj_hardware1.jpg" width="600" title="Circuit Pictures">
  <img src="screenshots/montaj_hardware2.jpg" width="600" title="Circuit Pictures">
</p>

---

## 📊 Air Quality Index (AQI) Logic
The system interprets raw sensor data and categorizes it based on international standards:

| Score | Quality | UI Color |
| :--- | :--- | :--- |
| **0 - 50** | Excellent | 🟢 Green |
| **51 - 100** | Good | 🟢 Light Green |
| **101 - 150** | Lightly Polluted | 🟡 Yellow |
| **151 - 200** | Moderately Polluted | 🟠 Orange |
| **201 - 300** | Heavily Polluted | 🔴 Red |
| **301 - 500** | Severely Polluted | 🟣 Purple |

---

## 📸 App Screenshots

<table border="0">
  <tr>
    <td>
      <p align="center"><b>Dashboard</b></p>
      <img src="screenshots/app_dashboard.jpg" width="250">
    </td>
    <td>
      <p align="center"><b>Settings</b></p>
      <img src="screenshots/app_settings.jpg" width="250">
    </td>
    <td>
      <p align="center"><b>Customization</b></p>
      <img src="screenshots/app_custom.jpg" width="250">
    </td>
  </tr>
</table>

---

## 🛠️ Technical Stack
- **Languages:** Kotlin (Android), C++ (ESP32 Firmware).
- **Backend:** Google Firebase Realtime Database.
- **Hardware:** ESP32 DevKit V1, Bosch BME680.
- **Communication:** I2C Protocol, HTTPS/WSS via Firebase SDK.



---

## 🔧 How to Setup

1. **Firebase:** Add your `google-services.json` to the `/app` folder.
2. **Firmware:** Update the `WiFi_SSID`, `WiFi_PASS`, and `FIREBASE_URL` in the ESP32 code.
3. **Android Studio:** Build and run the project on an API 24+ device.

---

<div align="center">
  <sub>Developed for License Thesis Project • 2024</sub>
</div>
