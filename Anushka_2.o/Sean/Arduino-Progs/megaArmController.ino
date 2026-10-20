#include <Servo.h>

/*
  Flash one copy to the left-arm Mega and one copy to the right-arm Mega.
  All gesture angles live in Python (Sean/Arms/arms_controller.py); this board
  only moves smoothly to the angles it receives.

  ARM:rotate,lift,elbow,wrist[,stepMs]
      Shoulder lift and elbow are mirror pairs: servo A gets the angle,
      servo B gets 180 - angle. All joints start and stop together with an
      eased (soft start/stop) motion; stepMs = ms per degree of the largest
      joint change (default 8).
  HAND:f1,f2,f3,f4,f5
  SPECIAL:JAADUTONA

  Replies after every command, once the move has finished:
      POSE:rotate,liftA,liftB,elbowA,elbowB,wrist | f1,f2,f3,f4,f5
      OK <command>          <- Python waits for this before the next step
*/

// Arm servos: shoulder rotate, lift A, lift B, elbow A, elbow B, wrist.
const int ARM_PINS[6] = {2, 3, 4, 5, 6, 7};
// Fingers are not wired yet; set these when they are.
const int FINGER_SERVO_PINS[5] = {8, 9, 10, 11, 12};

// Power-on pose (arm hanging at the side): rotate, lift, elbow, wrist.
// Python's REST must match.
int currentArm[4] = {0, 105, 90, 90};
int currentFingers[5] = {90, 90, 90, 90, 90};

Servo armServos[6];
Servo fingerServos[5];

String incoming;

int nextValue(String &payload, int &cursor, int fallback) {
  if (cursor >= (int)payload.length()) return fallback;
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

// Write one arm joint; lift (1) and elbow (2) also drive their mirror servo.
void writeArm(int i, int angle) {
  if (i == 0) armServos[0].write(angle);
  if (i == 1) { armServos[1].write(angle); armServos[2].write(180 - angle); }
  if (i == 2) { armServos[3].write(angle); armServos[4].write(180 - angle); }
  if (i == 3) armServos[5].write(angle);
}

// Move every joint from its current angle to its target so they all start and
// finish together, easing in and out (smoothstep) to avoid jerks.
// isArm: true writes through writeArm(), false writes servos[i] directly.
void moveSmooth(Servo *servos, int *currentValues, const int *targets, int count, int stepMs, bool isArm) {
  int start[5];
  int steps = 0;
  for (int i = 0; i < count; i++) {
    start[i] = currentValues[i];
    steps = max(steps, abs(targets[i] - start[i]));
  }
  for (int s = 1; s <= steps; s++) {
    float t = (float)s / steps;
    float e = t * t * (3.0 - 2.0 * t);
    for (int i = 0; i < count; i++) {
      int v = start[i] + (int)round((targets[i] - start[i]) * e);
      if (v != currentValues[i]) {
        currentValues[i] = v;
        if (isArm) writeArm(i, v); else servos[i].write(v);
      }
    }
    delay(stepMs);
  }
}

void handleArm(String payload) {
  int cursor = 0;
  int rotate = nextValue(payload, cursor, currentArm[0]);
  int lift = nextValue(payload, cursor, currentArm[1]);
  int elbow = nextValue(payload, cursor, currentArm[3]);
  int wristAngle = nextValue(payload, cursor, currentArm[5]);
  int stepMs = nextValue(payload, cursor, 8);
  int targets[4] = {rotate, lift, elbow, wristAngle};
  moveSmooth(armServos, currentArm, targets, 4, stepMs, true);
}

void handleHand(String payload) {
  int targets[5];
  int cursor = 0;
  for (int i = 0; i < 5; i++) {
    targets[i] = nextValue(payload, cursor, currentFingers[i]);
  }
  moveSmooth(fingerServos, currentFingers, targets, 5, 8, false);
}

void doJaaduTona() {
  int spellA[5] = {170, 90, 55, 55, 55};
  int spellB[5] = {120, 40, 130, 40, 130};
  moveSmooth(fingerServos, currentFingers, spellA, 5, 6, false);
  delay(250);
  moveSmooth(fingerServos, currentFingers, spellB, 5, 6, false);
  delay(250);
  moveSmooth(fingerServos, currentFingers, spellA, 5, 6, false);
}

void printPose() {
  // rotate, liftA, liftB, elbowA, elbowB, wrist
  int six[6] = {currentArm[0], currentArm[1], 180 - currentArm[1],
                currentArm[2], 180 - currentArm[2], currentArm[3]};
  Serial.print("POSE:");
  for (int i = 0; i < 6; i++) {
    if (i) Serial.print(',');
    Serial.print(six[i]);
  }
  Serial.print(" | ");
  for (int i = 0; i < 5; i++) {
    if (i) Serial.print(',');
    Serial.print(currentFingers[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < 6; i++) {
    armServos[i].attach(ARM_PINS[i]);
  }
  for (int i = 0; i < 4; i++) {
    writeArm(i, currentArm[i]);
  }
  for (int i = 0; i < 5; i++) {
    fingerServos[i].attach(FINGER_SERVO_PINS[i]);
    fingerServos[i].write(currentFingers[i]);
  }

  printPose();
  Serial.println("READY");
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
  } else {
    Serial.print("ERR unknown: ");
    Serial.println(incoming);
    return;
  }
  printPose();
  Serial.print("OK ");
  Serial.println(incoming);
}
