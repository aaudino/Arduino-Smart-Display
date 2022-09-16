//#############################
//IMPORT LIBRARIES
//#############################
#include <FastLED.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
//#############################
//SETUP BLUETOOTH
//#############################
// Dataports 
SoftwareSerial BTserial (3, 2);
char command = ' ';
char request = ' ';
// String that contains all of the accepted commands -> mapped to the buttons of the Bluetooth Serial app 
String acceptedComm = "1234567";

//#############################
//Setup Wifi
//#############################
#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)


int status = WL_IDLE_STATUS;
// Initialize the WiFi client library
WiFiClient client;

//#############################
// API CALLS
//#############################

//Weather
char serverWeather[] = "api.openweathermap.org";
char hostWeather[] = "Host: api.openweathermap.org";
char requestWeather[] = "GET /data/2.5/weather?q=vienna,at&APPID=1a8eb15e7fc04b1592e032e80b551f8b";

//STOCK Prices 
char server[]="finnhub.io";
char requestGoogle[] = "GET /api/v1/quote?symbol=GOOG&token=cakatkiad3ier73m2pt0 HTTP/1.1";
char requestFB[] = "GET /api/v1/quote?symbol=META&token=cakatkiad3ier73m2pt0 HTTP/1.1";
char requestTWTR[] = "GET /api/v1/quote?symbol=TWTR&token=cakatkiad3ier73m2pt0 HTTP/1.1";



//#############################
// Setup Display
//#############################
// String that contains all of the accepted commands -> mapped to the buttons of the Bluetooth Serial app 
#define NUM_LEDS  64
#define LED_PIN   6
// Params for width and height
const uint8_t kMatrixWidth = 8;
const uint8_t kMatrixHeight = 8;

#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
CRGB leds[ NUM_LEDS ];
#define LAST_VISIBLE_LED 63
uint8_t XY (uint8_t x, uint8_t y) {
  // any out of bounds address maps to the first hidden pixel
  if ( (x >= kMatrixWidth) || (y >= kMatrixHeight) ) {
    return (LAST_VISIBLE_LED + 1);
  }

  const uint8_t XYTable[] = {
    63,  48,  47,  32,  31,  16,  15,   0,
    62,  49,  46,  33,  30,  17,  14,   1,
    61,  50,  45,  34,  29,  18,  13,   2,
    60,  51,  44,  35,  28,  19,  12,   3,
    59,  52,  43,  36,  27,  20,  11,   4,
    58,  53,  42,  37,  26,  21,  10,   5,
    57,  54,  41,  38,  25,  22,   9,   6,
    56,  55,  40,  39,  24,  23,   8,   7
  };

  uint8_t i = (y * kMatrixWidth) + x;
  uint8_t j = XYTable[i];
  return j;
}

//#############################
// VOID SETUP
//#############################
void setup() {
  //Initialize Serial Communication and the Serial of the Bluetooth Modul 
  Serial.begin(9600);
  BTserial.begin(38400);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  //Define the type of the lightstrip,the data port as well as colors   
  FastLED.addLeds<WS2812B, LED_PIN, GRB> (leds, NUM_LEDS);
   // set default brightness to 10 
  FastLED.setBrightness(10);
}

//#############################
// VOID LOOP
//#############################

void loop() {
  //check for command
  if (BTserial.available()){
   // save the command to variable
   command = BTserial.read();
   Serial.println(command);
   //check if command is part of the acceptedComm String
   if (acceptedComm.indexOf(command) >= 0 ){
   request = command;
   //Switch cases trigger http Request methods
   //Show the warning if the command is not recognized
   //Due to limited space it was only possible to add 5 cases (99%)
    switch(request){
      case '1':
        httpRequestFin(requestTWTR);
        break;
      case '2':
        httpRequestFin(requestFB);
        break;
      case '3':
        httpRequestFin(requestGoogle);
        break;
      case '4':
        httpRequestWeather(hostWeather,requestWeather,serverWeather);
        break; 
      default:
        warning();
        break;
   
  }
  //Display the check symbol to show the user that the panel is ready for new commands 
  delay(5000);
  check();
  
  }
  }

}

//#############################
//Wifi Methods
//#############################
void httpRequestFin (char regF[]){
    Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (client.connectSSL(server, 443)) {
    Serial.println("Connected to server");
    // Make a HTTP request:
    client.println(regF);
    client.println("Host: finnhub.io");
    client.println("Connection: close");
    client.println();
    Serial.println("Request sent");
    Serial.println("REG F");
    Serial.println(regF);
    //Show the internet followed by the check symbol to show that the data was fetched successfully
    internet();
    delay(1000);
    check();
    delay(1000);
    //check what was requested and show the corresponding symbol 
    if(regF == requestTWTR ){
      twitter();
    }
    else if (regF == requestGoogle){
      google();
    }

    else {
      facebook();
      }
    delay(2000);
    //Safe the response to a string 
    String payloadFin = client.readString();
    // Safe the substring which contains the requested data without the headers 
    int endIndex = payloadFin.lastIndexOf('\n\n');
    String content = payloadFin.substring(endIndex+1,payloadFin.length());
    StaticJsonDocument<200> filterFin;
    //set filter for the percental change of the stock price 
    filterFin["dp"]=true;
    StaticJsonDocument<400> docFin;
    //deserialize the json document and filter it to get onyl the requested data 
    deserializeJson(docFin, content, DeserializationOption::Filter(filterFin));
    serializeJsonPretty(docFin, Serial);
    //serialize again and save the value to the dp variable
    float dp = docFin ["dp"];
    Serial.println(dp);
    // display the arrows and adjust brightness depending on the value of "dp"
    if ( dp > 0 ){
      arrowUp();
      if(dp > 1){
        FastLED.setBrightness(50);
        arrowUp();
      }
      
    }
     else if (dp < -1){
        FastLED.setBrightness(50);
        arrowDown();
     }
     else {
      arrowDown();
     }
  }
  //reset the brightness to the original value 
  delay(3000);
  FastLED.setBrightness(10);
}




// this method makes a HTTP connection to the server:
 String httpRequestWeather (char hos[],char req[], char serv[]) {
  // if there's a successful connection:
  if (client.connect(serv, 80)) {
    Serial.println("Connected to server");
    // send the HTTP GET request:
    client.println(req);
    client.println(hos);
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();
    Serial.println("Request sent");
    Serial.println("REG F");
     //Show the internet followed by the check symbol to show that the data was fetched successfully
    internet();
    delay(1000);
    check();
    delay(1000);
    String payload = client.readString();
    StaticJsonDocument<200> filter;
    filter["weather"][0]["main"]=true;
    StaticJsonDocument<400> doc;
    //deserialize the json document and filter it to get onyl the requested data 
    deserializeJson(doc, payload, DeserializationOption::Filter(filter));
    //serialize again and save the value to the weather variable 
    serializeJsonPretty(doc, Serial);
    //Display the weather depending on the http Request
    String weather = doc["weather"][0]["main"];
    Serial.println(weather);

    if(weather == "Clear"){
    sun();
    }
    else if (weather == "Clouds"){
      partly();
    }
     else if (weather == "Drizzle" || weather == "Rain"){
      rainy();
    }
    else{
      // if the weather can  not be displayed 
      warning();
    }
    delay(3000);
    }
}



