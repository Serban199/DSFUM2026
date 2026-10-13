int ledPin = 8; 
int b = 0; 
int pas = 1; 
 
unsigned long timpAnt = 0; 
 
void setup() { 
  pinMode(ledPin, OUTPUT); 
} 
 
void loop() { 
  unsigned long timpCurent = micros(); 
  unsigned long perioada = 2000;  
  unsigned long pozitie = timpCurent % perioada; 
  unsigned long timpAprins = perioada * b / 255; 
 
  if (pozitie < timpAprins) { 
    digitalWrite(ledPin, HIGH); 
  } else { 
    digitalWrite(ledPin, LOW); 
  } 
 
  if (millis() - timpAnterior >= 10) { 
    timpAnt = millis(); 
 
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