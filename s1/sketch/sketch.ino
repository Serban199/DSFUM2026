int optiune_curenta = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("introduceti optiunea:");
  pinMode(13, OUTPUT);
}

void loop() {
  if (Serial.available() > 0) {
    optiune_curenta = Serial.parseInt();
    
      
      if (optiune_curenta == 1) {
        Serial.println("pornit");
        digitalWrite(13, 1);
      } 
      else if (optiune_curenta == 2) {
        Serial.println("oprit");
        digitalWrite(13, 0);
      } 
      else if (optiune_curenta == 3) {
        Serial.println("blink");
      }
    }

  if (optiune_curenta == 3) {
    digitalWrite(13, 1);
    delay(10000);
    digitalWrite(13, 0);
    delay(10000);
  }
}