//#############################
//ICONS - Methods
//#############################

void facebook () {
leds[XY(0, 0)]= CRGB(66, 103, 178);
leds[XY(0, 1)]= CRGB(66, 103, 178);
leds[XY(0, 2)]= CRGB(66, 103, 178);
leds[XY(0, 3)]= CRGB(255, 255, 255);
leds[XY(0, 4)]= CRGB(255, 255, 255);
leds[XY(0, 5)]= CRGB(66, 103, 178);
leds[XY(0, 6)]= CRGB(66, 103, 178);
leds[XY(0, 7)]= CRGB(66, 103, 178);
leds[XY(1, 0)]= CRGB(66, 103, 178);
leds[XY(1, 1)]= CRGB(66, 103, 178);
leds[XY(1, 2)] = CRGB(66, 103, 178);
leds[XY(1, 3)] = CRGB(255, 255, 255);
leds[XY(1, 4)] = CRGB(255, 255, 255);
leds[XY(1, 5)] = CRGB(66, 103, 178);
leds[XY(1, 6)] = CRGB(66, 103, 178);
leds[XY(1, 7)] = CRGB(66, 103, 178);
leds[XY(2, 0)] = CRGB(66, 103, 178);
leds[XY(2, 1)] = CRGB(66, 103, 178);
leds[XY(2, 2)] = CRGB(66, 103, 178);
leds[XY(2, 3)] = CRGB(255, 255, 255);
leds[XY(2, 4)] = CRGB(255, 255, 255);
leds[XY(2, 5)] = CRGB(66, 103, 178);
leds[XY(2, 6)] = CRGB(66, 103, 178);
leds[XY(2, 7)] = CRGB(66, 103, 178);
leds[XY(3, 0)] = CRGB(66, 103, 178);
leds[XY(3, 1)] = CRGB(66, 103, 178);
leds[XY(3, 2)] = CRGB(255, 255, 255);
leds[XY(3, 3)] = CRGB(255, 255, 255);
leds[XY(3, 4)] = CRGB(255, 255, 255);
leds[XY(3, 5)] = CRGB(255, 255, 255);
leds[XY(3, 6)] = CRGB(66, 103, 178);
leds[XY(3, 7)] = CRGB(66, 103, 178);
leds[XY(4, 0)] = CRGB(66, 103, 178);
leds[XY(4, 1)] = CRGB(66, 103, 178);
leds[XY(4, 2)] = CRGB(255, 255, 255);
leds[XY(4, 3)] = CRGB(255, 255, 255);
leds[XY(4, 4)] = CRGB(255, 255, 255);
leds[XY(4, 5)] = CRGB(255, 255, 255);
leds[XY(4, 6)] = CRGB(66, 103, 178);
leds[XY(4, 7)] = CRGB(66, 103, 178);
leds[XY(5, 0)] = CRGB(66, 103, 178);
leds[XY(5, 1)] = CRGB(66, 103, 178);
leds[XY(5, 2)] = CRGB(66, 103, 178);
leds[XY(5, 3)] = CRGB(255, 255, 255);
leds[XY(5, 4)] = CRGB(255, 255, 255);
leds[XY(5, 5)] = CRGB(66, 103, 178);
leds[XY(5, 6)] = CRGB(66, 103, 178);
leds[XY(5, 7)] = CRGB(66, 103, 178);
leds[XY(6, 0)] = CRGB(66, 103, 178);
leds[XY(6, 1)] = CRGB(255, 255, 255);
leds[XY(6, 2)] = CRGB(255, 255, 255);
leds[XY(6, 3)] = CRGB(255, 255, 255);
leds[XY(6, 4)] = CRGB(255, 255, 255);
leds[XY(6, 5)] = CRGB(66, 103, 178);
leds[XY(6, 6)] = CRGB(66, 103, 178);
leds[XY(6, 7)] = CRGB(66, 103, 178);
leds[XY(7, 0)] = CRGB(66, 103, 178);
leds[XY(7, 1)] = CRGB(255, 255, 255);
leds[XY(7, 2)] = CRGB(255, 255, 255);
leds[XY(7, 3)] = CRGB(255, 255, 255);
leds[XY(7, 4)] = CRGB(66, 103, 178);
leds[XY(7, 5)] = CRGB(66, 103, 178);
leds[XY(7, 6)] = CRGB(66, 103, 178);
leds[XY(7, 7)] = CRGB(66, 103, 178);
FastLED.show();
}

