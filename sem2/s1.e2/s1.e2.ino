
int ledPins[] = {8, 9, 10, 11, 12, 13};

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < 6; i++) {
    pinMode(ledPins[i], OUTPUT);
  }

  Serial.println("numar intre 0 si 63:");
}

char s[1000];
int idx = 0;
char c;

void loop() {
  while (Serial.available() > 0) {
    c = Serial.read();

    if (c == '\n') {
      if (idx > 0) {
        s[idx] = '\0';

        Serial.print("receptionat: ");
        Serial.println(s);

        int number = atoi(s);

        if (number >= 0 && number <= 63) {
          for (int bit = 0; bit < 6; bit++) {
            digitalWrite(ledPins[bit], number % 2);
            number = number / 2;
          }
        } else {
          Serial.println("introduceti o valoare intre 0 si 63");
        }

        idx = 0;
      }
    } else if (c != '\r' && idx < 999) {
      s[idx++] = c;
    }
  }
}