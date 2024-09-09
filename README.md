# Remote-Controlled Car Project

## Overview

This project features a remote-controlled car equipped with multiple driving modes and various sensor integrations. The car is controlled via a remote control with an OLED display that shows environmental data such as temperature, humidity, and light intensity. The car operates in three different modes: Joystick, Gesture, and Self-Drive.

## Features

### Remote Control

#### OLED Display

- **Sensor Data Display**: 
  - Shows temperature, humidity, and light intensity for 3 seconds when the "print" button is pressed (in Joystick and Gesture modes).
  - During this time, the car stops.
- **Driving Mode Display**:
  - Continuously displays the current driving mode.
  - Default mode is "Joystick mode".
- **Self-Drive Mode Display**:
  - Continuously displays recorded sensor data.
  - Updates every 3 seconds.
  - When an obstacle is detected, the distance from it is displayed for 3 seconds (without sensor data).

#### Buttons

- **print**: Displays recorded sensor data.
- **lights**: Toggles the car's headlights.
- **buzzer**: Sounds the car's horn while pressed.
- **joystick**: Activates Joystick mode.
- **gesture**: Activates Gesture mode.
- **self_drive**: Activates the car's autonomous driving mode (obstacle avoidance).

*Note: Print, lights, and buzzer buttons are only used in Joystick and Gesture modes.*

### Car

#### Driving Modes

- **Joystick Mode**: Controlled via joystick input.
- **Gesture Mode**: Controlled using gestures detected by the MPU6050 accelerometer (I2C protocol).
- **Self-Drive Mode**: 
  - Autonomous driving with obstacle avoidance.
  - Automatic sensor data display.
  - Automatic lighting control based on ambient light conditions.
  - When an obstacle is detected, the distance is displayed for 3 seconds.

#### Sensors

- **Ultrasonic Sensor (HC-SR04)**: Measures distance to obstacles for obstacle avoidance.
- **DHT11 Sensor**: Measures temperature and humidity.
- **Light Sensor**: Measures ambient light intensity for automatic lighting control and displaying.

### Software and Libraries

- **RF24.h**: For radio communication
- **Remote Control**:
  - `Wire.h`: I2C communication
  - `MPU6050.h`: Accelerometer library
  - `ezButton.h`: Button logic
  - `Adafruit_GFX.h` and `Adafruit_SSD1306.h`: OLED display
- **Car**:
  - `NewPing.h`: Ultrasonic sensor
  - `Servo.h`: Servo motor control
  - `SimpleDHT.h`: DHT11 sensor

### Algorithms and Functions

#### Movement

- **Motor Control**:
  - Motors are activated sequentially to create a smooth transition.
  - PWM (Pulse Width Modulation) is used for speed control.
  - Gradual speed increase reduces initial shocks and prevents mechanical stress.

#### Light Intensity Calculation

- **Calculation**:
  - Light intensity is calculated from the analog value read from the sensor.
  - The formula used is:
    ```plaintext
    lux = (R / -0.8616) ^ (1 / -0.8616) x 10 ^ (5.118 / -0.8616)
    ```
  - Where R is the calculated resistance of the sensor.

#### Distance Calculation

- **Formula**:
  - Speed of sound is calculated as:
    ```plaintext
    Speed = 331.4 + 0.6 × Temperature + 0.0124 × Humidity
    ```
  - Distance is determined based on the speed of sound and the time taken for the wave to travel.

#### Obstacle Avoidance

- **Self-Drive Mode**:
  - When a frontal obstacle is detected, the car stops, reverses, and rotates the servo-mounted proximity sensor.
  - It calculates distances to potential obstacles in different directions and chooses the direction with the most clearance.

### Power Supply

- **Components**:
  - Car components are powered by a 9V battery.
  - Motors use 6 batteries of 1.5V in series.
  - A voltage regulator and capacitors provide a stable 5V supply for the car components.
- **Resistors**:
  - 10K resistors are used as pull-down resistors for the remote control buttons.
  - A 220-ohm pull-up resistor is used with a 2N2222 transistor to control the car's headlights.
  - A 2.2K resistor is used with the photocell.