void internet (){
  leds[XY(0, 0)]= CRGB(0, 0, 0);
leds[XY(0, 1)]= CRGB(0, 0, 0);
leds[XY(0, 2)]= CRGB(21, 96, 9);
leds[XY(0, 3)]= CRGB(0, 153, 255);
leds[XY(0, 4)]= CRGB(0, 153, 255);
leds[XY(0, 5)]= CRGB(0, 153, 255);
leds[XY(0, 6)]= CRGB(0, 0, 0);
leds[XY(0, 7)]= CRGB(0, 0, 0);
leds[XY(1, 0)]= CRGB(0, 0, 0);
leds[XY(1, 1)]= CRGB(21, 96, 9);
leds[XY(1, 2)] = CRGB(21, 96, 9);
leds[XY(1, 3)] = CRGB(21, 96, 9);
leds[XY(1, 4)] = CRGB(0, 153, 255);
leds[XY(1, 5)] = CRGB(0, 153, 255);
leds[XY(1, 6)] = CRGB(0, 153, 255);
leds[XY(1, 7)] = CRGB(0, 0, 0);
leds[XY(2, 0)] = CRGB(0, 153, 255);
leds[XY(2, 1)] = CRGB(21, 96, 9);
leds[XY(2, 2)] = CRGB(21, 96, 9);
leds[XY(2, 3)] = CRGB(21, 96, 9);
leds[XY(2, 4)] = CRGB(0, 153, 255);
leds[XY(2, 5)] = CRGB(21, 96, 9);
leds[XY(2, 6)] = CRGB(0, 153, 255);
leds[XY(2, 7)] = CRGB(0, 153, 255);
leds[XY(3, 0)] = CRGB(0, 153, 255);
leds[XY(3, 1)] = CRGB(0, 153, 255);
leds[XY(3, 2)] = CRGB(0, 153, 255);
leds[XY(3, 3)] = CRGB(0, 153, 255);
leds[XY(3, 4)] = CRGB(0, 153, 255);
leds[XY(3, 5)] = CRGB(0, 153, 255);
leds[XY(3, 6)] = CRGB(21, 96, 9);
leds[XY(3, 7)] = CRGB(0, 153, 255);
leds[XY(4, 0)] = CRGB(0, 153, 255);
leds[XY(4, 1)] = CRGB(0, 153, 255);
leds[XY(4, 2)] = CRGB(0, 153, 255);
leds[XY(4, 3)] = CRGB(0, 153, 255);
leds[XY(4, 4)] = CRGB(21, 96, 9);
leds[XY(4, 5)] = CRGB(21, 96, 9);
leds[XY(4, 6)] = CRGB(21, 96, 9);
leds[XY(4, 7)] = CRGB(21, 96, 9);
leds[XY(5, 0)] = CRGB(255, 255, 255);
leds[XY(5, 1)] = CRGB(0, 153, 255);
leds[XY(5, 2)] = CRGB(0, 153, 255);
leds[XY(5, 3)] = CRGB(21, 96, 9);
leds[XY(5, 4)] = CRGB(21, 96, 9);
leds[XY(5, 5)] = CRGB(21, 96, 9);
leds[XY(5, 6)] = CRGB(21, 96, 9);
leds[XY(5, 7)] = CRGB(21, 96, 9);
leds[XY(6, 0)] = CRGB(0, 0, 0);
leds[XY(6, 1)] = CRGB(255, 255, 255);
leds[XY(6, 2)] = CRGB(0, 153, 255);
leds[XY(6, 3)] = CRGB(0, 153, 255);
leds[XY(6, 4)] = CRGB(21, 96, 9);
leds[XY(6, 5)] = CRGB(0, 153, 255);
leds[XY(6, 6)] = CRGB(21, 96, 9);
leds[XY(6, 7)] = CRGB(0, 0, 0);
leds[XY(7, 0)] = CRGB(0, 0, 0);
leds[XY(7, 1)] = CRGB(0, 0, 0);
leds[XY(7, 2)] = CRGB(0, 153, 255);
leds[XY(7, 3)] = CRGB(0, 153, 255);
leds[XY(7, 4)] = CRGB(255, 255, 255);
leds[XY(7, 5)] = CRGB(255, 255, 255);
leds[XY(7, 6)] = CRGB(0, 0, 0);
leds[XY(7, 7)] = CRGB(0, 0, 0);
FastLED.show();
}

void twitter(){
  leds[XY(0, 0)]= CRGB(0, 0, 0);
leds[XY(0, 1)]= CRGB(0, 0, 0);
leds[XY(0, 2)]= CRGB(255, 255, 0);
leds[XY(0, 3)]= CRGB(0, 0, 0);
leds[XY(0, 4)]= CRGB(0, 0, 0);
leds[XY(0, 5)]= CRGB(255, 255, 0);
leds[XY(0, 6)]= CRGB(0, 0, 0);
leds[XY(0, 7)]= CRGB(0, 0, 0);
leds[XY(1, 0)]= CRGB(0, 0, 0);
leds[XY(1, 1)]= CRGB(0, 0, 0);
leds[XY(1, 2)] = CRGB(0, 86, 119);
leds[XY(1, 3)] = CRGB(0, 172, 238);
leds[XY(1, 4)] = CRGB(0, 172, 238);
leds[XY(1, 5)] = CRGB(0, 172, 238);
leds[XY(1, 6)] = CRGB(0, 0, 0);
leds[XY(1, 7)] = CRGB(0, 0, 0);
leds[XY(2, 0)] = CRGB(0, 0, 0);
leds[XY(2, 1)] = CRGB(0, 172, 238);
leds[XY(2, 2)] = CRGB(0, 172, 238);
leds[XY(2, 3)] = CRGB(0, 86, 119);
leds[XY(2, 4)] = CRGB(0, 172, 238);
leds[XY(2, 5)] = CRGB(0, 172, 238);
leds[XY(2, 6)] = CRGB(0, 172, 238);
leds[XY(2, 7)] = CRGB(0, 0, 0);
leds[XY(3, 0)] = CRGB(0, 172, 238);
leds[XY(3, 1)] = CRGB(0, 172, 238);
leds[XY(3, 2)] = CRGB(0, 172, 238);
leds[XY(3, 3)] = CRGB(0, 172, 238);
leds[XY(3, 4)] = CRGB(0, 172, 238);
leds[XY(3, 5)] = CRGB(0, 172, 238);
leds[XY(3, 6)] = CRGB(0, 172, 238);
leds[XY(3, 7)] = CRGB(0, 0, 0);
leds[XY(4, 0)] = CRGB(0, 0, 0);
leds[XY(4, 1)] = CRGB(0, 0, 0);
leds[XY(4, 2)] = CRGB(0, 172, 238);
leds[XY(4, 3)] = CRGB(0, 172, 238);
leds[XY(4, 4)] = CRGB(0, 172, 238);
leds[XY(4, 5)] = CRGB(0, 172, 238);
leds[XY(4, 6)] = CRGB(255, 255, 0);
leds[XY(4, 7)] = CRGB(255, 255, 0);
leds[XY(5, 0)] = CRGB(0, 0, 0);
leds[XY(5, 1)] = CRGB(0, 0, 0);
leds[XY(5, 2)] = CRGB(0, 172, 238);
leds[XY(5, 3)] = CRGB(255, 255, 255);
leds[XY(5, 4)] = CRGB(0, 172, 238);
leds[XY(5, 5)] = CRGB(0, 172, 238);
leds[XY(5, 6)] = CRGB(0, 172, 238);
leds[XY(5, 7)] = CRGB(0, 0, 0);
leds[XY(6, 0)] = CRGB(0, 0, 0);
leds[XY(6, 1)] = CRGB(0, 0, 0);
leds[XY(6, 2)] = CRGB(0, 172, 238);
leds[XY(6, 3)] = CRGB(255, 255, 255);
leds[XY(6, 4)] = CRGB(0, 172, 238);
leds[XY(6, 5)] = CRGB(0, 172, 238);
leds[XY(6, 6)] = CRGB(0, 172, 238);
leds[XY(6, 7)] = CRGB(0, 0, 0);
leds[XY(7, 0)] = CRGB(0, 0, 0);
leds[XY(7, 1)] = CRGB(0, 0, 0);
leds[XY(7, 2)] = CRGB(0, 0, 0);
leds[XY(7, 3)] = CRGB(0, 172, 238);
leds[XY(7, 4)] = CRGB(0, 172, 238);
leds[XY(7, 5)] = CRGB(0, 172, 238);
leds[XY(7, 6)] = CRGB(0, 0, 0);
leds[XY(7, 7)] = CRGB(0, 0, 0);
FastLED.show();
}


void arrowDown (){
leds[XY(0, 0)]= CRGB(0, 0, 0);
leds[XY(0, 1)]= CRGB(0, 0, 0);
leds[XY(0, 2)]= CRGB(0, 0, 0);
leds[XY(0, 3)]= CRGB(208, 0, 0);
leds[XY(0, 4)]= CRGB(208, 0, 0);
leds[XY(0, 5)]= CRGB(0, 0, 0);
leds[XY(0, 6)]= CRGB(0, 0, 0);
leds[XY(0, 7)]= CRGB(0, 0, 0);
leds[XY(1, 0)]= CRGB(0, 0, 0);
leds[XY(1, 1)]= CRGB(0, 0, 0);
leds[XY(1, 2)] = CRGB(208, 0, 0);
leds[XY(1, 3)] = CRGB(208, 0, 0);
leds[XY(1, 4)] = CRGB(208, 0, 0);
leds[XY(1, 5)] = CRGB(208, 0, 0);
leds[XY(1, 6)] = CRGB(0, 0, 0);
leds[XY(1, 7)] = CRGB(0, 0, 0);
leds[XY(2, 0)] = CRGB(0, 0, 0);
leds[XY(2, 1)] = CRGB(208, 0, 0);
leds[XY(2, 2)] = CRGB(208, 0, 0);
leds[XY(2, 3)] = CRGB(208, 0, 0);
leds[XY(2, 4)] = CRGB(208, 0, 0);
leds[XY(2, 5)] = CRGB(208, 0, 0);
leds[XY(2, 6)] = CRGB(208, 0, 0);
leds[XY(2, 7)] = CRGB(0, 0, 0);
leds[XY(3, 0)] = CRGB(208, 0, 0);
leds[XY(3, 1)] = CRGB(208, 0, 0);
leds[XY(3, 2)] = CRGB(208, 0, 0);
leds[XY(3, 3)] = CRGB(208, 0, 0);
leds[XY(3, 4)] = CRGB(208, 0, 0);
leds[XY(3, 5)] = CRGB(208, 0, 0);
leds[XY(3, 6)] = CRGB(208, 0, 0);
leds[XY(3, 7)] = CRGB(208, 0, 0);
leds[XY(4, 0)] = CRGB(125, 0, 0);
leds[XY(4, 1)] = CRGB(125, 0, 0);
leds[XY(4, 2)] = CRGB(125, 0, 0);
leds[XY(4, 3)] = CRGB(208, 0, 0);
leds[XY(4, 4)] = CRGB(208, 0, 0);
leds[XY(4, 5)] = CRGB(125, 0, 0);
leds[XY(4, 6)] = CRGB(125, 0, 0);
leds[XY(4, 7)] = CRGB(125, 0, 0);
leds[XY(5, 0)] = CRGB(0, 0, 0);
leds[XY(5, 1)] = CRGB(0, 0, 0);
leds[XY(5, 2)] = CRGB(208, 0, 0);
leds[XY(5, 3)] = CRGB(208, 0, 0);
leds[XY(5, 4)] = CRGB(208, 0, 0);
leds[XY(5, 5)] = CRGB(125, 0, 0);
leds[XY(5, 6)] = CRGB(0, 0, 0);
leds[XY(5, 7)] = CRGB(0, 0, 0);
leds[XY(6, 0)] = CRGB(0, 0, 0);
leds[XY(6, 1)] = CRGB(0, 0, 0);
leds[XY(6, 2)] = CRGB(208, 0, 0);
leds[XY(6, 3)] = CRGB(208, 0, 0);
leds[XY(6, 4)] = CRGB(208, 0, 0);
leds[XY(6, 5)] = CRGB(125, 0, 0);
leds[XY(6, 6)] = CRGB(0, 0, 0);
leds[XY(6, 7)] = CRGB(0, 0, 0);
leds[XY(7, 0)] = CRGB(0, 0, 0);
leds[XY(7, 1)] = CRGB(0, 0, 0);
leds[XY(7, 2)] = CRGB(208, 0, 0);
leds[XY(7, 3)] = CRGB(208, 0, 0);
leds[XY(7, 4)] = CRGB(208, 0, 0);
leds[XY(7, 5)] = CRGB(125, 0, 0);
leds[XY(7, 6)] = CRGB(0, 0, 0);
leds[XY(7, 7)] = CRGB(0, 0, 0);
FastLED.show();  
}

void arrowUp(){
leds[XY(0, 0)]= CRGB(0,0,0);
leds[XY(0, 1)]= CRGB(0, 0, 0);
leds[XY(0, 2)]= CRGB(25, 115, 11);
leds[XY(0, 3)]= CRGB(41, 191, 18);
leds[XY(0, 4)]= CRGB(41, 191, 18);
leds[XY(0, 5)]= CRGB(41, 191, 18);
leds[XY(0, 6)]= CRGB(0, 0, 0);
leds[XY(0, 7)]= CRGB(0, 0, 0);
leds[XY(1, 0)]= CRGB(0, 0, 0);
leds[XY(1, 1)]= CRGB(0, 0, 0);
leds[XY(1, 2)] = CRGB(25, 115, 11);
leds[XY(1, 3)] = CRGB(41, 191, 18);
leds[XY(1, 4)] = CRGB(41, 191, 18);
leds[XY(1, 5)] = CRGB(41, 191, 18);
leds[XY(1, 6)] = CRGB(0, 0, 0);
leds[XY(1, 7)] = CRGB(0, 0, 0);
leds[XY(2, 0)] = CRGB(0, 0, 0);
leds[XY(2, 1)] = CRGB(0, 0, 0);
leds[XY(2, 2)] = CRGB(25, 115, 11);
leds[XY(2, 3)] = CRGB(41, 191, 18);
leds[XY(2, 4)] = CRGB(41, 191, 18);
leds[XY(2, 5)] = CRGB(41, 191, 18);
leds[XY(2, 6)] = CRGB(0, 0, 0);
leds[XY(2, 7)] = CRGB(0, 0, 0);
leds[XY(3, 0)] = CRGB(25, 115, 11);
leds[XY(3, 1)] = CRGB(25, 115, 11);
leds[XY(3, 2)] = CRGB(25, 115, 11);
leds[XY(3, 3)] = CRGB(41, 191, 18);
leds[XY(3, 4)] = CRGB(41, 191, 18);
leds[XY(3, 5)] = CRGB(25, 115, 11);
leds[XY(3, 6)] = CRGB(25, 115, 11);
leds[XY(3, 7)] = CRGB(25, 115, 11);
leds[XY(4, 0)] = CRGB(41, 191, 18);
leds[XY(4, 1)] = CRGB(41, 191, 18);
leds[XY(4, 2)] = CRGB(41, 191, 18);
leds[XY(4, 3)] = CRGB(41, 191, 18);
leds[XY(4, 4)] = CRGB(41, 191, 18);
leds[XY(4, 5)] = CRGB(41, 191, 18);
leds[XY(4, 6)] = CRGB(41, 191, 18);
leds[XY(4, 7)] = CRGB(41, 191, 18);
leds[XY(5, 0)] = CRGB(0, 0, 0);
leds[XY(5, 1)] = CRGB(41, 191, 18);
leds[XY(5, 2)] = CRGB(41, 191, 18);
leds[XY(5, 3)] = CRGB(41, 191, 18);
leds[XY(5, 4)] = CRGB(41, 191, 18);
leds[XY(5, 5)] = CRGB(41, 191, 18);
leds[XY(5, 6)] = CRGB(41, 191, 18);
leds[XY(5, 7)] = CRGB(0, 0, 0);
leds[XY(6, 0)] = CRGB(0, 0, 0);
leds[XY(6, 1)] = CRGB(0, 0, 0);
leds[XY(6, 2)] = CRGB(41, 191, 18);
leds[XY(6, 3)] = CRGB(41, 191, 18);
leds[XY(6, 4)] = CRGB(41, 191, 18);
leds[XY(6, 5)] = CRGB(41, 191, 18);
leds[XY(6, 6)] = CRGB(0, 0, 0);
leds[XY(6, 7)] = CRGB(0, 0, 0);
leds[XY(7, 0)] = CRGB(0, 0, 0);
leds[XY(7, 1)] = CRGB(0, 0, 0);
leds[XY(7, 2)] = CRGB(0, 0, 0);
leds[XY(7, 3)] = CRGB(41, 191, 18);
leds[XY(7, 4)] = CRGB(41, 191, 18);
leds[XY(7, 5)] = CRGB(0, 0, 0);
leds[XY(7, 6)] = CRGB(0, 0, 0);
leds[XY(7, 7)] = CRGB(0, 0, 0);
FastLED.show();
}

void google (){
  leds[XY(0, 0)]= CRGB(0, 0, 0);
leds[XY(0, 1)]= CRGB(66, 133, 244);
leds[XY(0, 2)]= CRGB(15, 157, 88);
leds[XY(0, 3)]= CRGB(15, 157, 88);
leds[XY(0, 4)]= CRGB(15, 157, 88);
leds[XY(0, 5)]= CRGB(15, 157, 88);
leds[XY(0, 6)]= CRGB(244, 180, 0);
leds[XY(0, 7)]= CRGB(0, 0, 0);
leds[XY(1, 0)]= CRGB(66, 133, 244);
leds[XY(1, 1)]= CRGB(15, 157, 88);
leds[XY(1, 2)] = CRGB(15, 157, 88);
leds[XY(1, 3)] = CRGB(15, 157, 88);
leds[XY(1, 4)] = CRGB(15, 157, 88);
leds[XY(1, 5)] = CRGB(15, 157, 88);
leds[XY(1, 6)] = CRGB(15, 157, 88);
leds[XY(1, 7)] = CRGB(244, 180, 0);
leds[XY(2, 0)] = CRGB(66, 133, 244);
leds[XY(2, 1)] = CRGB(66, 133, 244);
leds[XY(2, 2)] = CRGB(0, 0, 0);
leds[XY(2, 3)] = CRGB(0, 0, 0);
leds[XY(2, 4)] = CRGB(0, 0, 0);
leds[XY(2, 5)] = CRGB(0, 0, 0);
leds[XY(2, 6)] = CRGB(244, 180, 0);
leds[XY(2, 7)] = CRGB(244, 180, 0);
leds[XY(3, 0)] = CRGB(66, 133, 244);
leds[XY(3, 1)] = CRGB(66, 133, 244);
leds[XY(3, 2)] = CRGB(66, 133, 244);
leds[XY(3, 3)] = CRGB(66, 133, 244);
leds[XY(3, 4)] = CRGB(0, 0, 0);
leds[XY(3, 5)] = CRGB(0, 0, 0);
leds[XY(3, 6)] = CRGB(244, 180, 0);
leds[XY(3, 7)] = CRGB(244, 180, 0);
leds[XY(4, 0)] = CRGB(66, 133, 244);
leds[XY(4, 1)] = CRGB(66, 133, 244);
leds[XY(4, 2)] = CRGB(66, 133, 244);
leds[XY(4, 3)] = CRGB(66, 133, 244);
leds[XY(4, 4)] = CRGB(0, 0, 0);
leds[XY(4, 5)] = CRGB(0, 0, 0);
leds[XY(4, 6)] = CRGB(244, 180, 0);
leds[XY(4, 7)] = CRGB(244, 180, 0);
leds[XY(5, 0)] = CRGB(0, 0, 0);
leds[XY(5, 1)] = CRGB(0, 0, 0);
leds[XY(5, 2)] = CRGB(0, 0, 0);
leds[XY(5, 3)] = CRGB(0, 0, 0);
leds[XY(5, 4)] = CRGB(0, 0, 0);
leds[XY(5, 5)] = CRGB(0, 0, 0);
leds[XY(5, 6)] = CRGB(219, 69, 55);
leds[XY(5, 7)] = CRGB(219, 69, 55);
leds[XY(6, 0)] = CRGB(0, 0, 0);
leds[XY(6, 1)] = CRGB(219, 69, 55);
leds[XY(6, 2)] = CRGB(219, 69, 55);
leds[XY(6, 3)] = CRGB(219, 69, 55);
leds[XY(6, 4)] = CRGB(219, 69, 55);
leds[XY(6, 5)] = CRGB(219, 69, 55);
leds[XY(6, 6)] = CRGB(219, 69, 55);
leds[XY(6, 7)] = CRGB(0, 0, 0);
leds[XY(7, 0)] = CRGB(0, 0, 0);
leds[XY(7, 1)] = CRGB(0, 0, 0);
leds[XY(7, 2)] = CRGB(219, 69, 55);
leds[XY(7, 3)] = CRGB(219, 69, 55);
leds[XY(7, 4)] = CRGB(219, 69, 55);
leds[XY(7, 5)] = CRGB(219, 69, 55);
leds[XY(7, 6)] = CRGB(0, 0, 0);
leds[XY(7, 7)] = CRGB(0, 0, 0);
FastLED.show();
}


void warning(){
leds[XY(0, 0)]= CRGB(208, 0, 0);
leds[XY(0, 1)]= CRGB(208, 0, 0);
leds[XY(0, 2)]= CRGB(208, 0, 0);
leds[XY(0, 3)]= CRGB(208, 0, 0);
leds[XY(0, 4)]= CRGB(208, 0, 0);
leds[XY(0, 5)]= CRGB(208, 0, 0);
leds[XY(0, 6)]= CRGB(208, 0, 0);
leds[XY(0, 7)]= CRGB(208, 0, 0);
leds[XY(1, 0)]= CRGB(208, 0, 0);
leds[XY(1, 1)]= CRGB(208, 0, 0);
leds[XY(1, 2)] = CRGB(208, 0, 0);
leds[XY(1, 3)] = CRGB(255, 255, 255);
leds[XY(1, 4)] = CRGB(255, 255, 255);
leds[XY(1, 5)] = CRGB(208, 0, 0);
leds[XY(1, 6)] = CRGB(208, 0, 0);
leds[XY(1, 7)] = CRGB(208, 0, 0);
leds[XY(2, 0)] = CRGB(0, 0, 0);
leds[XY(2, 1)] = CRGB(208, 0, 0);
leds[XY(2, 2)] = CRGB(208, 0, 0);
leds[XY(2, 3)] = CRGB(208, 0, 0);
leds[XY(2, 4)] = CRGB(208, 0, 0);
leds[XY(2, 5)] = CRGB(208, 0, 0);
leds[XY(2, 6)] = CRGB(208, 0, 0);
leds[XY(2, 7)] = CRGB(0, 0, 0);
leds[XY(3, 0)] = CRGB(0, 0, 0);
leds[XY(3, 1)] = CRGB(208, 0, 0);
leds[XY(3, 2)] = CRGB(208, 0, 0);
leds[XY(3, 3)] = CRGB(255, 255, 255);
leds[XY(3, 4)] = CRGB(255, 255, 255);
leds[XY(3, 5)] = CRGB(208, 0, 0);
leds[XY(3, 6)] = CRGB(208, 0, 0);
leds[XY(3, 7)] = CRGB(0, 0, 0);
leds[XY(4, 0)] = CRGB(0, 0, 0);
leds[XY(4, 1)] = CRGB(0, 0, 0);
leds[XY(4, 2)] = CRGB(208, 0, 0);
leds[XY(4, 3)] = CRGB(255, 255, 255);
leds[XY(4, 4)] = CRGB(255, 255, 255);
leds[XY(4, 5)] = CRGB(208, 0, 0);
leds[XY(4, 6)] = CRGB(0, 0, 0);
leds[XY(4, 7)] = CRGB(0, 0, 0);
leds[XY(5, 0)] = CRGB(0, 0, 0);
leds[XY(5, 1)] = CRGB(0, 0, 0);
leds[XY(5, 2)] = CRGB(208, 0, 0);
leds[XY(5, 3)] = CRGB(255, 255, 255);
leds[XY(5, 4)] = CRGB(255, 255, 255);
leds[XY(5, 5)] = CRGB(208, 0, 0);
leds[XY(5, 6)] = CRGB(0, 0, 0);
leds[XY(5, 7)] = CRGB(0, 0, 0);
leds[XY(6, 0)] = CRGB(0, 0, 0);
leds[XY(6, 1)] = CRGB(0, 0, 0);
leds[XY(6, 2)] = CRGB(0, 0, 0);
leds[XY(6, 3)] = CRGB(208, 0, 0);
leds[XY(6, 4)] = CRGB(208, 0, 0);
leds[XY(6, 5)] = CRGB(0, 0, 0);
leds[XY(6, 6)] = CRGB(0, 0, 0);
leds[XY(6, 7)] = CRGB(0, 0, 0);
leds[XY(7, 0)] = CRGB(0, 0, 0);
leds[XY(7, 1)] = CRGB(0, 0, 0);
leds[XY(7, 2)] = CRGB(0, 0, 0);
leds[XY(7, 3)] = CRGB(208, 0, 0);
leds[XY(7, 4)] = CRGB(208, 0, 0);
leds[XY(7, 5)] = CRGB(0, 0, 0);
leds[XY(7, 6)] = CRGB(0, 0, 0);
leds[XY(7, 7)] = CRGB(0, 0, 0);
FastLED.show();
}


