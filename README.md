# 🌡️ Real-Time Precision Cooling System

### Arduino-Based Multi-Mode Fan Controller with PID Integration

---

## 🚀 Overview

This project implements a **smart temperature-controlled cooling system** using an Arduino.
It dynamically adjusts fan speed based on real-time temperature input using multiple control strategies.

Unlike conventional systems, this project integrates:

* Manual control
* Linear temperature scaling
* PID-based precision control
* Hybrid AUTO mode (adaptive control)

---

## ⚙️ Hardware Components

* Arduino Uno
* DS18B20 Temperature Sensor
* 16x2 LCD (I2C)
* Rotary Encoder with Push Button
* 12V DC Brushless Fan
* MOSFET Driver Circuit
* Breadboard + Jumper Wires

---

## 🧠 Control Modes

### 🔵 Manual Mode

User directly sets fan speed.

---

### 🟡 Linear Mode

Fan speed increases linearly between min and max temperature.

---

### 🔴 PID Mode (Core Feature)

This mode maintains a **target temperature** using feedback control.

#### 📐 PID Equation:

```
Output = Kp * error + Ki * integral + Kd * derivative
```

#### Where:

* **Error = Current Temp − Set Temp**
* **Kp (Proportional)** → reacts to current error
* **Ki (Integral)** → corrects accumulated error
* **Kd (Derivative)** → predicts future trend

#### 🔧 Implementation (from code)

```cpp
float error = temp - setT;

integral += error * dt;
integral = constrain(integral, -50, 50);

float derivative = (error - lastError) / dt;

float output = Kp * error + Ki * integral + Kd * derivative;
```

#### ⚙️ Tuned Values

```
Kp = 8.0
Ki = 0.3
Kd = 2.0
```

#### 💡 Behavior

* High error → strong cooling
* Near setpoint → fine control
* Stable temperature achieved

---

### 🟢 AUTO Mode (Hybrid Intelligence)

AUTO mode dynamically selects control strategy:

| Condition         | Behavior       |
| ----------------- | -------------- |
| Far from setpoint | Full speed     |
| Medium error      | Linear control |
| Near setpoint     | PID control    |

#### 🔧 Implementation

```cpp
if (error > 5) fanOut = 100;
else if (error > 2) fanOut = linear;
else fanOut = PID;
```

AUTO internally switches between:

* AUTO-M → Manual-like
* AUTO-L → Linear
* AUTO-P → PID

---

## 📊 Data Visualization

System outputs real-time data:

```
Temp:32.5, Fan:45, Mode:PID
```

This can be visualized using **Arduino Serial Plotter** to analyze:

* Temperature trends
* Fan response
* PID stability

---

## 🎛️ User Interface

* LCD displays:

  * Temperature
  * Mode
  * Fan Speed
* Rotary encoder allows:

  * Mode switching
  * Parameter editing

---

## 💎 Key Features

* Multi-mode adaptive control
* Real-time PID feedback system
* Hybrid AUTO intelligence
* Flicker-free LCD optimization
* Serial data visualization
* Low-cost embedded solution

---

## 🎯 Applications

* Smart cooling systems
* Electronics thermal management
* Environmental control
* Embedded systems learning

---

## 📌 Novelty

* Integration of multiple control strategies
* Hybrid AUTO mode combining PID + Linear
* Real-time interactive UI
* Embedded system optimization techniques

---

## 📷 Hardware Setup

<p align="center">
  <img src="https://github.com/user-attachments/assets/8f1ff4b0-32ee-4d6e-bea1-be094729bafc" width="45%" />
  <img src="https://github.com/user-attachments/assets/60aa5787-541e-425f-9e34-0cc42f8b7b6b" width="45%" />
</p>



---

## 🛠️ Code

Main implementation available in:

```
/code/sketch_apr3a.ino

```

---

