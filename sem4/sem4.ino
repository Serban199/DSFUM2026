struct LedState {
  int pin;
  String animatie;
  unsigned long timp_ms;
  unsigned long start_time;
};

LedState leds[6];

void setup() {
  Serial.begin(9600);
  
  for (int i = 0; i < 6; i++) {
    leds[i].pin = 8 + i;
    leds[i].animatie = "off";
    leds[i].timp_ms = 0;
    leds[i].start_time = 0;
    
    pinMode(leds[i].pin, OUTPUT);
    digitalWrite(leds[i].pin, LOW);
  }
  
  Serial.println("Sistem pornit. Astept comenzi (ex: 8,fadeinout,2000)");
}

void loop() {
  if (Serial.available() > 0) {
    String comanda = Serial.readStringUntil('\n');
    comanda.trim(); 
    
    int primaVirgula = comanda.indexOf(',');
    int aDouaVirgula = comanda.indexOf(',', primaVirgula + 1);
    
    if (primaVirgula > 0 && aDouaVirgula > 0) {
      int nr_led = comanda.substring(0, primaVirgula).toInt();
      String animatie = comanda.substring(primaVirgula + 1, aDouaVirgula);
      unsigned long timp = comanda.substring(aDouaVirgula + 1).toInt();
      
      if (nr_led >= 8 && nr_led <= 13) {
        int index = nr_led - 8; 
        
        leds[index].animatie = animatie;
        leds[index].timp_ms = timp;
        leds[index].start_time = millis(); 
        
        if (animatie == "off") {
          digitalWrite(leds[index].pin, LOW);
        }
        
        Serial.print("Comanda acceptata pentru LED ");
        Serial.println(nr_led);
      } else {
        Serial.println("Eroare: Pinul trebuie sa fie intre 8 si 13.");
      }
    }
  }

  unsigned long timpCurent = millis();
  
  for (int i = 0; i < 6; i++) {
    if (leds[i].animatie == "off" || leds[i].timp_ms == 0) {
      continue; 
    }
    
    unsigned long timpScurs = (timpCurent - leds[i].start_time) % leds[i].timp_ms;
    
    float progres = (float)timpScurs / leds[i].timp_ms; 
    
    if (leds[i].animatie == "fadein") {
      int valoarePwm = progres * 255;
      analogWrite(leds[i].pin, valoarePwm);
    } 
    else if (leds[i].animatie == "fadeout") {
      int valoarePwm = (1.0 - progres) * 255;
      analogWrite(leds[i].pin, valoarePwm);
    } 
    else if (leds[i].animatie == "fadeinout") {
      int valoarePwm;
      if (progres < 0.5) {
        valoarePwm = (progres * 2.0) * 255;
      } else {
        valoarePwm = ((1.0 - progres) * 2.0) * 255;
      }
      analogWrite(leds[i].pin, valoarePwm);
    } 
    else if (leds[i].animatie == "blink") {
      if (progres < 0.5) {
        digitalWrite(leds[i].pin, HIGH);
      } else {
        digitalWrite(leds[i].pin, LOW);
      }
    }
  }
}