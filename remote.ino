// REMOTE

// I2C
#include <Wire.h>

// RADIO LIBRARIES
#include <RF24.h>
#include <RF24_config.h>
#include <nRF24L01.h>
#include <printf.h>

// MPU LIBRARY
#include <MPU6050.h>

// OLED LIBRARIES
#include <Adafruit_GFX.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>
#include <Adafruit_SSD1306.h>
#include <splash.h>

// BUTTONS LIBRARY
#include <ezButton.h>

// Buttons pins definitions
#define BUZZER_BUTTON_PIN 2
#define LIGHTS_BUTTON_PIN 3
#define PRINT_BUTTON_PIN 4
#define SELF_DRIVE_BUTTON_PIN 5
#define GESTURE_BUTTON_PIN 6
#define JOYSTICK_BUTTON_PIN 7

// Radio pins
#define CE_PIN 8
#define CSN_PIN 9

// Pins definitions
#define X_AXIS_PIN A1
#define Y_AXIS_PIN A0

// Display init
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Radio init and adresses
RF24 radio(CE_PIN, CSN_PIN);  // CE, CSN
const byte address[][6] = { "00001", "00002" };
char receivedData[32] = "";

// States init
bool print_state = false;
bool buzzer_state = false;


unsigned long printPressedTime = 0;     // Time to manage print state
unsigned long lastDataRequestTime = 0;  // Time to manage data requests
unsigned long selfDriveTime = 0;        // Time to manage timer in self drive
bool buzzer = false;


// Sensor data
int t = 0, h = 0, l = 0, distance = 0;
;
// Init buttons
ezButton printButton(PRINT_BUTTON_PIN);
ezButton buzzerButton(BUZZER_BUTTON_PIN);
ezButton joystickButton(JOYSTICK_BUTTON_PIN);
ezButton gestureButton(GESTURE_BUTTON_PIN);
ezButton selfdriveButton(SELF_DRIVE_BUTTON_PIN);
ezButton lightsButton(LIGHTS_BUTTON_PIN);

// Mpu init
MPU6050 accelgyro;

String xyData;
int xAxis, yAxis;

// Driving mods
enum Mode { JOYSTICK_MODE,
            GESTURE_MODE,
            SELF_DRIVE_MODE };

Mode currentMode = JOYSTICK_MODE;

unsigned long distanceDisplayStartTime = 0;
bool displayDistance = false;

void displayMode() {
  if (!print_state) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    if (currentMode == GESTURE_MODE) {
      display.println("   Gesture Mode");
      display.display();
    } else if (currentMode == JOYSTICK_MODE) {
      display.println("   Joystick Mode");
      display.display();
    } else if (currentMode == SELF_DRIVE_MODE) {
      unsigned long currTime = millis();

      if (currTime - selfDriveTime >= 120000 && buzzer == false) {
        display.clearDisplay();
        display.println("        02:00");
        display.display();
        radio.stopListening();
        radio.write("self_drive", sizeof("self_drive"));
        delay(100);
        display.clearDisplay();
        buzzer = true;
      } else if (currTime - selfDriveTime >= 60000 && currTime - selfDriveTime < 120000) {
        display.clearDisplay();
        display.println("        01:00");
        display.display();
      } else if (currTime - selfDriveTime >= 0 && currTime - selfDriveTime < 60000) {
        display.clearDisplay();
        display.println("        00:00");
        display.display();
      }

      if (buzzer) {
        radio.startListening();

        // Check for received data
        if (radio.available()) {
          radio.read(&receivedData, sizeof(receivedData));
          if (strncmp(receivedData, "data:", 5) == 0) {
            sscanf(receivedData, "data:%d,%d,%d", &t, &h, &l);
          } else if (strncmp(receivedData, "distance:", 8) == 0) {
            sscanf(receivedData, "distance:%d", &distance);
            display.clearDisplay();
            display.println("   Self Drive Mode");
            display.print("Obstacol distance: ");
            display.print(distance);
            display.println(" cm");
            display.display();
            delay(3000);
          }
        }


        display.println("   Self Drive Mode");

        display.print("Temperature: ");
        display.print(t);
        display.println(" C");
        display.print("Humidity: ");
        display.print(h);
        display.println(" %");
        display.print("Light: ");
        display.print(l);
        display.println(" lux");
        display.display();
      }
    }

    radio.stopListening();
  }
}

