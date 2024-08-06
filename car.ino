// CAR

// MUSIC LIBRARY 
#include <pitches.h>

// ULTRASONIC SENSOR LIBRARY
//#include <NewPing.h>


//SERVOMOTOR LIBRARY
#include <Servo.h>

// RADIO LIBRARIES
#include <RF24.h>
#include <RF24_config.h>
#include <nRF24L01.h>
#include <printf.h>

// TEMPERATURE SENSOR LIBRARY
#include <SimpleDHT.h>

// Pins definitions
#define LIGHT_PIN A0
#define ECHO_PIN A1
#define TRIG_PIN A2

// #define BUZZER_PIN A3
#define BUZZER_PIN 7

#define LIGHTS_PIN A4
#define TEMP_PIN A5

// Radio pins
#define CE_PIN 8
#define CSN_PIN 9

// Bridge pins definitions
#define enA 2
#define in1 3
#define in2 4
#define in3 5
#define in4 6
//#define enB 7
#define enB A3

// Init ultrasonic sensor
//#define MAX_DISTANCE 200

//NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

// Init servomotor
Servo servomotor;

// Init temperature sensor
SimpleDHT11 dht11;

// Radio init and adresses
RF24 radio(CE_PIN, CSN_PIN);  // CE, CSN
const byte address[][6] = { "00001", "00002" };
char receivedData[32] = "";

// Init states
bool print_state = false;
bool buzzer_state = false;
bool lights_state = false;
bool obstacle_state = false;

// Axis variables
int xAxis;
int yAxis;
String dataString;

// DHT11 variables
byte temperature = 0;
byte humidity = 0;


unsigned long lastSensorUpdateTime = 0;  // time for sending data (self-drive)
unsigned long lastObstacleTime = 0;
unsigned long selfDriveTime = 0; // Time to manage timer in self drive


int melody[] = {
  NOTE_G4, NOTE_C4, NOTE_DS4, NOTE_F4, NOTE_G4, NOTE_C4, NOTE_DS4, NOTE_F4,
  NOTE_G4, NOTE_C4, NOTE_DS4, NOTE_F4, NOTE_G4, NOTE_C4, NOTE_DS4, NOTE_F4,
  NOTE_G4, NOTE_C4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_C4, NOTE_E4, NOTE_F4,
  NOTE_G4, NOTE_C4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_C4, NOTE_E4, NOTE_F4,
  NOTE_G4, NOTE_C4,
  
  NOTE_DS4, NOTE_F4, NOTE_G4, NOTE_C4, NOTE_DS4, NOTE_F4,
  NOTE_D4,
  NOTE_F4, NOTE_AS3,
  NOTE_DS4, NOTE_D4, NOTE_F4, NOTE_AS3,
  NOTE_DS4, NOTE_D4, NOTE_C4,
  
  NOTE_G4, NOTE_C4,
  
  NOTE_DS4, NOTE_F4, NOTE_G4, NOTE_C4, NOTE_DS4, NOTE_F4,
  NOTE_D4,
  NOTE_F4, NOTE_AS3,
  NOTE_DS4, NOTE_D4, NOTE_F4, NOTE_AS3,
  NOTE_DS4, NOTE_D4, NOTE_C4,
  NOTE_G4, NOTE_C4,
  NOTE_DS4, NOTE_F4, NOTE_G4,  NOTE_C4, NOTE_DS4, NOTE_F4,
  
  NOTE_D4,
  NOTE_F4, NOTE_AS3,
  NOTE_D4, NOTE_DS4, NOTE_D4, NOTE_AS3,
  NOTE_C4,
  NOTE_C5,
  NOTE_AS4,
  NOTE_C4,
  NOTE_G4,
  NOTE_DS4,
  NOTE_DS4, NOTE_F4,
  NOTE_G4,
  
  NOTE_C5,
  NOTE_AS4,
  NOTE_C4,
  NOTE_G4,
  NOTE_DS4,
  NOTE_DS4, NOTE_D4,
  NOTE_C5, NOTE_G4, NOTE_GS4, NOTE_AS4, NOTE_C5, NOTE_G4, NOTE_GS4, NOTE_AS4,
  NOTE_C5, NOTE_G4, NOTE_GS4, NOTE_AS4, NOTE_C5, NOTE_G4, NOTE_GS4, NOTE_AS4,
  
  REST, NOTE_GS5, NOTE_AS5, NOTE_C6, NOTE_G5, NOTE_GS5, NOTE_AS5,
  NOTE_C6, NOTE_G5, NOTE_GS5, NOTE_AS5, NOTE_C6, NOTE_G5, NOTE_GS5, NOTE_AS5
};

