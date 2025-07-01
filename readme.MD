# Summary
This repository contains the code for a simple weight sensor that return how full a bottle (or any other container) is. It opens a web server that has a single endpoint that returns the percentage of how full a bottle is.
# Hardware

1. ESP32-WROOM-32D
2. DF9-16 Film Pressure Sensor 20g-2Kg 
3. 10K Ohm Resistor

# Schematic

The ESP uses a GPIO pin to read the analog value from the DF9-16 pressure sensor. One leg of the DF9-16 connects to the 3.3v while the other branch in two. The other leg has one direct connection to the GPIO pin (34) while the other goes through a 10K Ohm resistor to the ground. The schematic is shown below:


```
   ┌───────┐         ┌────┐
   │       │         │    │
 ┌─┤  DF9  ├─────────┤    │
 │ │       │         │    │
 │ └───────┘         │    │
3.3v        GND─10kΩ─┘    │
 │           │            │
┌┴───────────┴──┐         │
│               │         │
│    ESP-32     │         │
│               │         │
└──┬────────────┘         │
   34                     │
   │                      │
   └──────────────────────┘
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

# Code

The code is in the `main.ino`. Few points to note:

1. The serial is set to `115200` baud rate. You need to set Arduino to the same baud rate to see the output.
2. The GPIO pin used is `34`. If you want to change it, you can change the `sensorPin` constant.
3. The weight is gram is calculated using an heuristic, it is not accurate. For our case, it does not matter since we will register when the bottle is empty and full and then determine a range between 0-100% full.

# How it works

The DF9-16 adds resistance. The more weight, the more resistance. The circuit has two resistances: the constant 10K Ohm resistor and the variable resistance of the DF9-16. 

The analog pin, the GPIO pin 34, receives a smaller voltage than received initially (3.3v) because of the voltage divider effect. The more weight, the more resistance, and the less voltage is received by the GPIO pin. Thus, it receives a value from this formula:

```
Vout = Vin * (R2 / (R1 + R2))
```
Where `Vin` is the initial voltage (3.3v), `R1` is the constant 10K Ohm resistor, and `R2` is the dynamic resistance of the DF9-16 sensor.

The ESP32 receives the analog value with a value between 0-4095 that represent 0v to 3.3v. Zero means full presure (full resistance) and 4095 means no pressure (no resistance). The `minPressure` and `maxPressure` constants are used to determine the range of the pressure sensor and to calibrate by reading the serial minitor. 

The heuristic uses a linear interpolation to convert the analog value to gram.