void sun (){
leds[XY(0, 0)]= CRGB(255, 204, 0);
leds[XY(0, 1)]= CRGB(255, 204, 0);
leds[XY(0, 2)]= CRGB(0, 0, 0);
leds[XY(0, 3)]= CRGB(255, 204, 0);
leds[XY(0, 4)]= CRGB(255, 204, 0);
leds[XY(0, 5)]= CRGB(0, 0, 0);
leds[XY(0, 6)]= CRGB(255, 204, 0);
leds[XY(0, 7)]= CRGB(255, 204, 0);
leds[XY(1, 0)]= CRGB(255, 204, 0);
leds[XY(1, 1)]= CRGB(0, 0, 0);
leds[XY(1, 2)] = CRGB(255, 255, 0);
leds[XY(1, 3)] = CRGB(255, 255, 0);
leds[XY(1, 4)] = CRGB(255, 255, 0);
leds[XY(1, 5)] = CRGB(255, 255, 0);
leds[XY(1, 6)] = CRGB(0, 0, 0);
leds[XY(1, 7)] = CRGB(255, 204, 0);
leds[XY(2, 0)] = CRGB(0, 0, 0);
leds[XY(2, 1)] = CRGB(255, 255, 0);
leds[XY(2, 2)] = CRGB(255, 255, 0);
leds[XY(2, 3)] = CRGB(255, 255, 0);
leds[XY(2, 4)] = CRGB(255, 255, 0);
leds[XY(2, 5)] = CRGB(255, 255, 0);
leds[XY(2, 6)] = CRGB(255, 255, 0);
leds[XY(2, 7)] = CRGB(0, 0, 0);
leds[XY(3, 0)] = CRGB(255, 204, 0);
leds[XY(3, 1)] = CRGB(255, 255, 0);
leds[XY(3, 2)] = CRGB(255, 255, 0);
leds[XY(3, 3)] = CRGB(255, 255, 0);
leds[XY(3, 4)] = CRGB(255, 255, 0);
leds[XY(3, 5)] = CRGB(255, 255, 0);
leds[XY(3, 6)] = CRGB(255, 255, 0);
leds[XY(3, 7)] = CRGB(255, 204, 0);
leds[XY(4, 0)] = CRGB(255, 204, 0);
leds[XY(4, 1)] = CRGB(255, 255, 0);
leds[XY(4, 2)] = CRGB(255, 255, 0);
leds[XY(4, 3)] = CRGB(255, 255, 0);
leds[XY(4, 4)] = CRGB(255, 255, 0);
leds[XY(4, 5)] = CRGB(255, 255, 0);
leds[XY(4, 6)] = CRGB(255, 255, 0);
leds[XY(4, 7)] = CRGB(255, 204, 0);
leds[XY(5, 0)] = CRGB(0, 0, 0);
leds[XY(5, 1)] = CRGB(255, 255, 0);
leds[XY(5, 2)] = CRGB(255, 255, 0);
leds[XY(5, 3)] = CRGB(255, 255, 0);
leds[XY(5, 4)] = CRGB(255, 255, 0);
leds[XY(5, 5)] = CRGB(255, 255, 0);
leds[XY(5, 6)] = CRGB(255, 255, 0);
leds[XY(5, 7)] = CRGB(0, 0, 0);
leds[XY(6, 0)] = CRGB(255, 204, 0);
leds[XY(6, 1)] = CRGB(0, 0, 0);
leds[XY(6, 2)] = CRGB(255, 255, 0);
leds[XY(6, 3)] = CRGB(255, 255, 0);
leds[XY(6, 4)] = CRGB(255, 255, 0);
leds[XY(6, 5)] = CRGB(255, 255, 0);
leds[XY(6, 6)] = CRGB(0, 0, 0);
leds[XY(6, 7)] = CRGB(255, 204, 0);
leds[XY(7, 0)] = CRGB(255, 204, 0);
leds[XY(7, 1)] = CRGB(255, 204, 0);
leds[XY(7, 2)] = CRGB(0, 0, 0);
leds[XY(7, 3)] = CRGB(255, 204, 0);
leds[XY(7, 4)] = CRGB(255, 204, 0);
leds[XY(7, 5)] = CRGB(0, 0, 0);
leds[XY(7, 6)] = CRGB(255, 204, 0);
leds[XY(7, 7)] = CRGB(255, 204, 0);
FastLED.show();
}
void partly (){
 leds[XY(0, 0)]= CRGB(192, 192, 192);
leds[XY(0, 1)]= CRGB(192, 192, 192);
leds[XY(0, 2)]= CRGB(192, 192, 192);
leds[XY(0, 3)]= CRGB(192, 192, 192);
leds[XY(0, 4)]= CRGB(192, 192, 192);
leds[XY(0, 5)]= CRGB(192, 192, 192);
leds[XY(0, 6)]= CRGB(128, 129, 128);
leds[XY(0, 7)]= CRGB(128, 129, 128);
leds[XY(1, 0)]= CRGB(192, 192, 192);
leds[XY(1, 1)]= CRGB(192, 192, 192);
leds[XY(1, 2)] = CRGB(192, 192, 192);
leds[XY(1, 3)] = CRGB(192, 192, 192);
leds[XY(1, 4)] = CRGB(192, 192, 192);
leds[XY(1, 5)] = CRGB(128, 129, 128);
leds[XY(1, 6)] = CRGB(255, 204, 0);
leds[XY(1, 7)] = CRGB(0, 0, 0);
leds[XY(2, 0)] = CRGB(192, 192, 192);
leds[XY(2, 1)] = CRGB(192, 192, 192);
leds[XY(2, 2)] = CRGB(192, 192, 192);
leds[XY(2, 3)] = CRGB(192, 192, 192);
leds[XY(2, 4)] = CRGB(128, 129, 128);
leds[XY(2, 5)] = CRGB(255, 255, 0);
leds[XY(2, 6)] = CRGB(255, 255, 0);
leds[XY(2, 7)] = CRGB(255, 204, 0);
leds[XY(3, 0)] = CRGB(192, 192, 192);
leds[XY(3, 1)] = CRGB(192, 192, 192);
leds[XY(3, 2)] = CRGB(192, 192, 192);
leds[XY(3, 3)] = CRGB(192, 192, 192);
leds[XY(3, 4)] = CRGB(128, 129, 128);
leds[XY(3, 5)] = CRGB(255, 255, 0);
leds[XY(3, 6)] = CRGB(255, 255, 0);
leds[XY(3, 7)] = CRGB(255, 204, 0);
leds[XY(4, 0)] = CRGB(128, 129, 128);
leds[XY(4, 1)] = CRGB(192, 192, 192);
leds[XY(4, 2)] = CRGB(192, 192, 192);
leds[XY(4, 3)] = CRGB(128, 128, 128);
leds[XY(4, 4)] = CRGB(255, 255, 0);
leds[XY(4, 5)] = CRGB(255, 255, 0);
leds[XY(4, 6)] = CRGB(255, 255, 0);
leds[XY(4, 7)] = CRGB(255, 204, 0);
leds[XY(5, 0)] = CRGB(0, 0, 0);
leds[XY(5, 1)] = CRGB(128, 129, 128);
leds[XY(5, 2)] = CRGB(128, 129, 128);
leds[XY(5, 3)] = CRGB(128, 129, 128);
leds[XY(5, 4)] = CRGB(255, 255, 0);
leds[XY(5, 5)] = CRGB(255, 255, 0);
leds[XY(5, 6)] = CRGB(255, 255, 0);
leds[XY(5, 7)] = CRGB(255, 204, 0);
leds[XY(6, 0)] = CRGB(0, 0, 0);
leds[XY(6, 1)] = CRGB(0, 0, 0);
leds[XY(6, 2)] = CRGB(255, 204, 0);
leds[XY(6, 3)] = CRGB(255, 255, 0);
leds[XY(6, 4)] = CRGB(255, 255, 0);
leds[XY(6, 5)] = CRGB(255, 255, 0);
leds[XY(6, 6)] = CRGB(255, 204, 0);
leds[XY(6, 7)] = CRGB(0, 0, 0);
leds[XY(7, 0)] = CRGB(0, 0, 0);
leds[XY(7, 1)] = CRGB(0, 0, 0);
leds[XY(7, 2)] = CRGB(0, 0, 0);
leds[XY(7, 3)] = CRGB(255, 204, 0);
leds[XY(7, 4)] = CRGB(255, 204, 0);
leds[XY(7, 5)] = CRGB(255, 204, 0);
leds[XY(7, 6)] = CRGB(0, 0, 0);
leds[XY(7, 7)] = CRGB(0, 0, 0);
FastLED.show();
}

