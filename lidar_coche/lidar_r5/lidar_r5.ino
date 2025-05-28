#include "Arduino_GigaDisplay_GFX.h"
#include "ld06.h"
#include "ArduinoGraphics.h"

/*
Connections:
L   White   ----------\/-----Yellow---- Not connected-----------A G
I   Yellow  ----------/\-----White----- Pin0--------------------R I
D   Red     -------\         Void                               D G
A   Blue    ----\    --------Red------- 5V (3.3Vnot working)----U A
R                ------------Blue------ GND---------------------INO                 
Color conversion: https://rgbcolorpicker.com/565
*/

int diameter = 400;

GigaDisplay_GFX Display;

float centerX = Display.width() / 2;
float centerY = Display.height() / 2;
LD06 ld06(Serial1);

void setup() {
  Serial.begin(115200);
  Display.begin();
  ld06.init();
  ld06.disableCRC();

  float x1 = 0;
  float y1 = 0;
  float x2 = 0;
  float y2 = 0;

  Display.fillScreen(0xffff);
  Display.drawCircle(centerX, centerY, diameter / 2, 0x0410);

  for (int i = 0; i <= 360; i = i + 1) {
    x1 = (diameter / 2 - 5) * cosf(i * 2.0 * PI / 360.0);
    y1 = (diameter / 2 - 5) * sinf(i * 2.0 * PI / 360.0);
    x2 = (diameter / 2 + 5) * cosf(i * 2.0 * PI / 360.0);
    y2 = (diameter / 2 + 5) * sinf(i * 2.0 * PI / 360.0);
    Display.drawLine(centerX + x1, centerY + y1, centerX + x2, centerY + y2, 0x0410);
  }
  for (int i = 0; i <= 360; i = i + 5) {
    x1 = (diameter / 2 - 10) * cosf(i * 2.0 * PI / 360.0);
    y1 = (diameter / 2 - 10) * sinf(i * 2.0 * PI / 360.0);
    x2 = (diameter / 2 + 10) * cosf(i * 2.0 * PI / 360.0);
    y2 = (diameter / 2 + 10) * sinf(i * 2.0 * PI / 360.0);
    Display.drawLine(centerX + x1, centerY + y1, centerX + x2, centerY + y2, 0x0410);
  }
  for (int i = 0; i <= 360; i = i + 10) {
    x1 = (diameter / 2 - 15) * cosf(i * 2.0 * PI / 360.0);
    y1 = (diameter / 2 - 15) * sinf(i * 2.0 * PI / 360.0);
    x2 = (diameter / 2 + 15) * cosf(i * 2.0 * PI / 360.0);
    y2 = (diameter / 2 + 15) * sinf(i * 2.0 * PI / 360.0);
    Display.drawLine(centerX + x1, centerY + y1, centerX + x2, centerY + y2, 0x0410);
  }
}

void loop(void) {
  if (ld06.readScan()) {            
    uint16_t n = ld06.getNbPointsInScan();  
    int maxDist = 3000; //mm

//    float oldX = centerX  + cosf(ld06.getPoints(n-1)->angle * 2.0 * PI / 360.0) * map(min(ld06.getPoints(n-1)->distance, maxDist), 0, maxDist, 0, diameter/2 - 10);
//    float oldY = centerY + sinf(ld06.getPoints(n-1)->angle * 2.0 * PI / 360.0) * map(min(ld06.getPoints(n-1)->distance, maxDist), 0, maxDist, 0, diameter/2 - 10);

    Display.fillCircle(centerX, centerY, diameter/2 - 5 , 0xffff);
    for (uint16_t i = 0; i < n; i+=2) {

      float angle = ld06.getPoints(i)->angle+90.0;
      float dist = ld06.getPoints(i)->distance;

      if(80 < dist){ 
        float newX = centerX  + cosf(angle * 2.0 * PI / 360.0) * map(min(dist, maxDist), 0, maxDist, 0, diameter/2 - 10);
        float newY = centerY + sinf(angle * 2.0 * PI / 360.0) * map(min(dist, maxDist), 0, maxDist, 0, diameter/2 - 10);
      
        //Display.drawPixel(newX,newY,0x867D);
        Display.fillRect(newX,newY,5,5,0x867D);
//      Display.drawLine(oldX, oldY, newX, newY, 0x867D);
//      Display.drawLine(oldX+1, oldY+1, newX+1, newY+1, 0xc141);
//      Display.drawLine(oldX-1, oldY-1, newX-1, newY-1, 0xc141);
//      Display.drawLine(oldX+2, oldY+2, newX+2, newY+2, 0xc141);
//      Display.drawLine(oldX-2, oldY-2, newX-2, newY-2, 0xc141);

      }      
      //oldX = newX;
      //oldY = newY;      
   
    }

    Display.drawLine(centerX+4, centerY, centerX-2, centerY+3, 0xc141);
    Display.drawLine(centerX-2, centerY+3, centerX-2, centerY-3, 0xc141);
    Display.drawLine(centerX-2, centerY-3, centerX+4, centerY, 0xc141);
    delay(20);
  }
}
