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

## 🗺️ Functional Architecture (System Flowchart)

The structural workflow below outlines data acquisition, algorithmic calculation, range validation checks, and corresponding output routines:
