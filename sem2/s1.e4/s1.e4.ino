
int ledPin = 9;

int b = 0;
int pas = 1;
int viteza = 10;

unsigned long timpAnterior = 0;

char s[100];
int idx = 0;
char c;

void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);

  Serial.println("Introduceti viteza in milisecunde:");
}

void loop() {
  while (Serial.available() > 0) {
    c = Serial.read();

    if (c == '\n') {
      if (idx > 0) {
        s[idx] = '\0';

        int valoare = atoi(s);

        if (valoare > 0) {
          viteza = valoare;
          Serial.print("Viteza setata: ");
          Serial.println(viteza);
        }

        idx = 0;
      }
    } else if (c != '\r' && idx < 99) {
      s[idx++] = c;
    }
  }

  unsigned long timpCurent = millis();

  if (timpCurent - timpAnterior >= (unsigned long)viteza) {
    timpAnterior = timpCurent;

    analogWrite(ledPin, b);

    b = b + pas;

    if (b >= 255) {
      b = 255;
      pas = -1;
    }

    if (b <= 0) {
      b = 0;
      pas = 1;
    }
  }
}