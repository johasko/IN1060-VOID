/*
 * Kodefil for prosjektoppgave i IN1060 v2019
 * Gruppemedlemmer:
 * • Sverre Blom Breivik
 * • Unni Le
 * • Johannes Skøien
 * • Kristian Arnesen Vik
 * • Martine Woldseth
 */

#include <Adafruit_NeoPixel.h>             //Bibliotek for LED-strips
#include <Wire.h>                          //Bibliotek for sammenkobling av arduinoer

#define MUNN                8              //Pins for output til LED-strips
#define OYNE                9

#define SLAVE_ADDR          9              //Definerer nummer for slave-Arduino
#define ANSWER_SIZE         1              //Stoerrelse paa forventet svar fra slave

#define PIXELMUNN           15             //Antall LEDs i munn
#define PIXELOYE            12             //Antall LEDs i oynene

unsigned long forrigeTid =  0;             //Variabler for debounce-forsinkelse
int forsinkelseSnurr =      100;

int kortReg[] = {262, 294, 400};           //Toner for avspilling ved registrert kort

//Pins til lys og knapper for spiller 1 og 2
const int gronnL1 = 7;
const int rodL1 =   6;
const int gronnL2 = 5;
const int rodL2 =   4;

const int gronnS1 = 10;
const int rodS1 =   11;
const int gronnS2 = 12;
const int rodS2 =   13;

const int lydUt = 3;

//1 = true, 2 = false
int svarS1 = 0;
int svarS2 = 0;

int kort;

//1 = true, 2 = false
int svarKat1[5] = {1, 1, 2, 1,2};          //Kort
int svarKat2[5] = {2, 2, 1, 2, 1};         //Brikke

int lydfilerKat1[5] = {1, 2, 3, 4, 5};
int lydfilerKat2[5] = {6, 7, 8, 9, 10};

//Lengde paa lydfiler pluss noen ms saa programmet venter under avspilling
int forsinkelserKat1[5] = {6000, 6000, 5000, 4500, 5000};
int forsinkelserKat2[5] = {5000, 5000, 5000, 4500, 4000};


