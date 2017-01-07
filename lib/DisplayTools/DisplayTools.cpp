#include "DisplayTools.h"

DisplayTools* DisplayTools::_instance = 0;

DisplayTools::DisplayTools(int oledReset, int insda, int inscl, int addr) {
  NTP.begin("pool.ntp.org", 1, true);
  NTP.setInterval(63);
  display = new Adafruit_SSD1306(oledReset);
  Wire.begin(insda, inscl);
  display->begin(SSD1306_SWITCHCAPVCC, addr);  // initialize with the I2C addr 0x3D (for the 128x64)
}


DisplayTools* DisplayTools::getInstance(int oledReset, int insda, int inscl, int addr)
{
  if (_instance == 0){
    _instance = new DisplayTools(oledReset, insda, inscl, addr);
  }
  return _instance;
}

DisplayTools* DisplayTools::getInstance()
{
  return _instance;
}

DisplayTools::~DisplayTools() {
  _instance=0;
}

void DisplayTools::setLineOne(String lineOne){
  firstLine = lineOne;
}

void DisplayTools::setLineTwo(String lineTwo){
  secondLine = lineTwo;

}

void DisplayTools::DisplayTools::show(){
  display->clearDisplay();
  display->setTextSize(1);
  display->setTextColor(WHITE);
  display->setCursor(0,0);
  display->println(firstLine);
  display->println(secondLine);
  display->println(NTP.getTimeStr());
  display->display();
}
