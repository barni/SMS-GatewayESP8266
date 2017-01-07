#ifndef displayTools_h
#define displayTools_h
#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <TimeLib.h>
#include <NtpClientLib.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


class DisplayTools {
private:
  static DisplayTools* _instance;
  String firstLine;
  String secondLine;
  Adafruit_SSD1306* display;
  DisplayTools() {};
  DisplayTools(int oledReset, int insda, int inscl, int addr);
  DisplayTools( const DisplayTools& );
  DisplayTools & operator = (const DisplayTools &);

public:
  static DisplayTools* getInstance(int oledReset, int insda, int inscl, int addr);

  static DisplayTools* getInstance();
  ~DisplayTools();
  void setLineOne(String lineOne);
  void setLineTwo(String lineTwo);
  void show();
};


#endif