Adafruit_NeoPixel munn(PIXELMUNN, MUNN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel oyne(PIXELOYE, OYNE, NEO_GRB + NEO_KHZ800);

void setup() {

  Serial.begin(9600);
  delay(3000);                             //Sikkerhetsdelay ved oppstart for aa hindre kortslutting
  Wire.begin();                            //Initialiserer kobling mellom de to arduinoene
  munn.begin();                            //Initialiserer LED-strips
  oyne.begin();
  munn.setBrightness(40);                  //Justerer lysstyrke paa LEDs
  oyne.setBrightness(40);
  pinMode(gronnS1, INPUT_PULLUP);          //Aktiverer Arduinos innebygde Pull-up-resistorer, som hindrer floating i signalene
  pinMode(rodS1, INPUT_PULLUP);            //knappene sender inn
  pinMode(gronnS2, INPUT_PULLUP);          
  pinMode(rodS2, INPUT_PULLUP);

  pinMode(gronnL1, OUTPUT);                //Pins for lys paa kontrollere settes som output
  pinMode(rodL1, OUTPUT);
  pinMode(gronnL2, OUTPUT);
  pinMode(rodL2, OUTPUT);

}

void loop() {
  munn.clear();                            // Slaar av alle pixler i munn
  oyne.clear();                            // Slaar av alle pixler i oyne

  svarS1 = 0;
  svarS2 = 0;
  kort = 0;
  delay(10);

  Serial.write(16);                        //"Velg ny kategori"

  while (kort == 0) {                      //Lytter etter signaler fra RFID-scanneren saa lenge kort ikke enda er registrert
    Wire.requestFrom(SLAVE_ADDR, 1);
  
    while (Wire.available()) {
      kort = Wire.read();                  //Leser ID for scannet kort og lagrer dette i variabel 'kort'
     }
  }

  registrertKort();
  velgKat(kort);
}

void velgKat(int kort) {
  if (kort == 1) {
    spill(1);
  } 
  else if (kort == 2) {
    spill(2);
  }
}

void spill(int kat) {

  if (kat == 1) {
    Serial.write(12);                      //Sender signal til Raspberry Pi via Serial for aa spille av lydfil
    delay(6000);
    for (int i = 0; i < 5; i++) {
      Serial.write(lydfilerKat1[i]);
      delay(forsinkelserKat1[i]);
      nedtelling();
      delay(2000);

      //Sjekker svar for begge spillere
      sjekkSvarS1(svarKat1[i]);
      sjekkSvarS2(svarKat1[i]);
      if (svarKat1[i] == 1) {
        Serial.write(17);
        allePaa();
      } else {
        Serial.write(18);
        allePaa();
      }
      delay(5000);

      //Slaar av alle lys etter endt paastand, og legger inn et lite delay for aa skape rom mellom paastandene
      munnAv();
      oyeAv();
      slukkLysKontroll();
      svarS1 = 0;
      svarS2 = 0;
      delay(1000);
    }
  } 
  else {
    Serial.write(11);
    delay(6000);
    for (int i = 0; i < 5; i++) {
      Serial.write(lydfilerKat2[i]);
      delay(forsinkelserKat2[i]);
      nedtelling();
      delay(1000);

      //Sjekker svar for begge spillere
      sjekkSvarS1(svarKat2[i]);
      sjekkSvarS2(svarKat2[i]);
      if (svarKat2[i] == 1) {
        Serial.write(17);
        allePaa();
      } else {
        Serial.write(18);
        allePaa();
      }
      allePaa();
      delay(5000);

      //Slaar av alle lys etter endt paastand, og legger inn et lite delay for aa skape rom mellom paastandene
      munnAv();
      oyeAv();
      slukkLysKontroll();
      svarS1 = 0;
      svarS2 = 0;
      delay(1000);
      }
  }

   //Resetter verdi for kort slik at Arduinoen igjen lytter etter scanning av nye kort
   kort = 0;
   return;
}

void sjekkTrykk() {
  if (digitalRead(gronnS1) == LOW) {
        svarS1 = 1;
    };

    if (digitalRead(rodS1) == LOW) {
        svarS1 = 2;
    };

    if (digitalRead(gronnS2) == LOW) {
        svarS2 = 1;
    };

    if (digitalRead(rodS2) == LOW) {
        svarS2 = 2;
    }
}

void sjekkSvarS1(int riktig) {
    if (svarS1 == riktig) {
        digitalWrite(gronnL1, HIGH);
    } else if (svarS1 == 0) {
        digitalWrite(rodL1, LOW);
        digitalWrite(gronnL1, LOW);
    } else {
        digitalWrite(rodL1, HIGH);
    }
}

void sjekkSvarS2(int riktig) {
  if (svarS2 == riktig) {
        digitalWrite(gronnL2, HIGH);
    } else if (svarS2 == 0) {
        digitalWrite(rodL2, LOW);
        digitalWrite(gronnL2, LOW);
        
    } else {
        digitalWrite(rodL2, HIGH);
    }
}

/* For aa unngaa at programmet fryser fullstendig ved bruk av delay(), 
 * slik at f.eks. knappetrykk ikke registreres, lager vi en egen
 * metode for aa oppnaa forsinkelser i programmet mens knapper fremdeles
 * kan trykkes, og bruker dette der det er formaalsmessig.
 */
void forsinkelse(int forsinkelse) { 
  forrigeTid = millis();
  while (millis() < forrigeTid + forsinkelse) {
    //Venter [forsinkelse] ms
    sjekkTrykk();
  }
}

boolean nedtelling() {

  munnPaa();
  munn.show();

  oyeSnurr();

  munn.setPixelColor(0, munn.Color(0, 0, 0));
  munn.setPixelColor(1, munn.Color(0, 0, 0));
  munn.show();

  oyeSnurr();
  munn.setPixelColor(2, munn.Color(0, 0, 0));
  munn.setPixelColor(3, munn.Color(0, 0, 0));
  munn.show();

  oyeSnurr();
  munn.setPixelColor(4, munn.Color(0, 0, 0));
  munn.setPixelColor(5, munn.Color(0, 0, 0));
  munn.show();
  
  oyeSnurr();
  munn.setPixelColor(6, munn.Color(0, 0, 0));
  munn.setPixelColor(7, munn.Color(0, 0, 0));
  munn.show();
  
  oyeSnurr();
  munn.setPixelColor(8, munn.Color(0, 0, 0));
  munn.setPixelColor(9, munn.Color(0, 0, 0));
  munn.show();

  oyeSnurr();
  munn.setPixelColor(10, munn.Color(0, 0, 0));
  munn.setPixelColor(11, munn.Color(0, 0, 0));
  munn.show();

  oyeSnurr();

  munn.setPixelColor(12, munn.Color(0, 0, 0));
  munn.setPixelColor(13, munn.Color(0, 0, 0));
  munn.show();  

  oyeSnurr();
  tidUte();
  slukkLysKontroll();

}

void tidUte() {
  for (int i = 0; i<4; i++) {
    blinking();
  }
}

void blinking() {
  munnPaa();
  munn.show();
  delay(200);
  munnAv();
  munn.show();
  delay(200);
}

//LIGHTS

void munnPaa() {                           //Setter alle LEDs i munnen til aa vaere paa med hvitt lys
  for (int i = 0; i < PIXELMUNN; i++) {
    munn.setPixelColor(i, munn.Color(255, 255, 255));
  }
}

void allePaa() {                          //Setter alle LEDs til aa vaere paa med hvitt lys
  for (int i = 0; i < PIXELMUNN; i++) {
    munn.setPixelColor(i, munn.Color(255, 255, 255));
  }

  for (int i = 0; i < PIXELOYE; i++) {
    oyne.setPixelColor(i, oyne.Color(255, 255, 255));
  }
  munn.show();
  oyne.show();
}

void munnAv() {                            //Slaar av alle LEDs i munnen
  for (int i = 0; i < PIXELMUNN; i++) {
    munn.setPixelColor(i, munn.Color(0, 0, 0));
  }
  munn.show();
}

void oyeSnurr() {
    oyne.clear();                          //Starter med alle pixler slaatt av

  for (int i = 0; i < PIXELOYE; i++) {
    oyne.setPixelColor(i, oyne.Color(255, 255, 255));
    oyne.show();
    forsinkelse(100);                      //Bruker forsinkelse() fremfor delay() for aa fremdeles kunne registrere knappetrykk
  }

  for (int i = 0; i < PIXELOYE; i++) {
    oyne.setPixelColor(i, oyne.Color(0, 0, 0));
    oyne.show();
    forsinkelse(100);
  }

}

void oyeAv() {                             //Slaar av LEDs i oyne
  for (int j = 0; j < PIXELOYE; j++) {
      oyne.setPixelColor(j, oyne.Color(0, 0, 0));
      oyne.show();
      forsinkelse(100);
  }
}

void slukkLysKontroll() {                  //Slaar av LEDs paa kontrollerne
  digitalWrite(gronnL1, LOW);
  digitalWrite(rodL1, LOW);
  digitalWrite(gronnL2, LOW);
  digitalWrite(rodL2, LOW);
}

//PIEZO          
void registrertKort() {                   //Lydsignaler som sendes til piezo ved registrert RFID-kort
  tone(lydUt, kortReg[0]);
  delay(200);
  tone(lydUt, kortReg[1]);
  delay(200);
  tone(lydUt, kortReg[2]);
  delay(200);
  noTone(lydUt);
}
