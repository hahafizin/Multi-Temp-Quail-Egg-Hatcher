//assigning pins
#define HEATER 5  //>> [PWM] pin gate mosfet (untuk control Heater)
#define LAMP   6  //>> [PWM] untuk control LED
#define DHTPIN 7  //>> dht11 sensor
#define PIR    8  //>> input pir sensor
#define PIEZO  9  //>> [PWM] piezo (guna piezo sbb boleh buat tone berbeza)
#define HUMID  10 //>> pin base bc547

//PWM PINS : 3, 5, 6, 9, 10, 11

//buat lagu kat piezo
#include "tone.h"
//for DHT11 module
#include <DHT.h>
//parameters: (pin number, sensor model)
DHT dht(DHTPIN, DHT11);
//communication antara arduino dgn esp (arduino as slave)
#include <Wire.h>

//melody
int melody[]         = {NOTE_D5,NOTE_A5,NOTE_D5,NOTE_A5,NOTE_D5};
int noteDurations[]  = {3,3,3,7,7};
int melody2[]        = {NOTE_E5,NOTE_D5,NOTE_FS4,NOTE_GS4,NOTE_CS5,NOTE_B4,NOTE_D4,NOTE_E4,NOTE_B4,NOTE_A4,NOTE_CS4,NOTE_E4,NOTE_A4};
int noteDurations2[] = {8,8,4,4,8,8,4,4,8,8,4,4,1};
//simpan mesej dari esp
String messagefromesp;
int8_t phase,maintainhumidityL,maintainhumidityH,lamponpwm=38;
float  maintainTemp,h,t;
char   datatoesp[17],dhtdata[12];
char   c;
bool   callibrated=1,hstate;

void setup() {
  Serial.begin(9600);
  dht.begin();
  playTone(4);

  pinMode(LED_BUILTIN,OUTPUT);
  pinMode(LAMP,       OUTPUT);  //ke gate mosfet
  pinMode(HEATER,     OUTPUT);  //ke gate mosfet
  pinMode(HUMID,      OUTPUT);  //ke bc547
  pinMode(PIEZO,      OUTPUT);  //direct ke piezo
  pinMode(PIR,        INPUT);   //dari output pir sensor
  
  //testing heater
  playTone(1);
  digitalWrite(HEATER, HIGH);
  _delay_ms(2000);
  digitalWrite(HEATER, LOW);

  //testing humimdifier
  playTone(1);
  digitalWrite(HUMID, HIGH);
  _delay_ms(1300);
  digitalWrite(HUMID, LOW);

  //testing lamp
  //testing humimdifier
  playTone(1);
  for (int i=0; i<=lamponpwm; i++) {
    analogWrite(LAMP, i);
    delay(30);
  }

  playTone(2);
  Serial.println("Arduino started.");
  
  Wire.begin(8); //address = 8
  Wire.onReceive(receiveEvent);
  Wire.onRequest(sendEvent);
}

void receiveEvent(int howMany){
  while (0 < Wire.available()) {
    c = Wire.read();
    messagefromesp += c;
  }
  digitalWrite(LED_BUILTIN,HIGH);
  delay(50);
  digitalWrite(LED_BUILTIN,LOW);

  Serial.print("Received : ");
  Serial.println(c);

       if (c == '0') phase=0;
  else if (c == '1') phase=1;
  else if (c == '2') phase=2;
  else if (c == '3') phase=3;
  
  else if (c == '4') {
    for (int i=0; i<=lamponpwm; i++) {
      analogWrite(LAMP, i);
      delay(30);
    }
  }
  else if (c == '5') {
    for (int i=lamponpwm; i>=0; i--) {
      analogWrite(LAMP, i);
      delay(30);
    }
  }
}

void sendEvent() {
  sprintf(datatoesp,"%02d,%02d.%02d,%d,%d,%d ",(int)h,(int)t,(int)(t*100)%100,digitalRead(HEATER),hstate,digitalRead(PIR));
  Wire.write(datatoesp);
  Serial.print("Send     : ");
  Serial.println(datatoesp);
  Serial.print("Length   : ");
  Serial.println(strlen(datatoesp)); 
}

void loop() {

  if (callibrated) {
    h = dht.readHumidity();
    t = dht.readTemperature()-1;
  } else {
    h = dht.readHumidity();
    t = dht.readTemperature();
  }

  /*ubah suhu dan kelembapan mengikut fasa penetasan

  1. Fasa 1 : Suhu 37.5 °C, Kelembapan 50%-60%, hari 1-7
  2. Fasa 2 : Suhu 38.5 °C, Kelembapan 50%-60%, hari 8-13
  3. Fasa 3 : Suhu 38.5 °C, Kelembapan 60%-65%, hari 14-18
  4. Fasa 4 : Suhu 38.5 °C, Kelembapan 60%-65%, hari ke 18 dan keatas
  */
  if      (phase == 0) { maintainTemp=37.5; maintainhumidityL=50; maintainhumidityH=60; }
  else if (phase == 1) { maintainTemp=38.5; maintainhumidityL=50; maintainhumidityH=60; }
  else if (phase == 2) { maintainTemp=38.5; maintainhumidityL=60; maintainhumidityH=65; }
  else if (phase == 3) { maintainTemp=38.5; maintainhumidityL=60; maintainhumidityH=65; }
  
  maintainTemperature(maintainTemp);
  maintainhumidity(maintainhumidityL, maintainhumidityH);

  if (digitalRead(PIR)) playTone(3);
}

void maintainTemperature(float maintaintemp) {
  if      (t < maintaintemp) {
    digitalWrite(HEATER, HIGH); //hidupkan pemanas
  }
  else if (t > maintaintemp) {
    digitalWrite(HEATER, LOW);  // matikan pemanas
  }
}
void maintainhumidity(int8_t humidmin, int8_t humidmax)  {
  if      (h < humidmin) {
    digitalWrite(HUMID, HIGH); //hidupkan humidifier
    hstate=1;
  }
  else if (h > humidmax) {
    digitalWrite(HUMID, LOW);  //matikan humidifier
    hstate=0;
  }
  else {
    digitalWrite(HUMID, HIGH); //hidupkan humidifier
    hstate=1;
  }
}

void playTone(int mode) {
  if      (mode==1) {
    tone(PIEZO, 3000, 100);
  }
  else if (mode==2) {
    tone(PIEZO, 3000);delay(100);noTone(PIEZO);delay(10);
    tone(PIEZO, 3000);delay(100);noTone(PIEZO);
  }
  else if (mode==3) {
    for (int thisNote = 0; thisNote < 8; thisNote++) {
      int noteDuration = 1000/noteDurations[thisNote];
      tone(PIEZO, melody[thisNote],noteDuration);
      delay(noteDuration +30);
    }
    delay(500);
  }
  else if (mode==4) {
    for (int thisNote = 0; thisNote < 14; thisNote++) {
      int noteDuration = 1000/noteDurations2[thisNote];
      tone(PIEZO, melody2[thisNote],noteDuration);
      delay(noteDuration +30);
    }
    delay(500);
  }
}