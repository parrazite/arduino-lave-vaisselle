


#include <math.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <RTClib.h>

RTC_DS1307 RTC;

LiquidCrystal lcd(13, 12, 11, 10, 9, 8);

const char* dayOfWeek[] = {
  "Lundi ", "Mardi", "Mercredi", "Jeudi", "Vendredi ", "Samedi ", "Dimanche"
};

const char* currentMonth[] = {
  "Janvier", "Fevrier", "Mars", "Avril", "Mai", "Juin", "Juillet", "Aout", "Septembre", "Octobre", "Novembre", "Decembre"
};
boolean pm = false;
unsigned long cycleStart;
double tempArray[25];
byte arrayIndex = 0;

#define ventPin 7
#define soapDispensor 6
#define waterInlet 5
#define drainPin 4
#define washMotor 3
#define heaterPin 2
#define goButton A1
#define tempSensor A2
#define goLight 0
#define stopLight 1
#define stopButton A3
#define tiltSensor A0

boolean prog_long = false;

double waterTempMaintien(float consigne) {
  if (arrayIndex > 23) {
    arrayIndex = 0;
  }
  else {
    arrayIndex++;
  }
  double Temp;
  int RawADC = analogRead(tempSensor);
  Temp = log(((10240000 / RawADC) - 10000));
  Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp )) * Temp );
  Temp = Temp - 273.15;
  tempArray[arrayIndex] = Temp;
  Temp = 0;
  for (int i = 0; i < 24; i++) {
    Temp += tempArray[i];
  }
  if (Temp >= (consigne + 2)) {
    digitalWrite(heaterPin, HIGH);
  }
  else {
    if (Temp <= (consigne - 2)) {
      digitalWrite(heaterPin, LOW);
    }
  }
  return (Temp / 25);

}

double waterTemp() {
  if (arrayIndex > 23) {
    arrayIndex = 0;
  }
  else {
    arrayIndex++;
  }
  double Temp;
  int RawADC = analogRead(tempSensor);
  Temp = log(((10240000 / RawADC) - 10000));
  Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp )) * Temp );
  Temp = Temp - 273.15;
  tempArray[arrayIndex] = Temp;
  Temp = 0;
  for (int i = 0; i < 24; i++) {
    Temp += tempArray[i];
  }
  return (Temp / 25);

}

void setup () {
  for (int i = 0; i < 25; i++) tempArray[i] = 70;
  Wire.begin();
  RTC.begin();
  lcd.begin(20, 4);

  pinMode(tempSensor,   INPUT);
  pinMode(ventPin,      OUTPUT); digitalWrite (ventPin, HIGH);      //7
  pinMode(soapDispensor, OUTPUT); digitalWrite (soapDispensor, HIGH); //6
  pinMode(waterInlet,   OUTPUT); digitalWrite (waterInlet, HIGH);   //5
  pinMode(drainPin,     OUTPUT); digitalWrite (drainPin, HIGH);     //4
  pinMode(washMotor,    OUTPUT); digitalWrite (washMotor, HIGH);    //3
  pinMode(heaterPin,    OUTPUT); digitalWrite (heaterPin, HIGH);    //2
  pinMode(goButton,     INPUT_PULLUP);                              //A1
  pinMode(stopButton,   INPUT_PULLUP);                              //A3
  pinMode(tiltSensor,   INPUT_PULLUP);                              //A0

  pinMode(0, OUTPUT);                                               //0 led lavage en cours
  pinMode(1, OUTPUT);                                               //1 led vaisselle propre
}

void updateRuntimeDisplay(byte currentMode) {
  unsigned long elapsedSeconds = ((millis() - cycleStart) / 1000);
  unsigned long elapsedMinutes = (elapsedSeconds / 60);
  unsigned long elapsedHours = (elapsedMinutes / 60);
  lcd.setCursor(0, 0);
  lcd.print(F("Duree cycle: "));
  if (elapsedHours > 0) {
    lcd.print(elapsedHours);
    lcd.print(':');
    if (elapsedHours == 1) elapsedMinutes -= 60;
    if (elapsedHours == 2) elapsedMinutes -= 120;
  }
  if (elapsedMinutes < 10) {
    lcd.print('0');
  }
  lcd.print(elapsedMinutes);
  lcd.setCursor(0, 1);
  lcd.print(F("Temp eau: "));
  lcd.print(int(waterTemp()));
  lcd.print('C');
  lcd.setCursor(0, 3);

  switch (currentMode) {
    case 1:
      lcd.print(F("Remplissage "));
      break;
    case 2:
      lcd.print(F("Vidange "));
      break;
    case 3:
      lcd.print(F("Lavage "));
      break;
    case 4:
      lcd.print(F("Pre rincage "));
      break;
    case 5:
      lcd.print(F("Rincage final "));
      break;
    case 6:
      lcd.print(F("sechage vaisselle "));
      break;
  }
}
void fillItUp() {
  unsigned long fillTime = millis();
  digitalWrite(waterInlet, LOW);
  while ((millis() - fillTime) < 105000) {
    delay(100);
    updateRuntimeDisplay(1);
  }
  digitalWrite(waterInlet, HIGH);
  delay(100);
  //lcd.begin(20, 4);
  lcd.clear();
}

void drainItOut() {
  unsigned long drainTime = millis();
  digitalWrite(drainPin, LOW);
  delay(500);
  lcd.begin(20, 4);
  // lcd.clear();
  while ((millis() - drainTime) < 105000) {
    delay(100);
    updateRuntimeDisplay(2);
  }
  digitalWrite(drainPin, HIGH);
  delay(100);
  //lcd.begin(20, 4);
  lcd.clear();
}

