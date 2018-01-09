#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
 
const char* ssid = "xxx";
const char* password = "xxx";
 
void setup () {
 
  Serial.begin(115200);
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
 
    delay(1000);
    Serial.print("Connecting..");
 
  }
 
}
 
void loop() {
 
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
 
    HTTPClient http;  //Declare an object of class HTTPClient
 
    http.begin("http://192.168.2.23/");  //Specify request destination
    int httpCode = http.GET();                                                                  //Send the request
 
    if (httpCode > 0) { //Check the returning code
 
      String payload = http.getString();   //Get the request response payload

      int vonPos = payload.indexOf("("); 
      int bisPos = payload.indexOf(")");
      Serial.print("Code: ");
      Serial.println(payload.substring(vonPos + 2, bisPos));
 
    }
 
    http.end();   //Close connection
 
  }
 
  delay(10000);    //Send a request every 10 seconds
 
}