int durations[] = {
  8, 8, 16, 16, 8, 8, 16, 16,
  8, 8, 16, 16, 8, 8, 16, 16,
  8, 8, 16, 16, 8, 8, 16, 16,
  8, 8, 16, 16, 8, 8, 16, 16,
  4, 4,
  
  16, 16, 4, 4, 16, 16,
  1,
  4, 4,
  16, 16, 4, 4,
  16, 16, 1,
  
  4, 4,
  
  16, 16, 4, 4, 16, 16,
  1,
  4, 4,
  16, 16, 4, 4,
  16, 16, 1,
  4, 4,
  16, 16, 4, 4, 16, 16,
  
  2,
  4, 4,
  8, 8, 8, 8,
  1,
  2,
  2,
  2,
  2,
  2,
  4, 4,
  1,
  
  2,
  2,
  2,
  2,
  2,
  4, 4,
  8, 8, 16, 16, 8, 8, 16, 16,
  8, 8, 16, 16, 8, 8, 16, 16,
  
  4, 16, 16, 8, 8, 16, 16,
  8, 16, 16, 16, 8, 8, 16, 16
};

void playMelody() {
  int size = sizeof(durations) / sizeof(int);

  while (true ) {
    for (int note = 0; note < size; note++) {
      int duration = 1000 / durations[note];
      tone(BUZZER_PIN, melody[note], duration);

      int pauseBetweenNotes = duration * 1.30;
      delay(pauseBetweenNotes);

      noTone(BUZZER_PIN);
    }
  }
}

// Driving mods
enum Mode { JOYSTICK_MODE,
            GESTURE_MODE,
            SELF_DRIVE_MODE };

Mode currentMode = JOYSTICK_MODE;



// Data reading functions
String getSensorData() {
  // read temperature and humidity
  dht11.read(TEMP_PIN, &temperature, &humidity, NULL);

  // calculate intensity
  float lux = calculateLux(analogRead(LIGHT_PIN));

  // Construim mesajul de date
  String sensorDataMsg = "data:" + String((int)temperature) + "," + String((int)humidity) + "," + String(int(lux));

  return sensorDataMsg;
}

// calculate intensity of light
float calculateLux(int analogValue) {
  float vout = analogValue / 204.6;
  float R = (11000 - vout * 2200) / vout;                              // Calculate the resistance
  float lux = (pow(R, (1 / -0.8616))) / (pow(10, (5.118 / -0.8616)));  // Lux calculation
  return lux;
}

// Moving functions
void moveStop() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}

void moveForward() {
  for (int i = 0; i < 20; i++) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    digitalWrite(in3, LOW);
    digitalWrite(in4, LOW);
    delay(1);

    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
    delay(1);


    analogWrite(enA, 150);
    analogWrite(enB, 150);
  }

  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);

  analogWrite(enA, 150);
  analogWrite(enB, 150);
}

void moveBackward() {
  for (int i = 0; i < 20; i++) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    digitalWrite(in3, LOW);
    digitalWrite(in4, LOW);
    delay(1);

    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
    delay(1);

    analogWrite(enA, 150);
    analogWrite(enB, 150);
  }

  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);

  analogWrite(enA, 150);
  analogWrite(enB, 150);
}

