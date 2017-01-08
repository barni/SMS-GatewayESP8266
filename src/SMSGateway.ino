/**
Install aRest API https://github.com/marcoschwartz/aREST.git  platformio lib install 429
Install NtpClientLib platformio lib install 727
Install Adafruit SSD1306 platformio lib install 135
Install timelib platformio lib install 44
Install Wifimanager platformio lib install 567 https://github.com/tzapu/WiFiManager
REST API for SMS, e.g. http://sms.local/sms?params=111111^Dies ist ein Test2
 http://192.168.1.63/sms?params=1111^Dies ist ein Test2
*/

//#define DEBUG 1

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <aREST.h>
#include <Tools.h>
#include <DisplayTools.h>
#include <A6libcust.h>
#include <WiFiManager.h>

// Default SMS number
const String messageNumber = "012356789000";
// The port to listen for incoming TCP connections
#define LISTEN_PORT 80
#define I2C_SSD_ADR 0x3C
// GPIOs
#define OLED_RESET D0 // GPIO16 ==> DUMMY for OLED Reset
#define ACTIVE_LED D0 // GPIO16/D0
#define _SDA SDA // SDA/GPIO4/D2
#define _SCL SCL // SCL/GPIO5/D1
A6libcust A6l(D6, D5); // tx GPIO12, GPIO14

int unreadSMSLocs[30] = {0};
int unreadSMSNum = 0;
int disconnected = 1;
bool led=true;
SMSmessage sms;

// Create aREST instance
aREST rest = aREST();

int smsSend = 0;

// Create an instance of the server
WiFiServer server(LISTEN_PORT);

void setup() {
  Serial.begin(115200);
  Serial.println("Setup start");
  DisplayTools* displayTools = DisplayTools::getInstance(OLED_RESET, _SDA, _SCL, I2C_SSD_ADR);
  displayTools->setLineOne("Starting Wifi...");
  displayTools->show();
  pinMode(ACTIVE_LED, OUTPUT);
  switchLed();
  WiFi.hostname("sms");
  if (!MDNS.begin("sms")) {
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");
  MDNS.addService("ws", "tcp", 81);
  MDNS.addService("http", "tcp", 80);

  sdelay(500);

    // Connect to WiFi
  reconnectWifi();
  while (WiFi.status() != WL_CONNECTED) {
    sdelay(500);
    Serial.print(".");
  }
  disconnected=0;
  Serial.println("WiFi connected");
  displayTools->setLineOne("WiFi connected");
  displayTools->setLineTwo("Starting GSM");
  displayTools->show();

  // A6 GSM
  // Power-cycle the module to reset it.
  A6l.powerCycle(D7); // GPIO13
  //A6l.blockUntilReady(9600);
  A6l.blockUntilReady(115200);

  A6l.test();
  displayTools->setLineOne("WiFi and GSM connected");
  displayTools->setLineTwo("Sending initial SMS");
  displayTools->show();

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());

  // rest
  // Function to be exposed
 rest.function("sms",sendSMS);
 rest.function("reset",resetWifi);

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
  DisplayTools* displayTools = DisplayTools::getInstance();

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
    displayTools->setLineOne("OK IP: " + WiFi.localIP().toString());
    displayTools->setLineTwo("SMS send: " + String(smsSend));
  }else{
    Serial.println("Error:");
    Serial.print("GSM conntected: ");
    Serial.println(conn);
    Serial.print("Wifi disconnected: ");
    Serial.println(disconnected);
    displayTools->setLineOne("ERROR");
    displayTools->setLineTwo("GSM: " + String(conn) + "Wifi: " + String(disconnected));
  }


  // Wait a bit before scanning again
  sdelay(2000);
}

int resetWifi(String vommand){
  WiFiManager wifiManager;
  //reset saved settings
  wifiManager.resetSettings();
  return 0;
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
  smsSend++;
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

void configModeCallback (WiFiManager *myWiFiManager) {
  DisplayTools* displayTools = DisplayTools::getInstance();
  displayTools->setLineOne("Config mode...");
  displayTools->setLineTwo(myWiFiManager->getConfigPortalSSID());
  displayTools->show();
}

void reconnectWifi() {
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset saved settings
  //wifiManager.resetSettings();

  wifiManager.setAPCallback(configModeCallback);


  //set custom ip for portal
  wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("SMS-Gateway-Config-AP");
  //or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();


  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  disconnected = 0;

}
void onDisconnected(const WiFiEventStationModeDisconnected& event)
{
    disconnected = 1;
    reconnectWifi();
}
