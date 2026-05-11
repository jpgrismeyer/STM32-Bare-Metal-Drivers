# STM32L4 I2C Master Implementation - LPS22HB Sensor

This project implements a low-level I2C driver for the **STM32L475** to communicate with the **LPS22HB** sensor using repeated start conditions.

## Hardware Configuration

| Function    | Pin  | Mode              | Note                          |
| :---------- | :--- | :---------------- | :---------------------------- |
| **I2C2_SCL**| PB10 | Alternate Function| Open-Drain                    |
| **I2C2_SDA**| PB11 | Alternate Function| Open-Drain                    |
| **USER LED**| PB14 | Output            | **Turns ON when ID matches** |
| **BUTTON** | PC13 | Input             | User trigger (Active Low)     |

## Key Features
* **Repeated Start:** Used during the `WHO_AM_I` register read sequence.
* **Visual Feedback:** The LED on **PB14** provides immediate confirmation of a successful I2C transaction (ID `0xB1`).
* **Debounced Input:** Button logic prevents multiple triggers during a single press.

## Operation
1. Initialize GPIOs, I2C2, and the PB14 LED.
2. Press the **PC13** button to trigger a sensor read.
3. The driver sends a write request for register `0x0F` followed by a repeated start for reading.
4. If the received value is `0xB1`, the **PB14 LED** is toggled or turned ON.

## Debugging with Logic Analyzer
For low-cost analyzers (Saleae clones):
* **Drivers:** Use **Zadig** to install the `WinUSB` driver.
* **Software:** Use **PulseView (sigrok)** for decoded I2C protocol analysis.
* **Probing:** Attach **CH0** to SCL (PB10) and **CH1** to SDA (PB11). Ensure a common **GND**.