void turnRight() {
  for (int i = 0; i < 20; i++) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    digitalWrite(in3, LOW);
    digitalWrite(in4, LOW);
    delay(1);
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    digitalWrite(in3, LOW);
    digitalWrite(in4, LOW);
    delay(1);

    analogWrite(enA, 150);
    analogWrite(enB, 150);
  }

  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);

  analogWrite(enA, 150);
  analogWrite(enB, 150);
}

void turnLeft() {
  for (int i = 0; i < 20; i++) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
    delay(1);
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    digitalWrite(in3, LOW);
    digitalWrite(in4, LOW);
    delay(1);

    analogWrite(enA, 150);
    analogWrite(enB, 150);
  }

  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);

  analogWrite(enA, 150);
  analogWrite(enB, 150);
}

// distance to object
// int calculateDistance() {
//   dht11.read(TEMP_PIN, &temperature, &humidity, NULL);

//   // calculate distance
//   float speedOfSound = 331.4 + (0.6 * temperature) + (0.0124 * humidity);  // m/s
//   float duration = sonar.ping_median(10);                                  // microseconds
//   duration = duration / 1000000;                                           // seconds
//   float distance = (speedOfSound * duration) / 2;
//   distance = distance * 100;  // cm
//   Serial.println(distance);

//   return (int)distance;
// }

// Function to handle self-drive logic
void self_drive() {
  
  playMelody();
  // int distance = calculateDistance();

  // if (distance <= 30) {

  //   unsigned long currentMillis = millis();
  //   if (currentMillis - lastObstacleTime >= 3000) {
  //     lastObstacleTime = currentMillis;
  //     // Send distance of obstacle
  //     String distance_msg = "distance:" + String((int)distance);

  //     Serial.println("distance");
  //     Serial.println(distance);
  //     radio.stopListening();
  //     radio.write(distance_msg.c_str(), distance_msg.length());
  //     delay(20);
  //     radio.startListening();
  //   }

  //   moveStop();
  //   delay(500);
  //   moveBackward();
  //   delay(200);
  //   moveStop();
  //   delay(500);

  //   // Rotate servo to left
  //   servomotor.write(180);
  //   delay(200);

  //   // Read left side distance using ultrasonic sensor
  //   int distanceL = calculateDistance();

  //   // Bring servo to center
  //   servomotor.write(90);
  //   delay(200);


  //   // Rotate servo to right
  //   servomotor.write(0);
  //   delay(200);

  //   // Read right side distance using ultrasonic sensor
  //   int distanceR = calculateDistance();

  //   // Bring servo to center
  //   servomotor.write(90);
  //   delay(200);

  //   if (distanceL == 0) {
  //     turnRight();
  //     delay(200);
  //   } else if (distanceR == 0) {
  //     turnLeft();
  //     delay(200);
  //   } else if (distanceL >= distanceR) {
  //     turnLeft();
  //     delay(200);
  //   } else {
  //     turnRight();
  //     delay(200);
  //   }
  //   moveStop();
  //   delay(200);
  // } else {
  //   moveForward();
  // }

  // // Light control
  // float lux = calculateLux(analogRead(LIGHT_PIN));

  // if (lux < 30 && !lights_state) {
  //   digitalWrite(LIGHTS_PIN, HIGH);
  //   lights_state = true;
  // }
  // if (lux >= 30 && lights_state) {
  //   digitalWrite(LIGHTS_PIN, LOW);
  //   lights_state = false;
  // }

  // // Send sensor data every 3 seconds
  // unsigned long currentMillis = millis();
  // if (currentMillis - lastSensorUpdateTime >= 3000) {
  //   lastSensorUpdateTime = currentMillis;

  //   Serial.println(currentMillis);

  //   String sensorData = getSensorData();

  //   Serial.println(sensorData);

  //   radio.stopListening();
  //   radio.write(sensorData.c_str(), sensorData.length());
  //   radio.startListening();
  // }
}

