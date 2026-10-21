int latchPin = 4;
int clockPin = 7;
int dataPin = 8;

byte cifre[] = {
  0xC0, 0xF9, 0xA4, 0xB0, 0x99, 
  0x92, 0x82, 0xF8, 0x80, 0x90
};

int counter = 0;

bool ultimaStareA1 = false;
bool ultimaStareA2 = false;
bool ultimaStareA3 = false;

void setup() {
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, INPUT_PULLUP);
}

void loop() {
  bool stareA1 = !digitalRead(A1);
  bool stareA2 = !digitalRead(A2);
  bool stareA3 = !digitalRead(A3);

  if (stareA1 && !ultimaStareA1) {
    if (counter < 9999) {
      counter++;
    }
  }
  ultimaStareA1 = stareA1;

  if (stareA2 && !ultimaStareA2) {
    if (counter > 0) {
      counter--;
    }
  }
  ultimaStareA2 = stareA2;
  if (stareA3 && !ultimaStareA3) {
    counter = 0;
  }
  ultimaStareA3 = stareA3;

  afisareNumar(counter);
}

void afisareNumar(int numar) {
  int extrase[4];
  extrase[0] = numar / 1000;
  extrase[1] = (numar / 100) % 10;
  extrase[2] = (numar / 10) % 10;
  extrase[3] = numar % 10;

  byte pozitii[] = {0x01, 0x02, 0x04, 0x08}; 
  
  bool afiseazaMii = (numar >= 1000);
  bool afiseazaSute = (numar >= 100);
  bool afiseazaZeci = (numar >= 10);
  bool afiseazaActivat[4] = {afiseazaMii, afiseazaSute, afiseazaZeci, true};

  for(int i = 0; i < 4; i++) {
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, 0xFF);
    shiftOut(dataPin, clockPin, MSBFIRST, 0x00);
    digitalWrite(latchPin, HIGH);
    
    if (afiseazaActivat[i]) {
      digitalWrite(latchPin, LOW);
      
      shiftOut(dataPin, clockPin, MSBFIRST, cifre[extrase[i]]);
      shiftOut(dataPin, clockPin, MSBFIRST, pozitii[i]);
      
      digitalWrite(latchPin, HIGH);
      delay(5); 
    }
  }
}