int latchPin = 4;
int clockPin = 7;
int dataPin = 8;

byte cifre[] = {
  0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90
};

const int nr_runde = 10;
unsigned long timpi[10];
int valori[10];
int runda = 0;

int numarCurent = 0;
unsigned long timpStart = 0;
bool asteptareInput = false;
bool terminat = false;
String inputBuffer = "";

void setup() {
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  
  Serial.begin(9600);
  randomSeed(analogRead(A0));
  
  Serial.println("incepem testul de viteza.");
}

void loop() {
  if (terminat) {
    return; 
  }

  if (!asteptareInput) {
    numarCurent = random(1000, 10000); 
    timpStart = millis();
    asteptareInput = true;
    inputBuffer = ""; 
    
    while(Serial.available()) {
      Serial.read();
    }
  }

  afisareNumar(numarCurent);

  if (Serial.available() > 0) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        int numarIntrodus = inputBuffer.toInt();
        
        if (numarIntrodus == numarCurent) {
          unsigned long timpTrecut = millis() - timpStart;
          
          timpi[runda] = timpTrecut;
          valori[runda] = numarCurent;
          runda++;
          
          asteptareInput = false;
          
          if (runda >= nr_runde) {
            terminat = true;
            stingeDisplay();
            afisareStatistici();
          } else {
            stingeDisplay();
            delay(1000); 
          }
        } else {
          inputBuffer = "";
        }
      }
    } else {
      inputBuffer += c;
    }
  }
}

void afisareNumar(int numar) {
  int extrase[4];
  extrase[0] = numar / 1000;
  extrase[1] = (numar / 100) % 10;
  extrase[2] = (numar / 10) % 10;
  extrase[3] = numar % 10;

  byte pozitii[] = {0x01, 0x02, 0x04, 0x08}; 

  for(int i = 0; i < 4; i++) {
    digitalWrite(latchPin, LOW);
    
    shiftOut(dataPin, clockPin, MSBFIRST, cifre[extrase[i]]);
    shiftOut(dataPin, clockPin, MSBFIRST, pozitii[i]);
    
    digitalWrite(latchPin, HIGH);
    
    delay(3); 
  }
}

void stingeDisplay() {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, 0xFF);
  shiftOut(dataPin, clockPin, MSBFIRST, 0x00);
  digitalWrite(latchPin, HIGH);
}

void afisareStatistici() {
  unsigned long timpMin = timpi[0];
  unsigned long timpMax = timpi[0];
  int valMin = valori[0];
  int valMax = valori[0];
  unsigned long suma = 0;

  for (int i = 0; i < nr_runde; i++) {
    if (timpi[i] < timpMin) {
      timpMin = timpi[i];
      valMin = valori[i];
    }
    if (timpi[i] > timpMax) {
      timpMax = timpi[i];
      valMax = valori[i];
    }
    suma += timpi[i];
  }

  float media = (float)suma / nr_runde;  
  Serial.print("timpul minim: ");
  Serial.print(timpMin);
  Serial.print(" ms (pentru numarul ");
  Serial.print(valMin);
  Serial.println(")");

  Serial.print("timpul maxim: ");
  Serial.print(timpMax);
  Serial.print(" ms (pentru numarul ");
  Serial.print(valMax);
  Serial.println(")");

  Serial.print("media timpilor: ");
  Serial.print(media);
  Serial.println(" ms");
}