void joystick() {
  if (!buzzer_state && !print_state) {
    if (radio.available()) {
      radio.read(&receivedData, sizeof(receivedData));

      // Unpack data into X and Y axis values
      String dataString = String(receivedData);
      int splitIndex = dataString.indexOf(';');
      String xAxisStr = dataString.substring(0, splitIndex);
      String yAxisStr = dataString.substring(splitIndex + 1);

      xAxis = xAxisStr.toInt();  // Convertim șirul în valoare întreagă pentru axa X
      yAxis = yAxisStr.toInt();  // Convertim șirul în valoare întreagă pentru axa Y

      delay(100);

      // Motor control based on coordinates
      if (yAxis < 600) {
        moveForward();
      } else if (yAxis > 700) {
        moveBackward();
      } else if (xAxis < 600) {
        turnLeft();
      } else if (xAxis > 700) {
        turnRight();
      } else {
        moveStop();
      }
    }
  } else moveStop();
}

void accel() {
  if (!buzzer_state && !print_state) {
    if (radio.available()) {
      radio.read(&receivedData, sizeof(receivedData));

      // Despachetăm datele în valorile pentru axa X și axa Y
      String dataString = String(receivedData);
      int splitIndex = dataString.indexOf(';');
      String xAxisStr = dataString.substring(0, splitIndex);
      String yAxisStr = dataString.substring(splitIndex + 1);

      xAxis = xAxisStr.toInt();  // Convertim șirul în valoare întreagă pentru axa X
      yAxis = yAxisStr.toInt();  // Convertim șirul în valoare întreagă pentru axa Y

      delay(100);

      // Controlul motorului în funcție de coordonate
      if (yAxis <= -4000) {
        moveForward();
      } else if (yAxis >= 4000) {
        moveBackward();
      } else if (xAxis <= -4000) {
        turnRight();
      } else if (xAxis >= 4000) {
        turnLeft();
      } else {
        moveStop();
      }
    }
  } else moveStop();
}

void setup() {
  Serial.begin(9600);

  // Servo init
  servomotor.attach(10);
  servomotor.write(90);

  // Motor pins
  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  // Radio
  radio.begin();
  radio.openWritingPipe(address[1]);     // 00002
  radio.openReadingPipe(1, address[0]);  // 00001
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();

  // Init pins
  pinMode(LIGHT_PIN, INPUT);
  pinMode(TEMP_PIN, INPUT);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LIGHTS_PIN, OUTPUT);

  // Initial states of pins
  digitalWrite(BUZZER_PIN, HIGH);
  digitalWrite(LIGHTS_PIN, LOW);

  moveStop();
}

void loop() {
  if (radio.available()) {
    radio.read(&receivedData, sizeof(receivedData));

    if (strncmp(receivedData, "joystick", 8) == 0) {
      currentMode = JOYSTICK_MODE;
      delay(100);
    } else if (strncmp(receivedData, "gesture", 7) == 0) {
      currentMode = GESTURE_MODE;
      delay(100);
    } else if (strncmp(receivedData, "self_drive", 10) == 0) {
      currentMode = SELF_DRIVE_MODE;
      delay(100);
    } else if (strncmp(receivedData, "buzz", 4) == 0) {
      buzzer_state = true;
      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
    } else if (strncmp(receivedData, "stop_buzz", 9) == 0) {
      buzzer_state = false;
      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);
    } else if (strncmp(receivedData, "lights", 6) == 0) {
      if (lights_state == true) {
        analogWrite(LIGHTS_PIN, 1024);
        lights_state = false;
      } else {
        analogWrite(LIGHTS_PIN, 0);
        lights_state = true;
      }
      delay(100);
    } else if (strncmp(receivedData, "data_request", 12) == 0) {
      print_state = true;

      radio.stopListening();

      String sensorData = getSensorData();

      radio.write(sensorData.c_str(), sensorData.length());

      delay(3000);

      print_state = false;
      radio.startListening();
    }
  }

  if (!buzzer_state && !print_state)
    switch (currentMode) {
      case JOYSTICK_MODE:
        joystick();
        break;
      case GESTURE_MODE:
        accel();
        break;
      case SELF_DRIVE_MODE:
        self_drive();
        break;
    }
}
