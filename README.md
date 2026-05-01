# Sitting Posture Monitoring Belt

## Introduction

This project implements the embedded software for a wearable sitting posture monitoring belt based on an STM32 microcontroller. The system reads motion data from four MPU6050 inertial measurement units (IMUs), processes the sensor data to estimate body posture, and uses a deployed NanoEdge AI machine learning model to classify the posture as either **Good Posture** or **Bad Posture**.

The software is designed to run on an STM32 board using STM32CubeIDE. Sensor data is collected through the I2C interface, processed into Euler angle features, passed into the NanoEdge AI classification model, and the classification result is transmitted through USART2 for Bluetooth communication.

---

## Contextual Overview

The software forms the main embedded control and classification layer of the wearable posture monitoring system.

```text
4 × MPU6050 Sensors
        |
        | I2C via TCA9548A multiplexer
        |
STM32 Microcontroller
        |
        | Sensor reading + Euler angle calculation
        |
Kalman filter
        |
        | 12 input features:
        | Roll, Pitch, Yaw × 4 sensors
        |
NanoEdge AI Classification Model
        |
        | Good Posture / Bad Posture
        |
USART2 Bluetooth Output

The four MPU6050 sensors are connected to the STM32 through an I2C multiplexer. Each sensor provides accelerometer and gyroscope data. The software converts these readings into physical values, estimates roll, pitch, and yaw angles, and then uses these twelve values as the input to the NanoEdge AI model.

Installation Instructions
Required Software

The following software is required:

STM32CubeMX
STM32CubeIDE
NanoEdge AI Studio
GitHub Desktop, optional for repository upload
A serial terminal or Bluetooth serial monitor
Required Hardware
STM32 development board
4 × MPU6050 IMU sensors
TCA9548A I2C multiplexer
Bluetooth module connected to USART2
Jumper wires and power supply
Project Setup
Open STM32CubeMX.
Configure the STM32 board pins and peripherals:
Enable I2C for MPU6050 communication.
Enable USART2 for Bluetooth output.
Enable USART1 if additional serial debugging is needed.
Configure GPIO pins if external indicators are used.
Generate the project for STM32CubeIDE.
Open the generated project in STM32CubeIDE.
Add the NanoEdge AI files to the project:
NanoEdgeAI.h
libneai.a
Add the NanoEdge AI include path in STM32CubeIDE:
Project Properties → C/C++ Build → Settings → MCU GCC Compiler → Include paths
Add the NanoEdge AI library path:
Project Properties → C/C++ Build → Settings → MCU GCC Linker → Libraries
Build the project.
How to Run the Software
Connect the four MPU6050 sensors to the TCA9548A I2C multiplexer.
Connect the multiplexer to the STM32 I2C pins.
Connect the Bluetooth module to USART2.
Flash the program to the STM32 board using STM32CubeIDE.
Open a Bluetooth serial monitor or serial terminal.
Reset the STM32 board.
The system will initialise the MPU6050 sensors and the NanoEdge AI model.
During operation, the software continuously classifies the sitting posture and sends one of the following outputs through USART2:
Good Posture

or

Bad Posture

The same output is also sent through USART1 if enabled in the _write() function.

Technical Details
Sensor Reading

The software reads accelerometer and gyroscope data from four MPU6050 sensors. Since all MPU6050 sensors use the same I2C address, a TCA9548A I2C multiplexer is used to select each sensor channel individually.

The MPU6050 raw data is converted into physical values using:

ax_g = Ax_raw * ACCEL_RESOLUTION;
ay_g = Ay_raw * ACCEL_RESOLUTION;
az_g = Az_raw * ACCEL_RESOLUTION;

gx_dps = Gx_raw * GYRO_RESOLUTION;
gy_dps = Gy_raw * GYRO_RESOLUTION;
gz_dps = Gz_raw * GYRO_RESOLUTION;
Euler Angle Calculation

The accelerometer data is used to estimate roll and pitch:

roll = atan2(ay, az)

pitch = -atan2(ax, sqrt(ay^2 + az^2))

The gyroscope data is integrated over time to estimate angular changes. A Kalman filter is then used to combine accelerometer-based and gyroscope-based angle estimates.

NanoEdge AI Input

The final input to the NanoEdge AI model contains 12 features:

Sensor 1: roll, pitch, yaw
Sensor 2: roll, pitch, yaw
Sensor 3: roll, pitch, yaw
Sensor 4: roll, pitch, yaw

These values are stored in:

float neai_input[12];

The classification function is called as:

neai_classification(neai_input, output_class_buffer, &class_id);

The model has two output classes:

class_id = 0 → Good Posture
class_id = 1 → Bad Posture
Output

The classification result is transmitted using printf(). The _write() function redirects the output to USART2 and USART1:

HAL_UART_Transmit(&huart2, ...);
HAL_UART_Transmit(&huart1, ...);

USART2 is used as the Bluetooth output channel.

Known Issues and Future Improvements
The current system only classifies posture into two classes: Good Posture and Bad Posture.
Classification accuracy depends on the quality and variety of the training data used in NanoEdge AI Studio.
Yaw angle estimation may drift over time because the MPU6050 does not include a magnetometer.
Sensor placement on the belt must remain consistent with the placement used during data collection.
Future improvements could include:
adding more posture classes,
improving calibration,
adding real-time vibration or buzzer feedback,
logging posture data to a mobile app,
using a more advanced IMU with magnetometer support.
