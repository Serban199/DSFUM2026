
int ledPins[] = {8, 9, 10, 11, 12, 13};

void setup() {
  for (int i = 0; i < 6; i++) {
    pinMode(ledPins[i], OUTPUT);
  }
}

void loop() {
  for (int number = 0; number < 64; number++) {
    int value = number;

    for (int bit = 0; bit < 6; bit++) {
      digitalWrite(ledPins[bit], value % 2);
      value = value / 2;
    }

    delay(500);
  }
}