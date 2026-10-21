int latchPin = 4;
int clockPin = 7;
int dataPin = 8;

byte litere[] = {
  0x88, // a
  0x83, // b
  0xC6, // c
  0xA1, // d
  0x86, // e
  0x8E  // f
};

void setup() {
  // iesire
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
}

void loop() {
  for (int i = 0; i < 6; i++) {
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, LSBFIRST, 0x08); 
    
    shiftOut(dataPin, clockPin, LSBFIRST, litere[i]);
    digitalWrite(latchPin, HIGH);
    delay(5000);
  }
}