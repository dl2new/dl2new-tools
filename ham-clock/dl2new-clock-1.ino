/* Stationsuhr DL2NEW
 *  V1.0 14.01.2018
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <OneWire.h>

//#include <Time.h>
#include <TimeLib.h>

#define BAND_PIN 2               // Connection pin to Band Selector
#define BAND_INTERRUPT 0          // Interrupt number associated with pin

LiquidCrystal_I2C lcd(0x3f, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
OneWire  ds(10);

SoftwareSerial mySerial(5, 4);

Adafruit_GPS GPS(&mySerial);
#define GPSECHO  false

boolean usingInterrupt = false;
void useInterrupt(boolean);  // Func prototype keeps Arduino 0023 happy

// Baken
char *callsign[19]={
"000000", " 4U1UN", " VE8AT", "  W6WX", " KH6RS", "  ZL6B", "VK6RBP", 
"JA2IGY", "  RR9O", "  VR2B", "  4S7B", " ZS6DN", "  5Z4B", " 4X6TU", 
"  OH2B", "  CS3B", " LU4AA", "  OA4B", "  YV5B"}; 

char *qrg[6] = {"      ","14.100","18.110","21.150","24.930","28.200"};

byte band, sekm,minm,feld,spalte,i;

unsigned long alteZeit=0, entprellZeit=20; //Bandwahl

byte  matrix[19][18] = { 
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
{1,2,3,4,5,0,0,0,0,0,0,0,0,0,0,0,0,0},
{0,1,2,3,4,5,0,0,0,0,0,0,0,0,0,0,0,0},
{0,0,1,2,3,4,5,0,0,0,0,0,0,0,0,0,0,0},
{0,0,0,1,2,3,4,5,0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,1,2,3,4,5,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,1,2,3,4,5,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,1,2,3,4,5,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,1,2,3,4,5,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,1,2,3,4,5,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,1,2,3,4,5,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0,1,2,3,4,5,0,0,0},
{0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,5,0,0},
{0,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,5,0},
{0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,5},
{5,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4},
{4,5,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,3},
{3,4,5,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2},
{2,3,4,5,0,0,0,0,0,0,0,0,0,0,0,0,0,1}
}; 

void setup()
{
  // Bandwahltaste
  pinMode(BAND_PIN, INPUT);       // Pin 2 ist INT0
  digitalWrite(BAND_PIN, HIGH);   // interner Pull up Widerstand auf 5V
  attachInterrupt(BAND_INTERRUPT, interruptRoutineBand, LOW);
  band = 1;
  
  Serial.begin(115200);
  GPS.begin(9600);

  lcd.begin(20, 4);                      // initialize the lcd
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(3, 1);
  lcd.print("GPS Stationsuhr");
  lcd.setCursor(3, 2);
  lcd.print("V0.9 DL2NEW");
  delay(5000);
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("Looking");
  lcd.setCursor(8, 1);
  lcd.print("For");
  lcd.setCursor(5, 2);
  lcd.print("Satellites");

  delay(10000);
  lcd.clear();

  // turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);

  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate

  // Request updates on antenna status, comment out to keep quiet
  GPS.sendCommand(PGCMD_ANTENNA);

  // the nice thing about this code is you can have a timer0 interrupt go off
  // every 1 millisecond, and read data from the GPS for you. that makes the
  // loop code a heck of a lot easier!
  useInterrupt(true);

  delay(1000);
  // Ask for firmware version
  mySerial.println(PMTK_Q_RELEASE);
}

// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
}

// Interrupt Bandwahl
void interruptRoutineBand() {
  if((millis() - alteZeit) > entprellZeit) { 
    // innerhalb der entprellZeit nichts machen
    alteZeit = millis(); // letzte Schaltzeit merken   
    band++;
    if (band > 5) {
      band = 1;
      }   
  //verarbeiten();
  }
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}

uint32_t timer = millis();

void loop()                     // run over and over again
{
  // in case you are not using the interrupt above, you'll
  // need to 'hand query' the GPS, not suggested :(
  if (! usingInterrupt) {
    // read data from the GPS in the 'main loop'
    char c = GPS.read();
  }

  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    //Serial.println(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false

    if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
      return;  // we can fail to parse a sentence in which case we should just wait for another
  }

  // if millis() or timer wraps around, we'll just reset it
  if (timer > millis())  timer = millis();

  // approximately every 1 seconds, print out the current stats
  if (millis() - timer > 1000) {
    timer = millis(); // reset the timer
    
    lcd.setCursor(0, 0);
    lcd.print("UTC   ");
    if (GPS.hour < 10) lcd.print("0");
    lcd.print((int)GPS.hour); lcd.print(":");
    if (GPS.minute < 10) lcd.print("0");
    lcd.print((int)GPS.minute); lcd.print(":");
    if (GPS.seconds < 10) lcd.print("0");
    lcd.print((int)GPS.seconds);

    lcd.setCursor(15, 0);
    if (GPS.day < 10) lcd.print("0");
    lcd.print((int)GPS.day); lcd.print("/");
    if (GPS.month < 10) lcd.print("0");
    lcd.print((int)GPS.month);

    // LAT und Fix
    lcd.setCursor(0, 1);
    lcd.print("LAT   ");
    lcd.print(GPS.latitudeDegrees);
    lcd.setCursor(15, 1);
    lcd.print("Fix");
    lcd.setCursor(18, 1);
    if (GPS.satellites < 10) lcd.print("0");
    lcd.print((int)GPS.satellites);

    // LON und Höhe
    lcd.setCursor(0, 2);
    lcd.print("LON   ");
    lcd.print(GPS.longitudeDegrees);

    //Positionierung der Höhe
    int hoehe = (int)GPS.altitude;
    if ((hoehe >= 0) && (hoehe < 10)) lcd.setCursor(18, 2);
    if ((hoehe >= 10) && (hoehe < 100)) lcd.setCursor(17, 2);
    if ((hoehe >= 100) && (hoehe < 1000)) lcd.setCursor(16, 2);
    if (hoehe > 1000) lcd.setCursor(15, 2);
    lcd.print(hoehe);
    lcd.setCursor(19, 2);
    lcd.print("m");

    // ncdxf bAKEN - Neu/Test
    //band = 1;

    minm = (int)GPS.minute % 3;
    sekm = (int)GPS.seconds / 10;
    feld = (minm*60)+(sekm*10);
    spalte = (feld/10);
    //Serial.print("Spalte: "); Serial.println(spalte);

    for (i=0;i<19;i++) {
      //Serial.println(matrix[i][spalte]);
      if (matrix[i][spalte] == band) {
        Serial.print("Matrix Zeile/Spalte: "); Serial.print(i); Serial.print("/"); Serial.print(spalte); Serial.print(" "); Serial.println(band);
        break;
      }
    }
    
    lcd.setCursor(0, 3);
    lcd.print("NCDXF");
    //lcd.setCursor(9, 3);
    lcd.setCursor(6, 3);
    lcd.print(qrg[band]);
    lcd.setCursor(14, 3);
    lcd.print(callsign[i]);
  }
}
