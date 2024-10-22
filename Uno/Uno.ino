#include <AFMotor.h>
#include <SoftwareSerial.h>

AF_DCMotor motor1(1);
AF_DCMotor motor2(2);
AF_DCMotor motor3(3);
AF_DCMotor motor4(4);

AF_DCMotor* motors[] = { &motor1, &motor2, &motor3, &motor4 };

SoftwareSerial espSerial(2, 3);

void setup() {
  Serial.begin(115200);        
  espSerial.begin(115200);     
  Serial.println("Arduino ready to receive commands.");

  for (int i = 0; i < 4; i++) {
    motors[i]->setSpeed(125);
  }
}

void loop() {
  if (espSerial.available() > 0) {

    char command = espSerial.read();

    switch (command) {
      case 'F':  
        Serial.println("Command: Forward");
        driveForward();
        break;

      case 'B': 
        Serial.println("Command: Backward");
        driveBackward();
        break;

      case 'L':  
        Serial.println("Command: Left");
        turnLeft();
        break;

      case 'R':  
        Serial.println("Command: Right");
        turnRight();
        break;

      case 'S':  
        Serial.println("Command: Stop");
        stopAllMotors();
        break;

      default:
        stopAllMotors();
        Serial.println("Unknown command! Motors stopped.");
        break;
    }
  }
}

void driveForward() {
  for (int i = 0; i < 4; i++) {
    motors[i]->run(FORWARD);
  }
}

void driveBackward() {
  for (int i = 0; i < 4; i++) {
    motors[i]->run(BACKWARD);
  }
}

void turnLeft() {
  motors[0]->run(BACKWARD);
  motors[1]->run(BACKWARD);
  motors[2]->run(FORWARD);
  motors[3]->run(FORWARD);
}

void turnRight() {
  motors[0]->run(FORWARD);
  motors[1]->run(FORWARD);
  motors[2]->run(BACKWARD);
  motors[3]->run(BACKWARD);
}

void stopAllMotors() {
  for (int i = 0; i < 4; i++) {
    motors[i]->run(RELEASE);
  }
}