void tiltRoutine() {
  long doorOpened = millis();
  while (digitalRead(tiltSensor)) {
    if ((millis() - doorOpened) > 120000) {
      lcd.setCursor(0, 3);
      lcd.print(" Porte ouverte ");
    }
  }
}
void ventDry() {
  digitalWrite(ventPin, LOW);
  while (waterTemp() > 115) {
    delay(1000);
    updateRuntimeDisplay(6);
  }
  digitalWrite(ventPin, HIGH);
}


void preRinse() {
  unsigned long rinseTime = millis();
  digitalWrite(washMotor, LOW);
  delay(500);
  //lcd.begin(20, 4);
  lcd.clear();
  while ((millis() - rinseTime) < 600000) {
    updateRuntimeDisplay(4);
    delay(1000);
  }
  digitalWrite(washMotor, HIGH);
  delay(100);
  lcd.begin(20, 4);
  lcd.clear();
}

void finalRinse() {
  unsigned long rinseTime = millis();
  digitalWrite(heaterPin, LOW);
  digitalWrite(washMotor, LOW);
  digitalWrite(soapDispensor, LOW);
  delay(500);
  //lcd.begin(20, 4);
  lcd.clear();
  while ((millis() - rinseTime) < 30000) {
    updateRuntimeDisplay(5);
    delay(500);
  }
  digitalWrite(soapDispensor, HIGH);
  while (waterTemp() < 60) {
    delay(1000);
    updateRuntimeDisplay(5);
  }
  digitalWrite(heaterPin, HIGH);
  delay(5000);
  digitalWrite(washMotor, HIGH);
  delay(100);
  //lcd.begin(20, 4);
  lcd.clear();
}

void washTheDishes() {
  digitalWrite(heaterPin, LOW);
  digitalWrite(washMotor, LOW);
  delay(1500);
  //lcd.begin(20, 4);
  lcd.clear();
  while (waterTemp() < 48) {
    updateRuntimeDisplay(3);
    delay(1000);
  }
  unsigned long soapTime = millis();
  digitalWrite(soapDispensor, LOW);
  while ((millis() - soapTime) < 30000) {
    delay(500);
    updateRuntimeDisplay(3);
  }
  digitalWrite(soapDispensor, HIGH);
  while (waterTemp() < 68) {
    delay(1000);
    updateRuntimeDisplay(3);
  }
  if (prog_long == true) {
    washTheDishesMaintien(360000, 68);
  }
}

void washTheDishesMaintien(long temps, int temperatur) {
  digitalWrite(heaterPin, LOW);
  digitalWrite(washMotor, LOW);
  delay(1500);
  //lcd.begin(20, 4);
  lcd.clear();
  unsigned long maintien = millis();
  while ((millis() - maintien) <= temps) {
    updateRuntimeDisplay(3);
    waterTempMaintien(temperatur);
  }
  digitalWrite(heaterPin, HIGH);
  delay(5000);
  digitalWrite(washMotor, HIGH);
  delay(100);
  //lcd.begin(20, 4);
  lcd.clear();
}

void loop() {
  // drainItOut();
  lcd.setCursor(0, 0);
  lcd.print(F("Date: "));
  digitalWrite(0, LOW);
  digitalWrite(1, LOW);
  while ((digitalRead(goButton))) {
    if (!digitalRead(stopButton)) drainItOut();
    if (digitalRead(tiltSensor)) {
      tiltRoutine();
    }

    DateTime now = RTC.now();
    lcd.setCursor(10, 0);
    //lcd.print(dayOfWeek[now.dayOfWeek()]);
    lcd.setCursor(0, 1);
    lcd.print(currentMonth[(now.month() - 1)]);
    lcd.print(' ');
    lcd.print(now.day(), DEC);
    lcd.print(", ");
    lcd.print(now.year(), DEC);
    lcd.setCursor(0, 2);
    if (now.hour() > 12) {
      pm = true;
    }
    else {
      pm = false;
    }
    if (pm) {
      lcd.print((now.hour() - 12), DEC);
    }
    else {
      lcd.print(now.hour(), DEC);
    }
    lcd.print(':');
    if (now.minute() < 10) {
      lcd.print('0');
    }
    lcd.print(now.minute(), DEC);
    lcd.print(':');
    if (now.second() < 10) {
      lcd.print('0');
    }
    lcd.print(now.second(), DEC);
    if (pm) {
      lcd.print(F(" PM "));
    }
    else {
      lcd.print(F(" AM "));
    }


    digitalWrite(goLight, HIGH);
    digitalWrite(stopLight, LOW);
    lcd.clear();
    cycleStart = millis();
    drainItOut();    //on vidange l'eau
    delay(1000);
    fillItUp();      //remplissage d'eau
    delay(1000);
    washTheDishes(); //on fait la vaisselle
    delay(1000);
    drainItOut();   //on vidange l'eau
    delay(1000);
    fillItUp();     //remplissage d'eau
    delay(1000);
    preRinse();     //on fait le pré rincage
    delay(1000);
    drainItOut();   //on vidange l'eau
    delay(1000);
    fillItUp();     //remplissage d'eau
    delay(1000);
    finalRinse();   //on fait le rincage final
    delay(1000);
    drainItOut();   //on vidange l'eau
    delay(1000);
    ventDry();      //on seche à l'air chaud
    lcd.clear();
    lcd.setCursor(0, 3);
    lcd.print(F("Vaisselle propre!"));
    digitalWrite(goLight, HIGH);             //extinction de la led "lavage en cours"
    digitalWrite(stopLight, LOW);            //allumage de la led " vaisselle propre"
  }
}
