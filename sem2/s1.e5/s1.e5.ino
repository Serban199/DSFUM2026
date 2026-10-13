int ledPin = 8;

char s[100];
int idx = 0;
char c;
float tensiune = 0;
unsigned long timpAnt = 0;
int perioada = 10;

void setup() {
  pinMode(ledPin, OUTPUT);

  Serial.begin(9600);

  Serial.println("introduceti tensiunea intre 0 si 5 V:");
}

void loop() {

  while (Serial.available() > 0) {
    c = Serial.read();

    if (c == '\n') {
      if (idx > 0) {
        s[idx] = '\0';

        tensiune = atof(s);

        if (tensiune >= 0 && tensiune <= 5) {
          Serial.print("Tensiune setata: ");
          Serial.println(tensiune);
        } 
        else {
          Serial.println("Introduceti o valoare intre 0 si 5 V!");
        }

        idx = 0;
      }
    }
    else if (c != '\r' && idx < 99) {
      s[idx++] = c;
    }
  }

 
  unsigned long timpCurent = millis();

  if (timpCurent - timpAnt >= perioada) {
    timpAnt = timpCurent;

    static int contor = 0;

    float duty = tensiune / 5.0;

    int prag = duty * 100;

    if (contor < prag) {
      digitalWrite(ledPin, HIGH);
    }
    else {
      digitalWrite(ledPin, LOW);
    }

    contor++;

    if (contor >= 100) {
      contor = 0;
    }
  }
}