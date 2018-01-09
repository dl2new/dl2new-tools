/*
 * Statusermittlung ueber Fotodiode
 * Darstellung auf Web-Seite
 * 
 * DL2NEW, V1, 06.01.2017
 * 
 * Tmp: const int led = 13; D7
 * 
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <NTPtimeESP.h>

const char *ssid = "xxx";
const char *password = "xxxf";

/* NodeMCU Pin auf IDE: D2=4 */
const int photo_pin = 4; /* Fotodiode: Wenn auf GND, dann Licht empfangen */

/* Waschmaschinen-LED - Erfassung mit Fotodiode */
int led_an_aus_blink = 0; /* an=0 aus=1 blink=2 */
int led_status = 0;
int led_status_pre = 0;
char led_status_c[50];
long loop_cnt = 0;
int change_cnt = 0;

int second_pre; /* Hilfsvariable, damit der Sprung von 60 auf 0 erkannt wird */

NTPtime NTPch("ch.pool.ntp.org");
long ntp_cnt = 0; /* alle paar Minuten die Zeit neu über ntp holen */

ESP8266WebServer server ( 80 );

strDateTime dateTime;
long previousMillis;
long currentMillis;
long diffMillis;


void handleRoot() {
	digitalWrite ( LED_BUILTIN, 0 );
	char temp[800];
	int sec = millis() / 1000;
	int min = sec / 60;
	int hr = min / 60;
  

  if(led_an_aus_blink == 0) {
    strcpy(led_status_c, "Laeuft noch");
  }
  else {
    if(led_an_aus_blink == 1) {
        strcpy(led_status_c, "Fertig oder Aus");
      }
      else {
        strcpy(led_status_c, "Nachlegen oder Stoerung");
      }
  }
  
	snprintf ( temp, 800,

"<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP8266 DL2NEW 1</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Waschmaschinenstatus via ESP8266</h1>\
    <p>Aktuelle Zeit: %02d:%02d</p>\
    <p>Laufzeit Modul: %02d:%02d:%02d</p>\
    <p>Maschine Status: %s (%02d)</p>\
    <p>\
  </body>\
</html>",

		dateTime.hour, dateTime.minute, hr, min % 60, sec % 60, led_status_c, led_an_aus_blink
	);
	server.send ( 200, "text/html", temp );
	digitalWrite ( LED_BUILTIN, 1 );
}

void handleNotFound() {
	digitalWrite ( LED_BUILTIN, 0 );
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";

	for ( uint8_t i = 0; i < server.args(); i++ ) {
		message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
	}

	server.send ( 404, "text/plain", message );
	digitalWrite ( LED_BUILTIN, 1 );
}

void setup ( void ) {
	pinMode ( photo_pin, INPUT_PULLUP );
	pinMode ( LED_BUILTIN, OUTPUT );
	digitalWrite ( LED_BUILTIN, 1 );
	Serial.begin ( 115200 );
	WiFi.begin ( ssid, password );
	Serial.println ( "" );

	// Wait for connection
	while ( WiFi.status() != WL_CONNECTED ) {
		delay ( 500 );
		Serial.print ( "." );
	}

	Serial.println ( "" );
	Serial.print ( "Connected to " );
	Serial.println ( ssid );
	Serial.print ( "IP address: " );
	Serial.println ( WiFi.localIP() );

	if ( MDNS.begin ( "esp8266" ) ) {
		Serial.println ( "MDNS responder started" );
	}

	server.on ( "/", handleRoot );
	server.on ( "/inline", []() {
		server.send ( 200, "text/plain", "this works as well" );
	} );
	server.onNotFound ( handleNotFound );
	server.begin();
	Serial.println ( "HTTP server started" );

  Serial.println ( "NTP request" );
  ntp();
}

void loop ( void ) {

  loop_cnt += 1;
  ntp_cnt += 1;
  
  /* Status Fotodiode ermitteln */
  led_status = digitalRead(photo_pin);

  /* 100.000 loop-Durchläude = 1s */
  /* 5s beobachten und die Impulse zaehlen */
  if(loop_cnt < 500000) {
    if(led_status != led_status_pre) {
      change_cnt += 1;
      led_status_pre = led_status;
    }
  }
  else {
    if(change_cnt < 50) { /* wenige Impulse = LED aus/dunkel */
      led_an_aus_blink = 1;
    }
    else {
      if(change_cnt < 750) { /* um 500 Impulse = LED blinkt */
        led_an_aus_blink = 2;
      }
      else {
        led_an_aus_blink = 0; /* ueber 1000 Impulse = LED an */
      }
    }

    loop_cnt = 0;
    change_cnt = 0;
    updateTime();
  }

  /* nach 2 Minuten Zeit neu abgleichen */
  /* 100.000 = 1s   6.000.000 = 60s*/   
  if(ntp_cnt == 12000000) {
    char currentTime[10];
    sprintf(currentTime, "ActTime %02d:%02d:%02d", dateTime.hour, dateTime.minute, dateTime.second);
    Serial.println(currentTime);
    Serial.println("NTP Zeit neu abgelichen");
    ntp();
    ntp_cnt = 0;
  }
  
	server.handleClient();
}

void updateTime()
{
  currentMillis = millis();
  diffMillis = currentMillis - previousMillis;

  second_pre = dateTime.second;
    
  dateTime.second = (dateTime.second + (diffMillis / 1000)) % 60;
  
  if (dateTime.second < second_pre) {
    dateTime.minute = (dateTime.minute + 1) % 60;
    if (dateTime.minute == 0) {
      dateTime.hour = (dateTime.hour + 1) % 24;
    }
  }
  previousMillis = currentMillis;
}

void ntp()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    // first parameter: Time zone in floating point (for India); second parameter: 1 for European summer time; 2 for US daylight saving time (not implemented yet)
    dateTime = NTPch.getNTPtime(1.0, 1);

    while(!dateTime.valid){ //DL2NEW: sometime ntp fails - while until return is valid
      dateTime = NTPch.getNTPtime(1.0, 1);
      delay(100);
    }

    NTPch.printDateTime(dateTime);

    previousMillis = millis();
  }
}
