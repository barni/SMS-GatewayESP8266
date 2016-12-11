/**
Install aRest API https://github.com/marcoschwartz/aREST.git  platformio lib install 429
REST API for SMS, e.g. http://sms.local/sms?params=111111^Dies ist ein Test2
 http://192.168.1.63/sms?params=1111^Dies ist ein Test2
*/

#define DEBUG
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <aREST.h>
#include <Tools.h>
#include <A6libcust.h>

A6libcust A6l(D6, D5); // tx GPIO12, GPIO14
int unreadSMSLocs[30] = {0};
int unreadSMSNum = 0;
int disconnected = 0;
bool led=true;
SMSmessage sms;


// Create aREST instance
aREST rest = aREST();

// WiFi parameters
const char* ssid = "-------";
const char* password = "---------";
const String messageNumber = "0123456790";

// The port to listen for incoming TCP connections
#define LISTEN_PORT           80
#define ACTIVE_LED            D2 // GPIO4

// Create an instance of the server
WiFiServer server(LISTEN_PORT);

void setup() {
  pinMode(ACTIVE_LED, OUTPUT);
  switchLed();
  WiFi.hostname("sms");
  if (!MDNS.begin("sms")) {
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");
  MDNS.addService("ws", "tcp", 81);
  MDNS.addService("http", "tcp", 80);
  Serial.begin(115200);
  sdelay(500);
  Serial.println("Setup start");
  // A6 GSM
  // Power-cycle the module to reset it.
  A6l.powerCycle(D1); // GPIO05
  //A6l.blockUntilReady(9600);
  A6l.blockUntilReady(115200);

  A6l.test();

  // Connect to WiFi
  reconnectWifi();
  while (WiFi.status() != WL_CONNECTED) {
    sdelay(500);
    Serial.print(".");
    disconnected=0;
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());

  // rest
  // Function to be exposed
 rest.function("sms",sendSMS);

 // Give name & ID to the device (ID should be 6 characters long)
 rest.set_id("123456");
 rest.set_name("SMS-Gateway");
 String tmp = "SMS-Gateway gestartet. IP: " + WiFi.localIP().toString() +
    " http://" + WiFi.hostname() + ".local";
 A6l.sendSMS(messageNumber, tmp);
 switchLed();
 Serial.println("Setup done");
}

void loop() {
  if (disconnected == 1 && WiFi.status() == WL_CONNECTED){
     Serial.print(String("[WIFI] IP: "));
     Serial.println(WiFi.localIP());
     disconnected = 0;
   }
   if ( disconnected == 0){
    // Handle REST calls
    WiFiClient client = server.available();
    if (client) {
      while(!client.available()){
        sdelay(1);
      }
      rest.handle(client);
    }
  }

  // SMSmessage
  bool conn = A6l.isConnected();
  if (conn){
    callInfo cinfo = A6l.checkCallStatus();
    if (cinfo.direction == DIR_INCOMING) {
       if (cinfo.number == messageNumber)
           A6l.sendSMS(messageNumber, "I can't come to the phone right now, I'm a machine.");
        A6l.hangUp();
    }
  }
  if (conn && (disconnected == 0)){
    switchLed();
  }else{
    Serial.println("Error:");
    Serial.print("GSM conntected: ");
    Serial.println(conn);
    Serial.print("Wifi disconnected: ");
    Serial.println(disconnected);
  }


  // Wait a bit before scanning again
  sdelay(2000);
}

int sendSMS(String command){
  String number = messageNumber;
  String message = httpDecode(command);
  int delimiter = message.indexOf('^');
  if (delimiter > -1){
    number = message.substring(0,delimiter);
    message = message.substring(delimiter+1);
  }

  Serial.print("Send SMS to: ");
  Serial.print(number);
  Serial.print(" Message: ");
  Serial.println(message);
  Serial.println(command);

  return A6l.sendSMS(number, message);
}

void switchLed(){
  if (led){
      digitalWrite(ACTIVE_LED, HIGH);
  }else{
      digitalWrite(ACTIVE_LED, LOW);
  }
  led = !led;
}

void reconnectWifi() {
  WiFi.begin(ssid, password);
}
void onDisconnected(const WiFiEventStationModeDisconnected& event)
{
    disconnected = 1;
    reconnectWifi();
}