void rainy(){
  leds[XY(0, 0)]= CRGB(0, 107, 125);
leds[XY(0, 1)]= CRGB(0, 0, 0);
leds[XY(0, 2)]= CRGB(0, 0, 0);
leds[XY(0, 3)]= CRGB(0, 107, 125);
leds[XY(0, 4)]= CRGB(0, 0, 0);
leds[XY(0, 5)]= CRGB(0, 0, 0);
leds[XY(0, 6)]= CRGB(0, 107, 125);
leds[XY(0, 7)]= CRGB(0, 0, 0);
leds[XY(1, 0)]= CRGB(0, 0, 0);
leds[XY(1, 1)]= CRGB(0, 0, 0);
leds[XY(1, 2)] = CRGB(0, 0, 0);
leds[XY(1, 3)] = CRGB(0, 0, 0);
leds[XY(1, 4)] = CRGB(0, 0, 0);
leds[XY(1, 5)] = CRGB(0, 0, 0);
leds[XY(1, 6)] = CRGB(0, 0, 0);
leds[XY(1, 7)] = CRGB(0, 0, 0);
leds[XY(2, 0)] = CRGB(0, 0, 0);
leds[XY(2, 1)] = CRGB(0, 107, 125);
leds[XY(2, 2)] = CRGB(0, 0, 0);
leds[XY(2, 3)] = CRGB(0, 0, 0);
leds[XY(2, 4)] = CRGB(0, 107, 125);
leds[XY(2, 5)] = CRGB(0, 0, 0);
leds[XY(2, 6)] = CRGB(0, 0, 0);
leds[XY(2, 7)] = CRGB(0, 107, 125);
leds[XY(3, 0)] = CRGB(0, 0, 0);
leds[XY(3, 1)] = CRGB(0, 0, 0);
leds[XY(3, 2)] = CRGB(0, 0, 0);
leds[XY(3, 3)] = CRGB(0, 0, 0);
leds[XY(3, 4)] = CRGB(0, 0, 0);
leds[XY(3, 5)] = CRGB(0, 0, 0);
leds[XY(3, 6)] = CRGB(0, 0, 0);
leds[XY(3, 7)] = CRGB(0, 0, 0);
leds[XY(4, 0)] = CRGB(128, 128, 128);
leds[XY(4, 1)] = CRGB(128, 128, 128);
leds[XY(4, 2)] = CRGB(128, 128, 128);
leds[XY(4, 3)] = CRGB(128, 128, 128);
leds[XY(4, 4)] = CRGB(128, 128, 128);
leds[XY(4, 5)] = CRGB(128, 128, 128);
leds[XY(4, 6)] = CRGB(128, 128, 128);
leds[XY(4, 7)] = CRGB(128, 128, 128);
leds[XY(5, 0)] = CRGB(0, 0, 0);
leds[XY(5, 1)] = CRGB(128, 128, 128);
leds[XY(5, 2)] = CRGB(128, 128, 128);
leds[XY(5, 3)] = CRGB(128, 128, 128);
leds[XY(5, 4)] = CRGB(128, 128, 128);
leds[XY(5, 5)] = CRGB(128, 128, 128);
leds[XY(5, 6)] = CRGB(0, 0, 0);
leds[XY(5, 7)] = CRGB(0, 0, 0);
leds[XY(6, 0)] = CRGB(0, 0, 0);
leds[XY(6, 1)] = CRGB(128, 128, 128);
leds[XY(6, 2)] = CRGB(128, 128, 128);
leds[XY(6, 3)] = CRGB(128, 128, 128);
leds[XY(6, 4)] = CRGB(128, 128, 128);
leds[XY(6, 5)] = CRGB(128, 128, 128);
leds[XY(6, 6)] = CRGB(0, 0, 0);
leds[XY(6, 7)] = CRGB(0, 0, 0);
leds[XY(7, 0)] = CRGB(0, 0, 0);
leds[XY(7, 1)] = CRGB(0, 0, 0);
leds[XY(7, 2)] = CRGB(128, 128, 128);
leds[XY(7, 3)] = CRGB(128, 128, 128);
leds[XY(7, 4)] = CRGB(128, 128, 128);
leds[XY(7, 5)] = CRGB(0, 0, 0);
leds[XY(7, 6)] = CRGB(0, 0, 0);
leds[XY(7, 7)] = CRGB(0, 0, 0);
FastLED.show();
}



void check(){
  leds[XY(0, 0)]= CRGB(0, 0, 0);
leds[XY(0, 1)]= CRGB(0, 0, 0);
leds[XY(0, 2)]= CRGB(0, 0, 0);
leds[XY(0, 3)]= CRGB(0, 0, 0);
leds[XY(0, 4)]= CRGB(21, 96, 9);
leds[XY(0, 5)]= CRGB(41, 191, 18);
leds[XY(0, 6)]= CRGB(0, 0, 0);
leds[XY(0, 7)]= CRGB(0, 0, 0);
leds[XY(1, 0)]= CRGB(0, 0, 0);
leds[XY(1, 1)]= CRGB(0, 0, 0);
leds[XY(1, 2)] = CRGB(0, 0, 0);
leds[XY(1, 3)] = CRGB(21, 96, 9);
leds[XY(1, 4)] = CRGB(41, 191, 18);
leds[XY(1, 5)] = CRGB(41, 191, 18);
leds[XY(1, 6)] = CRGB(21, 96, 9);
leds[XY(1, 7)] = CRGB(0, 0, 0);
leds[XY(2, 0)] = CRGB(0, 0, 0);
leds[XY(2, 1)] = CRGB(0, 0, 0);
leds[XY(2, 2)] = CRGB(0, 0, 0);
leds[XY(2, 3)] = CRGB(41, 191, 18);
leds[XY(2, 4)] = CRGB(41, 191, 18);
leds[XY(2, 5)] = CRGB(21, 96, 9);
leds[XY(2, 6)] = CRGB(41, 191, 18);
leds[XY(2, 7)] = CRGB(21, 96, 9);
leds[XY(3, 0)] = CRGB(0, 0, 0);
leds[XY(3, 1)] = CRGB(0, 0, 0);
leds[XY(3, 2)] = CRGB(21, 96, 9);
leds[XY(3, 3)] = CRGB(41, 191, 18);
leds[XY(3, 4)] = CRGB(0, 0, 0);
leds[XY(3, 5)] = CRGB(0, 0, 0);
leds[XY(3, 6)] = CRGB(41, 191, 18);
leds[XY(3, 7)] = CRGB(41, 191, 18);
leds[XY(4, 0)] = CRGB(0, 0, 0);
leds[XY(4, 1)] = CRGB(21, 96, 9);
leds[XY(4, 2)] = CRGB(41, 191, 18);
leds[XY(4, 3)] = CRGB(0, 0, 0);
leds[XY(4, 4)] = CRGB(0, 0, 0);
leds[XY(4, 5)] = CRGB(0, 0, 0);
leds[XY(4, 6)] = CRGB(0, 0, 0);
leds[XY(4, 7)] = CRGB(0, 0, 0);
leds[XY(5, 0)] = CRGB(21, 96, 9);
leds[XY(5, 1)] = CRGB(41, 191, 18);
leds[XY(5, 2)] = CRGB(21, 96, 9);
leds[XY(5, 3)] = CRGB(0, 0, 0);
leds[XY(5, 4)] = CRGB(0, 0, 0);
leds[XY(5, 5)] = CRGB(0, 0, 0);
leds[XY(5, 6)] = CRGB(0, 0, 0);
leds[XY(5, 7)] = CRGB(0, 0, 0);
leds[XY(6, 0)] = CRGB(41, 191, 18);
leds[XY(6, 1)] = CRGB(41, 191, 18);
leds[XY(6, 2)] = CRGB(0, 0, 0);
leds[XY(6, 3)] = CRGB(0, 0, 0);
leds[XY(6, 4)] = CRGB(0, 0, 0);
leds[XY(6, 5)] = CRGB(0, 0, 0);
leds[XY(6, 6)] = CRGB(0, 0, 0);
leds[XY(6, 7)] = CRGB(0, 0, 0);
leds[XY(7, 0)] = CRGB(41, 191, 18);
leds[XY(7, 1)] = CRGB(21, 96, 9);
leds[XY(7, 2)] = CRGB(0, 0, 0);
leds[XY(7, 3)] = CRGB(0, 0, 0);
leds[XY(7, 4)] = CRGB(0, 0, 0);
leds[XY(7, 5)] = CRGB(0, 0, 0);
leds[XY(7, 6)] = CRGB(0, 0, 0);
leds[XY(7, 7)] = CRGB(0, 0, 0);
FastLED.show();
}
