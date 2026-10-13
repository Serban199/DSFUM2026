
int ledPin = 9;

void setup() {
  pinMode(ledPin, OUTPUT);
}

void loop() {
  for (int b = 0;b <= 255; b++) {
    analogWrite(ledPin, b);
    delay(10);
  }

  for (int b = 255; b >= 0; b--) {
    analogWrite(ledPin, b);
    delay(10);
  }
}