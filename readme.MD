# Summary
This repository contains the code for a simple weight sensor that return how full a bottle (or any other container) is. The first ESP, sender, is the one collecting the water quantity. A second ESP, the receiver, listen for the sender and get the data to display it. Both ESP communicates using ESP-NOW for fast and low-power communication.

# Hardware

## Sender
1. ESP32-WROOM-32D
1. DF9-16 Film Pressure Sensor 20g-2Kg 
1. 10K Ohm Resistor

## Receiver
1. ESP32-WROOM-32D
1. MP1584EN Mini Adjustable Buck Converter
1. 3 AAA batteries holder
1. [Waveshare 4.2inch E-Ink](https://www.waveshare.com/wiki/File:4.2inch-e-paper-b-specification.pdf)
1. 470 µF capacitor

# Schematic For the ESP (Sender)

The ESP uses a GPIO pin to read the analog value from the DF9-16 pressure sensor. One leg of the DF9-16 connects to the 3.3v while the other branch in two. The other leg has one direct connection to the GPIO pin (34) while the other goes through a 10K Ohm resistor to the ground. The schematic is shown below:

The sender 

```
┌────────────────┐
│                │
│     3x AAA     │
│ +           -  │
└┬────────────┬──┘
┌┴────────────┴───┐
│ IN+       IN-   │
│    MP1584EN     │
│ Out+      Out-  │
└┬─────────────┬──┘
 │             │
 ├── +470µF- ──┤    // The capacitor is in a T shape between the MP1584EN and the ESP
 │(long)(short)│
┌┘             └──────────────┐ 
│                             │
│   ┌───────┐         ┌────┐  │
│   │       │         │    │  │
│ ┌─┤  DF9  ├─────────┤    │  │
│ │ │       │         │    │  │
│ │ └───────┘         │    │  │
│3.3v        GND─10kΩ─┘    │  │
│ │           │            │  │
└─┴─┐         │            │  │
  ┌─┴─────────┴───┐        │  │
  │ 3.3V     GND  │        │  │
  │     ESP-32    │        │  │
  │ GND   34      │        │  │
  └──┬────┬───────┘        │  │
     │    │                │  │
     │    │                │  │
     │    └────────────────┘  │
     └────────────────────────┘
```

# Schematic For the ESP (Receiver)

```
      Power via USB
           │
┌──────────┴───────────────┐
│                          │
│         ESP-32           │
│                          │
│ 3.3     GND     22    21 │
└┬─────────┬───────┬─────┬─┘
┌┴─────────┴───────┴─────┴─┐
│ VCC     GND     SCL  SDA │
│                          │
│        Waveshare         │
│                          │
└──────────────────────────┘
```

# Installation

The code in this repository is coded for the ESP32-WROOM-32D module and requires the following software and drivers to be installed:

1. Get the Arduino IDE from https://www.arduino.cc/en/software
2. Install the ESP32 board support package in the Arduino IDE by adding the following URL to the Additional Board Manager URLs in the preferences:

    ```
    https://dl.espressif.com/dl/package_esp32_index.json
    ```
3. In the Windows Device Manager. If the `Ports > Silicon Labs CP210x USB to UART Bridge (COMx)` is not installed, download and install the driver from https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
4. Select the board ESP32-WROOM-DA Module
5. You can plug both ESP with two differents USB cable. They will have different port.
6. Install in Arduino under Tools > Manage Libraries the following libraries:
   - `ESP-NOW` (comes with the ESP32 board support package)
   - `WiFi` (comes with the ESP32 board support package)
   - `ArduinoJson`
   - [GxEPD2](https://github.com/ZinggJM/GxEPD2)

# Code

The code is in the `sender.ino` and `receiver.ino`. Few points to note:

1. The serial is set to `115200` baud rate. You need to set Arduino to the same baud rate to see the output.
2. The GPIO pin used is `34`. If you want to change it, you can change the `sensorPin` constant.
3. The weight is gram is calculated using an heuristic, it is not accurate. For our case, it does not matter since we will register when the bottle is empty and full and then determine a range between 0-100% full.
4. The sender needs to have the MAC address of the receiver. You can find it in the serial monitor of the receiver. The MAC address is printed when the ESP32 starts.

# How it works

The DF9-16 adds resistance. The more weight, the more resistance. The circuit has two resistances: the constant 10K Ohm resistor and the variable resistance of the DF9-16. 

The analog pin, the GPIO pin 34, receives a smaller voltage than received initially (3.3v) because of the voltage divider effect. The more weight, the more resistance, and the less voltage is received by the GPIO pin. Thus, it receives a value from this formula:

```
Vout = Vin * (R2 / (R1 + R2))
```
Where `Vin` is the initial voltage (3.3v), `R1` is the constant 10K Ohm resistor, and `R2` is the dynamic resistance of the DF9-16 sensor.

The ESP32 receives the analog value with a value between 0-4095 that represent 0v to 3.3v. Zero means full presure (full resistance) and 4095 means no pressure (no resistance). The `minPressure` and `maxPressure` constants are used to determine the range of the pressure sensor and to calibrate by reading the serial minitor. 

The heuristic uses a linear interpolation to convert the analog value to gram.

To save power, the ESP32 goes to deep sleep mode after reading the weight. The ESP sends the information to another device using ESP now.

# Battery
The sender uses 3 AA batteries in series (1.5v x 3 = 4.5v). The MP1584EN buck converter converts the voltage down to 3.3v for the ESP32. 

```
1000 mAh @ 1.5 V (each)
1.5 V × 1000 mAh = 1.5 Wh
4.5 V × 1 Ah = 4.5 Wh
ESP active energy (runs for 2 seconds):  160 mA × 3.3 V × (2/3600 h) ≈ 0.293 mWh
ESP sleeping energy (30 minutes):  0.015 mA × 3.3 V × 0.4994 h ≈ 0.025 mWh
Energy = 0.293 + 0.025 = ~0.318 mWh
4.5 Wh / 0.318 mWh = ~14,150 cycles
Buck converter efficiency is 85%: 4.5 Wh × 0.85 = ~3.825 Wh
3.825 Wh / 0.318 mWh ≈ ~12,025 cycles
12,025 × 0.5 h = ~6012 hours = ~250 days
```

Note that changing from 30 minutes to 1h to capture the water level would increase the battery life to about 15 months.