void joystick() {
  int xAxis = analogRead(X_AXIS_PIN);
  int yAxis = analogRead(Y_AXIS_PIN);

  xyData = String(xAxis) + ";" + String(yAxis);

  radio.write(xyData.c_str(), xyData.length());

  delay(20);
}

void accel() {
  int16_t ax, ay, az;
  int16_t gx, gy, gz;
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  xyData = String(ax) + ";" + String(ay);
  if (!print_state && !buzzer_state)
    radio.write(xyData.c_str(), xyData.length());

  delay(20);
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display.clearDisplay();

  radio.begin();
  radio.openWritingPipe(address[0]);     // 00001
  radio.openReadingPipe(1, address[1]);  // 00002
  radio.setPALevel(RF24_PA_MIN);

  // Init buttons ezButton
  printButton.setDebounceTime(50);
  buzzerButton.setDebounceTime(50);
  gestureButton.setDebounceTime(50);
  joystickButton.setDebounceTime(50);
  selfdriveButton.setDebounceTime(50);
  lightsButton.setDebounceTime(50);

  accelgyro.initialize();

  // Verify connection
  Serial.println("Testing device connections...");
  Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

  radio.stopListening();
}

void loop() {
  // verify button states
  printButton.loop();
  buzzerButton.loop();
  gestureButton.loop();
  joystickButton.loop();
  selfdriveButton.loop();
  lightsButton.loop();

  // Buttons logic

  // verify state of print button
  if (printButton.isPressed() && (millis() - printPressedTime) > 3000 && !buzzer_state) {
    printPressedTime = millis();
    print_state = true;

    radio.stopListening();
    radio.write("data_request", sizeof("data_request"));
    radio.startListening();
    delay(100);
  } else if (joystickButton.isPressed()) {
    currentMode = JOYSTICK_MODE;
    buzzer = false;
    radio.write("joystick", sizeof("joystick"));
    delay(100);
  } else if (gestureButton.isPressed()) {
    currentMode = GESTURE_MODE;
    buzzer = false;
    radio.write("gesture", sizeof("gesture"));
    delay(100);


  } else if (selfdriveButton.isPressed()) {
    currentMode = SELF_DRIVE_MODE;
    selfDriveTime = millis();
  } else if (buzzerButton.isReleased()) {
    buzzer_state = false;
    radio.write("stop_buzz", sizeof("stop_buzz"));
    delay(100);
  } else if (buzzerButton.isPressed()) {
    buzzer_state = true;
    radio.write("buzz", sizeof("buzz"));
    delay(100);
  } else if (lightsButton.isPressed()) {
    radio.write("lights", sizeof("lights"));
    delay(100);
  }

  if (print_state) {
    if (radio.available()) {
      radio.read(&receivedData, sizeof(receivedData));
      if (strncmp(receivedData, "data:", 5) == 0) {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor(0, 0);

        display.println("   Sensors Data");
        sscanf(receivedData, "data:%d,%d,%d", &t, &h, &l);
        display.print("Temperature: ");
        display.print(t);
        display.println(" C");
        display.print("Humidity: ");
        display.print(h);
        display.println(" %");
        display.print("Light: ");
        display.print(l);
        display.println(" lux");
        display.display();
        delay(3000);
        display.clearDisplay();
        print_state = false;
      }
    }

    // reset state of print button after 3 seconds]
    if (millis() - printPressedTime >= 3000) {
      display.clearDisplay();
      display.display();
      print_state = false;
      radio.stopListening();
    }
  }

  // checking and managing the state of buttons according to the current mode
  if (!print_state && !buzzer_state)
    switch (currentMode) {
      case JOYSTICK_MODE:
        joystick();
        break;

      case GESTURE_MODE:
        accel();
        break;
    }
  displayMode();
}
