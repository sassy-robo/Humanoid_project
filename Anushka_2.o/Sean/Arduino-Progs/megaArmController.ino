#include <Servo.h>

/*
  Flash one copy to the left-arm Mega and one copy to the right-arm Mega.
  The Python runtime sends commands in these formats:

  ARM:a1,a2,a3,a4      a1 shoulder rotate, a2 shoulder lift, a3 elbow, a4 wrist
  HAND:f1,f2,f3,f4,f5
  SPECIAL:JAADUTONA
  HOME                 arm back to rest (0,90,90,90)

  Shoulder lift and elbow each use two servos mounted as mirror images:
  servo A gets the angle, servo B gets 180 - angle. So 4 ARM values drive
  6 arm servos. Replies "OK <command>" and "POSE:..." for Serial Monitor.
*/

// A servos (and shoulder rotate, wrist) on the original pins.
const int ARM_SERVO_PINS[4] = {2, 3, 4, 5};
const int FINGER_SERVO_PINS[5] = {6, 7, 8, 9, 10};
// Mirror partners for shoulder lift (index 1) and elbow (index 2).
const int LIFT_B_PIN = 11;
const int ELBOW_B_PIN = 12;

// Per-servo trim in degrees, for horns mounted a little off:
// shoulder rotate, lift A, elbow A, wrist, lift B, elbow B.
const int TRIM[6] = {0, 0, 0, 0, 0, 0};

const int ARM_REST[4] = {0, 90, 90, 90};

Servo armServos[4];
Servo liftB;
Servo elbowB;
Servo fingerServos[5];

int currentArm[4] = {0, 90, 90, 90};
int currentFingers[5] = {90, 90, 90, 90, 90};

String incoming;

int nextValue(String &payload, int &cursor) {
  int comma = payload.indexOf(',', cursor);
  String token;
  if (comma == -1) {
    token = payload.substring(cursor);
    cursor = payload.length();
  } else {
    token = payload.substring(cursor, comma);
    cursor = comma + 1;
  }
  token.trim();
  return constrain(token.toInt(), 0, 180);
}

void writeArm(int i, int angle) {
  armServos[i].write(constrain(angle + TRIM[i], 0, 180));
  if (i == 1) liftB.write(constrain(180 - angle + TRIM[4], 0, 180));
  if (i == 2) elbowB.write(constrain(180 - angle + TRIM[5], 0, 180));
}

void moveArmSmooth(const int *targets, int delayMs) {
  bool changed = true;
  while (changed) {
    changed = false;
    for (int i = 0; i < 4; i++) {
      if (currentArm[i] != targets[i]) {
        currentArm[i] += currentArm[i] < targets[i] ? 1 : -1;
        writeArm(i, currentArm[i]);
        changed = true;
      }
    }
    delay(delayMs);
  }
}

void moveSmooth(Servo *servos, int *currentValues, int *targets, int count, int delayMs) {
  bool changed = true;
  while (changed) {
    changed = false;
    for (int i = 0; i < count; i++) {
      if (currentValues[i] < targets[i]) {
        currentValues[i]++;
        servos[i].write(currentValues[i]);
        changed = true;
      } else if (currentValues[i] > targets[i]) {
        currentValues[i]--;
        servos[i].write(currentValues[i]);
        changed = true;
      }
    }
    delay(delayMs);
  }
}

void handleArm(String payload) {
  int targets[4];
  int cursor = 0;
  for (int i = 0; i < 4; i++) {
    targets[i] = nextValue(payload, cursor);
  }
  moveArmSmooth(targets, 12);
}

void handleHand(String payload) {
  int targets[5];
  int cursor = 0;
  for (int i = 0; i < 5; i++) {
    targets[i] = nextValue(payload, cursor);
  }
  moveSmooth(fingerServos, currentFingers, targets, 5, 12);
}

void doJaaduTona() {
  int spellA[5] = {170, 90, 55, 55, 55};
  int spellB[5] = {120, 40, 130, 40, 130};
  moveSmooth(fingerServos, currentFingers, spellA, 5, 10);
  delay(250);
  moveSmooth(fingerServos, currentFingers, spellB, 5, 10);
  delay(250);
  moveSmooth(fingerServos, currentFingers, spellA, 5, 10);
}

void printPose() {
  // shoulderRot, liftA, liftB, elbowA, elbowB, wrist | fingers
  Serial.print("POSE:");
  Serial.print(currentArm[0]); Serial.print(',');
  Serial.print(currentArm[1]); Serial.print(',');
  Serial.print(180 - currentArm[1]); Serial.print(',');
  Serial.print(currentArm[2]); Serial.print(',');
  Serial.print(180 - currentArm[2]); Serial.print(',');
  Serial.print(currentArm[3]); Serial.print(" | ");
  for (int i = 0; i < 5; i++) {
    if (i) Serial.print(',');
    Serial.print(currentFingers[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < 4; i++) {
    armServos[i].attach(ARM_SERVO_PINS[i]);
  }
  liftB.attach(LIFT_B_PIN);
  elbowB.attach(ELBOW_B_PIN);
  for (int i = 0; i < 4; i++) {
    writeArm(i, currentArm[i]);
  }

  for (int i = 0; i < 5; i++) {
    fingerServos[i].attach(FINGER_SERVO_PINS[i]);
    fingerServos[i].write(currentFingers[i]);
  }

  Serial.println("READY");
  printPose();
}

void loop() {
  if (Serial.available() <= 0) {
    return;
  }

  incoming = Serial.readStringUntil('\n');
  incoming.trim();
  if (incoming.length() == 0) {
    return;
  }

  if (incoming.startsWith("ARM:")) {
    handleArm(incoming.substring(4));
  } else if (incoming.startsWith("HAND:")) {
    handleHand(incoming.substring(5));
  } else if (incoming == "SPECIAL:JAADUTONA") {
    doJaaduTona();
  } else if (incoming == "HOME") {
    moveArmSmooth(ARM_REST, 12);
  } else {
    Serial.print("ERR unknown: ");
    Serial.println(incoming);
    return;
  }
  Serial.print("OK ");
  Serial.println(incoming);
  printPose();
}
