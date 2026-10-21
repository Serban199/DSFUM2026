int latchPin = 4;
int clockPin = 7;
int dataPin = 8;

byte cifre[] = {
  0xC0, // 0
  0xF9, //  1
  0xA4, // 2
  0xB0, // 3
  0x99, // 4
  0x92, // 5
  0x82, //6
  0xF8, // 7
  0x80, // 8
  0x90  // 9
};

void setup() {
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
}

void loop() {
  for (int i = 0; i < 10; i++) {
    digitalWrite(latchPin, LOW);
    
    
    shiftOut(dataPin, clockPin, LSBFIRST, 0x01); 
    
    shiftOut(dataPin, clockPin, LSBFIRST, cifre[i]);
    
    digitalWrite(latchPin, HIGH);
    
    delay(1000);
  }
}