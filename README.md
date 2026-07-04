# C4-Pulse-Intelligent-Plant-Health-Monitoring-System-
# Crop Guard: An IoT-Based Plant Health Monitoring System

Crop Guard is a real-time, smart agricultural diagnostic tool designed to evaluate crop water stress levels dynamically. By integrating environmental microclimate data with non-contact canopy temperature sensing, the system automates the acquisition, processing, and validation of the **Crop Water Stress Index (CWSI)**. The system translates raw environmental indicators into clear, actionable physical and visual feedback to optimize irrigation cycles and early disease detection.

---

## 💡 Abstract Idea
Crop Guard follows a structured, sequential workflow that transforms raw, real-time sensor data collected from target crops into human-identifiable visual outputs, providing precise insights into immediate plant health status. 

### Core Scientific Concepts
* **Leaf-Air Temperature Difference ($LAD$):** Represents the thermal variance between the plant canopy surface and its surrounding atmosphere ($LAD = T_{leaf} - T_{air}$). 
* **Vapour Pressure Deficit ($VPD$):** A measurement determining the drying power of the air based on its remaining moisture-carrying capacity, computed utilizing ambient air temperature and relative humidity.
* **Baseline Calibration ($NTB$ & $NWSB$):** Prior to calculating index values, the system computes target baseline references configured specifically for agricultural crops (tailored for distinct $C_3$/$C_4$ physiological traits):
    * **NTB (Non-Transpiring Baseline):** Represents the upper temperature bound ($\Delta T_{max}$) of a heavily stressed, non-transpiring plant canopy.
    * **NWSB (Non-Water-Stressed Baseline):** Represents the lower temperature bound ($\Delta T_{min}$) of a well-watered plant canopy experiencing optimal transpiration.

---

## 🔢 Engineering Formula & Logic
The execution core relies on calculating the relative position of the measured Leaf Surface Temperature ($LST$) within its upper and lower bounding limits:

$$\text{CWSI} = \frac{LST - NWSB}{NTB - NWSB}$$

### System Integrity Validation
To prevent anomalous false readings from distorting irrigation responses, the system includes an inline data integrity check:
* If the computed CWSI value registers outside realistic physiological boundaries ($< 0.0$ or $> 1.1$), it triggers an automatic flag designating a **Sensor Obstruction Error**.
* This attributes anomalous values to physical external interference factors (e.g., dust accumulation, birds blocking sensor line-of-sight, or loose connections).

---

## 📊 Plant Diagnostics Lookup Table

By stratifying calculated metrics into targeted mathematical ranges, the hardware response safely adapts dynamically to every expected state:

| Calculated CWSI | Mathematical Status / Diagnosis | System Action / Message Displayed | LED Status Indication |
| :---: | :--- | :--- | :--- |
| **$0.0 - 0.2$** | **Healthy** | "Plant is thriving." | 🟢 Green LED ON |
| **$0.3 - 0.5$** | **Transpiration Lag** | "Initial water stress." | 🟡 Yellow LED ON |
| **$0.6 - 0.8$** | **Sick (Vascular issues)** | "Biological fever detected." | 🔴 Red LED ON |
| **$0.9 - 1.0$** | **Critical State** | "Permanent wilting point." | 🔴 Red LED Flashing |
| **$> 1.1$** or **$< 0.0$** | **Sensor Obstruction** | "Check Sensor (Bird/Dust)" | All LEDs Flashing / Warning |

> *Note: The software logic captures a running average of 5 consecutive telemetry frames before cementing a critical state change output to minimize temporary environmental noise.*

---

## 🛠️ Hardware Simulation Schematic

The prototype architecture is implemented using an **Arduino UNO** microcontroller processing input modules communicating concurrently via a shared **I2C Serial Data Bus**.

### 📋 Full Bill of Materials (BOM) & Hardware Connections

| Component | Functionality | Primary Connection Point |
| :--- | :--- | :--- |
| **Arduino UNO** | Core Central Microcontroller Module | Main Host Controller |
| **BME280 Sensor** | Measures Ambient Air Temp & Relative Humidity | I2C Bus (`SDA`/`SCL` pins with 4.7kΩ Pull-Ups) |
| **MLX90614 / TC74** | Measures Non-Contact Infrared Leaf Temperature | I2C Bus (`SDA`/`SCL` shared interface) |
| **PCF8574** | Remote 8-Bit I/O Expander for I2C Bus | Interfaced between Arduino I2C and LCD Data pins |
| **LM016L LCD** | $16 \times 2$ Character State Display | Driven by PCF8574 outputs to save MCU Pins |
| **D2 (LED-GREEN)**| Shows Optimized Healthy Crop Condition | Driven via Current-Limiting Resistor via Expander/MCU |
| **D1 (LED-YELLOW)**| Shows Initial Warning / Incipient Water Stress | Driven via Current-Limiting Resistor via Expander/MCU |
| **D3 (LED-RED)** | Shows Critical / Biological Fever Warning | Driven via Current-Limiting Resistor via Expander/MCU |
| **Push Button** | Triggers Initial Field Reference Baseline Calibration| Hardware Digital Input Interrupt Pin |

### Proteus 8 Simulation Schematic Capture
Below is the circuit wireframe mapping layout configured inside Proteus for tracking signal execution profiles:

![](YOUR_IMAGE_LINK_HERE_e.g.,_schematic.png)

### Simulation Calibration Logic Workaround
In field deployments, an on-board **Calibration Push Button** samples a baseline healthy control plant to calibrate contextual coefficients relative to changing day-night intervals. 

Because Proteus evaluates immediate hardware logic cycles rather than long-term diurnal field shifts, the simulation environment is pre-configured to initialize direct water-stressed parameters immediately upon startup. This purposefully proves the downstream integrity warning checks, register evaluations, and automated diagnostic switching arrays function perfectly under stress limits.

---

## 🚀 How to Run the Files

### 💻 1. Simulating in Proteus
1. Download the file named `Crop_Guard_Simulation.pdsprj` from this repository.
2. Open the file inside **Proteus 8 Professional**.
3. Compile the Arduino sketch file (`Crop_Guard.ino`) inside the Arduino IDE to export your compiled `.hex` machine-code file (`Sketch -> Export Compiled Binary`).
4. Double-click the Arduino Uno module component inside your Proteus schematic design space, select the folder icon next to the **Program File** field, and upload the generated `.hex` binary file.
5. Click the lower-left **Play** button to begin real-time hardware execution tracing.

### 🔌 2. Physical Deployment
1. Connect the sensors according to the `Full Bill of Materials` connection matrix utilizing standard I2C physical pull-up configurations.
2. Upload the `Crop_Guard.ino` sketch directly into your physical Arduino Uno unit via a USB interface.
3. Open your hardware serial monitor workspace at `9600 baud` rate to inspect mathematical outputs concurrently alongside the physical LCD visual alerts.
