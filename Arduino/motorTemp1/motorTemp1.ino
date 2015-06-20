/*
This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.

VERSION 1.0
This is a temperature display for diesel engines combined with a wind speed display
It makes use of a ST7735 128x160 tft display.
The Displayed temprature value changes color to give extra information.
A alarm trip value can be set (hard coded) to create an over temperature alarm.

In wind display mode (if the engine is not running) it displays the wind speed in knots from the
NASA NMEA wind unit. it also displays the speed in Bft and draws a graph with adaptive scale to
show the wind speed in time averaged over 30 seconds and over 5 seconds to see the gusts.

If the engine is not running wind data is interesting. A graph with wind speed and
gusts would be nice. Inside the wind meter there is also a temp sensor.

To see is the engine is running an external input can be used, it can also be connected to
a switch. It would be nice to have an optocoupler on this input.

strings from the nasa wind meter:
$WIMWV,65.0,R,0.0,N,A*10
$YXXDR,C,16,C*64
$WIMWV,103.0,R,0.0,N,A*21
$YXXDR,C,16,C*64
$WIMWV,122.0,R,0.0,N,A*22
$YXXDR,C,16,C*64
$WIMWV,162.0,R,0.0,N,A*26
$YXXDR,C,16,C*64

The serial buffer holds 64 bytes. With these short strings the searching can be done directly on the buffer.
4800bps=>480 byte/s = 0.13Seconds to fill te buffer.
*/
//I also tried the arduino tft lib but that one has difficulty displaying floats.
//it has other positive sides like not having to set the tab color of the display
//and a better way to select the color and standarized arduino like library
//But in the end I did choose the adafruit. Because of the hassele to print floats

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>

// Libraries for DS1820 and Onewire
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Average.h>



#define DEBUG 0
//1 normal debug
//2 simulate wind speed

// For the breakout, you can use any 2 or 3 pins
// These pins will also work for the 1.8" TFT shield
#define TFT_CS     9
#define TFT_RST    8  // you can also connect this to the Arduino reset
// in which case, set this #define pin to 0!
#define TFT_DC     7


#define GRAPHLEN 95

#define TEMPLOW 10 //=temp*10
#define TEMPHI 1000

#define BLUE_TEMP 20 //until this temp blue
#define CYAN_TEMP 40
#define GREEN_TEMP 70
#define TEMPALARM 87

#define TEMPALARMPIN 4
#define SENSORPIN 6

#define MOTORRUNNING 3


OneWire oneWire(SENSORPIN); // Setup onewire instance to communicate
DallasTemperature sensors(&oneWire);

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

char runMode = 0; //to detect the first run of each mode
int windGraphMax = 40; //40 knots is end force 8 make scale adaptive to increase graph resolution

//RunningAverage avWindGusts(5); //1 sample/second 5 seconds avg gusts can be seen
//RunningAverage avWind(120);  //should be 300 for real avg wind speed = avg wind speed over 5 minutes
#define AVBUF 30
Average<float> avWind(AVBUF);
#define AVBUF1 5
Average<float> avWindGusts(AVBUF1);


void setup(void) {
  Serial.begin(4800);  //NMEA works on 4800N81 debug data
 // Serial1.begin(4800); //serial1 for real data
  sensors.begin();
  // Use this initializer if you're using a 1.8" TFT
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab (het ding heeft een groene tab maar black werkt beter)
  tft.fillScreen(ST7735_BLACK);

  pinMode(TEMPALARMPIN, OUTPUT);
  pinMode(MOTORRUNNING, INPUT_PULLUP);
}

void loop()
{
  
  if(digitalRead(MOTORRUNNING))
   {
   scheduelerMotor();
   runMode=1;
   }
  else
   {
   scheduelerWind();
   runMode=2;
   }
  
//  scheduelerWind();


}

void scheduelerWind()
{
  /*
  I'd like my tastks to run timed:
  The graph needs to be updated every 30 seconds
  The temprature display each second
  */
  static long lastSecond = 0;
  static long last3Seconds = 0;
  static long last30Seconds = 0;
  static long last600Seconds = 0; //10 minutes
  float windSpeed = 0;
  static int windGraphMaxNew = windGraphMax;

  if (runMode != 2) //this is the first run init first
  {
    graphTFTInit( 20, 15, 100, 110, ST7735_BLUE, "Time", "Speed(kn)", ST7735_GREEN , 40); //40 kn = end force 8
    windSpeed = getWindSpeed();
    //   avWind.fillValue(windSpeed, 120);
    //   avWindGusts.fillValue(windSpeed,5);
    for (int i = 0; i < AVBUF; i++)
    {
      avWind.push(windSpeed);    //fill buffer with current wind speed value
      avWindGusts.push(windSpeed);    //fill buffer with current wind speed value
    }
    runMode = 2;
  }

  long nu = millis(); //now is already reserved. in this routine I want to work with the time at the start of the routine.
  if (nu < lastSecond) //overflow afterv52 hours!!
  {
    lastSecond = 0;
    last3Seconds = 0;
    last30Seconds = 0;
    last600Seconds = 0; //10 minutes

  }
  // each second
  if (nu > lastSecond + 1000)
  { //do tasks that need to be done each second
    windSpeed = getWindSpeed();
    avWind.push(windSpeed);
    avWindGusts.push(windSpeed);

    printWindSpeed(avWind.mean(), 10, 0, 2, 0x07FF); //cyan
    printBft(knots2Bft(int(avWind.mean()*10.0)), 85, 0, 2, 0x07FF); //cyan
    
    //     avWindGusts.addValue(windSpeed);
    if(DEBUG)
    {
    Serial.println("Wind, windMax, winAV");
    Serial.print(windSpeed);
    Serial.print(" , ");
    Serial.print(avWind.maximum());
    Serial.print(" , ");
    Serial.println(avWind.mean());
    }
    //    trip();
    //Serial.println(getWindSpeed());
    lastSecond = nu;
  }
  if (nu > last3Seconds + 3000)
  { //do tasts that need to be done each 30 seconds
    //    graphTFTDraw( 20, 15, 100, 110, ST7735_RED,int(getTemp()*10.0),0);
    graphTFTDrawWind( 20, 15, 100, 110, ST7735_RED, int(avWind.mean() * 10.0), int(avWindGusts.mean() * 10.0));

    last3Seconds = nu;
  }
  if (nu > last30Seconds + 30000)
  { //windGraphMax
    windGraphMaxNew = setWindScale();
    if (windGraphMaxNew != windGraphMax)
    {
      windGraphMax=windGraphMaxNew;
      graphTFTInit( 20, 15, 100, 110, ST7735_BLUE, "Time", "Speed(kn)", ST7735_GREEN , windGraphMax); //40 kn = end force 8
    }
    if (DEBUG)
    {
      Serial.println("30 seconds !!");
      Serial.println(windGraphMax);
    }

    last30Seconds = nu;
  }
  if (nu > last600Seconds + 600000)
  { //draw graph again with the last max value in the graph*1.5 als the max of the graph


    last600Seconds = nu;
  }

}

int setWindScale()
{
  if (avWind.maximum() > 30)  return 40;
  if (avWind.maximum() > 20)  return 30;
  if (avWind.maximum() > 15)  return 20;
  if (avWind.maximum() > 10)  return 15;
  return 10;

}

int knots2Bft(int knx10)
{
  /*
  input knx10 is windseed in knots * 10 to avoid floats
  bft  knots
  0    <1    10
  1    1-3    35
  2    4-6    65
  3    7-10    105
  4    11-16    165
  5    17-21    215
  6    22-27    275
  7    28-33    335
  8    34-40    405
  9    41-47    475
  10   48-55    555
  11   56-63    635
  12   64+
  */
 if(knx10<10) return 0;
 if(knx10<35) return 1;
 if(knx10<65) return 2;
 if(knx10<105) return 3;
 if(knx10<165) return 4;
 if(knx10<215) return 5;
 if(knx10<275) return 6;
 if(knx10<335) return 7;
 if(knx10<405) return 8;
 if(knx10<475) return 9;
 if(knx10<555) return 10;
 if(knx10<635) return 11;
 return 12;
}

float getWindSpeed()
{
  /*
  Get the wind speed out of the serial buffer. In case of timeout exit with -1
  NMEA data:
  $WIMWV,65.0,R,0.0,N,A*10
  $YXXDR,C,16,C*64
  $WIMWV,103.0,R,0.0,N,A*21
  $YXXDR,C,16,C*64
  $WIMWV,122.0,R,0.0,N,A*22
  $YXXDR,C,16,C*64
  $WIMWV,162.0,R,0.0,N,A*26
  $YXXDR,C,16,C*64

  $WIMWV,162.0,R,0.0,N,A*26<CR><LF>
        dir     Speed N=Nautical A=valid
  this mast head unit always returns Relative and Nautical units

  Oops it should be float...
  */

  byte CRC = 0;
  byte index = 0;
  byte index2 = 0;
  float result = 0;

  if (DEBUG == 2)
  {
    result = 20 + (random(90) / 10.0);
    Serial.print("Simulater wind value : ");
    Serial.println(result, 2);
    return result;
  }


  String wString;
  if (Serial.find("$WIMWV,"))
    wString = Serial.readStringUntil('\n'); //string = 162.0,R,0.0,N,A*26<CR>
  else
    return -1; //timeout finding the string
  Serial.println(wString);
  String sCRC = String("WIMWV," + wString);
  index = sCRC.indexOf('*');    //find the first *
  if (index == -1)
    return -1;                  //wrongly formatted string no *
  wString = wString.substring(0, index - 2); // to make sure the string has no clutter

  if (DEBUG) Serial.println(index, DEC);
  for (int i = 0; i < (index); i++)
    CRC = CRC ^ sCRC.charAt(i); //calculate the CRC
  if (DEBUG)
  {
    Serial.println(sCRC);
    Serial.print("CRC=");
    Serial.println(CRC, HEX);
    Serial.println(wString);
  }
  sCRC = wString.substring(index - 5, index - 3); //get the crc from the string
  if (DEBUG)
  {
    Serial.println(sCRC);
  }
  index = byte(strtol( &sCRC[0], NULL, 16)); //the crc in the string is in hex, make it dec
  if (DEBUG == 0)
  {
    if (index != CRC)              //CRC fault exit -1
      return -1;
  }
  //ok the next part is ugly
  index = wString.indexOf('*');
  index2 = wString.indexOf(',');             //5
  wString = wString.substring(index2 + 1, index); //r,230.0,N,A
  index2 = wString.indexOf(',');             //1
  wString = wString.substring(index2 + 1, index); //230.0,N,A
  result = wString.toFloat();                     //this is the wind speed. If it ever turns out 230 I'm in trouble
  index2 = wString.indexOf(',');             //5
  wString = wString.substring(index2 + 1, index); //N,A
  index2 = wString.indexOf(',');             //1
  wString = wString.substring(index2 + 1, index); //A

  if (wString.charAt(0) != 'A')
    return -2;                               //received nmea is invalid
  return result;
}

byte printWindSpeed(float Wind, byte x, byte y, byte fontSize, int16_t color)
{
  //Only print the temp if it is changed since last time
  //That results in a more stable display
  static float oldWind = -255;
  //float Wind;
  int rond;
  //Nu ik echte floats met avg gebruik gaat het wissen van de oude waarde soms mis en dat geeft een blijvende fout op het display
  // dus als ik de float van x.yyyy naar x.y maak dan zou het wel eens beter kunnen gaan

  //Wind=getWindSpeed();
  if (Wind < 0) //err in readout
    return -1;
  rond = int(Wind * 10);
  Wind = rond / 10.0;
  if (DEBUG == 2)
  {
    Serial.println("Wind, oldWind, int(Wind * 10),int(oldWind * 10)");
    Serial.print(Wind);
    Serial.print(" , ");
    Serial.print(oldWind);
    Serial.print(" , ");
    Serial.print(int(Wind * 100));
    Serial.print(" , ");
    Serial.println(int(oldWind * 100));


  }

  if ((oldWind != -255) && (int(oldWind * 10) != int(Wind * 10))) //compairing floats is no good
  {
    tft.setCursor(x, y);
    tft.setTextColor(ST7735_BLACK); //Erase the last value
    tft.setTextSize(fontSize);
    tft.print(oldWind, 1);
    tft.setTextSize(1);
    tft.print("kn");
    if (DEBUG) Serial.println(oldWind);

  }
  oldWind = Wind;
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.setTextSize(fontSize);
  tft.print(Wind, 1);
  tft.setTextSize(1);
  tft.print("kn");
  //tft.setTextSize(fontSize);
  //tft.print('C');
  return 1;
}

byte printBft(int Wind, byte x, byte y, byte fontSize, int16_t color)
{
  //two almost the same routines is no good
  //Only print the temp if it is changed since last time
  //That results in a more stable display
  static int oldWind = -255;
  //float Wind;
  int rond;
  //Nu ik echte floats met avg gebruik gaat het wissen van de oude waarde soms mis en dat geeft een blijvende fout op het display
  // dus als ik de float van x.yyyy naar x.y maak dan zou het wel eens beter kunnen gaan

  //Wind=getWindSpeed();
  if (Wind < 0) //err in readout
    return -1;
  rond = int(Wind * 10);
  Wind = rond / 10.0;
  if (DEBUG == 2)
  {
    Serial.println("Wind, oldWind, int(Wind * 10),int(oldWind * 10)");
    Serial.print(Wind);
    Serial.print(" , ");
    Serial.print(oldWind);
    Serial.print(" , ");
    Serial.print(int(Wind * 100));
    Serial.print(" , ");
    Serial.println(int(oldWind * 100));


  }

  if ((oldWind != -255) && (int(oldWind * 10) != int(Wind * 10))) //compairing floats is no good
  {
    tft.setCursor(x, y);
    tft.setTextColor(ST7735_BLACK); //Erase the last value
    tft.setTextSize(fontSize);
    tft.print(oldWind);
    tft.setTextSize(1);
    tft.print("kn");
    if (DEBUG) Serial.println(oldWind);

  }
  oldWind = Wind;
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.setTextSize(fontSize);
  tft.print(Wind);
  tft.setTextSize(1);
  tft.print("Bft");
  //tft.setTextSize(fontSize);
  //tft.print('C');
  return 1;
}



void graphTFTDrawWind(byte x, byte y, byte sizeX, byte sizeY, uint16_t color, int var, int var1)
{
  /*
  Fill graph with data.
  DataBuf[GRAPHLEN] are GRAPHLEN points of data in a cicrular buffer. var is written in the buffer at location DataBufPoint.
  The drawing of the buffer to the tft starts at DataBufPoint and runs for GRAPHLEN points
  */

  static uint16_t dataBuf[GRAPHLEN];
  static uint16_t dataBuf1[GRAPHLEN];
  static char dataBufPoint = 0;
  int j = 0, k = 0;
  x++; //the graph area in 1 smaller than the axis drawn
  y++;
  sizeX;
  sizeY;
  if (var < 0) var = 0;
  if (var1 < 0) var1 = 0;
  tft.fillRect(x, tft.height() - sizeY - y, sizeX, sizeY + 1, 0);

  if (dataBufPoint == GRAPHLEN - 1)
    dataBufPoint = 0;

  dataBuf[dataBufPoint] = var;
  dataBuf1[dataBufPoint] = var1;

  dataBufPoint++;
  for (char i = 0; i < GRAPHLEN; i++)
  {
    j = dataBufPoint + i;
    if (j > GRAPHLEN - 1)
      j = dataBufPoint + i - GRAPHLEN;
    k = map(dataBuf[j], 0, windGraphMax * 10, tft.height() - y, tft.height() - y - sizeY);
    tft.drawPixel(x + i , constrain ((k), tft.height() - y - sizeY, tft.height() - y), color);

    k = map(dataBuf1[j], 0, windGraphMax * 10, tft.height() - y, tft.height() - y - sizeY);
    tft.drawPixel(x + i , constrain ((k), tft.height() - y - sizeY, tft.height() - y), 0xFFE0); //yellow

  }
}




void scheduelerMotor()
{
  /*
  I'd like my tastks to run timed:
  The graph needs to be updated every 30 seconds
  The temprature display each second
  */
  //  static long lastTime=0;
  static long lastSecond = 0;
  static long last30Seconds = 0;
  if (runMode != 1) //this is the first run init first
  {
    graphTFTInit( 20, 15, 100, 110, ST7735_BLUE, "Time(30sec)", "Temp", ST7735_GREEN, 100);
    runMode = 1;
  }
  long nu = millis(); //now is already reserved. in this routine I want to work with the time at the start of the routine.
  // each second
  if (nu > lastSecond + 1000)
  { //do tasks that need to be done each second
    printTemp(10, 0, 2, temp2Color(getTemp()));
    trip();
    lastSecond = nu;
  }
  if (nu > last30Seconds + 30000)
  { //do tasts that need to be done each 30 seconds
    graphTFTDraw( 20, 15, 100, 110, ST7735_RED, int(getTemp() * 10.0), 0);
    last30Seconds = nu;
  }


  //  lastTime=nu; //Preserve te last execution time
}

float getTemp()
{
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

void printTemp(byte x, byte y, byte fontSize, int16_t color)
{
  //Only print the temp if it is changed since last time
  //That results in a more stable display
  static float oldTemp = -255;
  float Temp;

  Temp = getTemp();

  if ((oldTemp != -255) && (int(oldTemp * 10) != int(Temp * 10))) //compairing floats is no good
  {
    tft.setCursor(x, y);
    tft.setTextColor(ST7735_BLACK); //Erase the last value
    tft.setTextSize(fontSize);
    tft.print(oldTemp, 1); //The temperature sensor returns only one digit
    tft.setTextSize(1);
    tft.print('o');
    tft.setTextSize(fontSize);
    tft.print('C');

  }
  oldTemp = Temp;
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.setTextSize(fontSize);
  tft.print(Temp, 1);
  tft.setTextSize(1);
  tft.print('o');
  tft.setTextSize(fontSize);
  tft.print('C');

}

void graphTFTInit(byte x, byte y, byte sizeX, byte sizeY, uint16_t color, char *xLegend, char *yLegend, uint16_t tColor, int maxYScale)
{
  //dispay has 0,0 top left
  //to convert to bottom left y1=tft.height()-y
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(tColor);
  tft.setTextSize(1);
  tft.setCursor(x + 10, (tft.height() - y + 2));
  tft.print(xLegend);

  tft.setRotation(3); // rotate screen to print y legend
  tft.setCursor(tft.width() - x - sizeX, y - 8);
  tft.print(yLegend);
  tft.setRotation(0); // rotate back again
  tft.setCursor(0, tft.height() - y - 8);
  tft.print(0);       //these values should be set by varibles or something, but this works too. 0 degrees at the botom
  tft.setCursor(0, tft.height() - y - sizeY);
  tft.print(maxYScale); //these values should be set by varibles or something, but this works too.100 degrees at the top


  tft.drawFastHLine(x, tft.height() - y, sizeX, color); //fastline code is realy different in the lib so I guess it is realy faster
  tft.drawFastVLine(x, tft.height() - sizeY - y, sizeY, color);

}

void graphTFTDraw(byte x, byte y, byte sizeX, byte sizeY, uint16_t color, int var, char trip)
{
  /*
  Fill graph with data.
  DataBuf[GRAPHLEN] are GRAPHLEN points of data in a cicrular buffer. var is written in the buffer at location DataBufPoint.
  The drawing of the buffer to the tft starts at DataBufPoint and runs for GRAPHLEN points
  */

  static int dataBuf[GRAPHLEN];
  static char dataBufPoint = 0;
  int j = 0, k = 0;
  x++; //the graph area in 1 smaller than the axis drawn
  y++;
  sizeX;
  sizeY;

  tft.fillRect(x, tft.height() - sizeY - y, sizeX, sizeY + 1, 0);

  if (dataBufPoint == GRAPHLEN - 1)
    dataBufPoint = 0;

  dataBuf[dataBufPoint] = var;
  dataBufPoint++;
  for (char i = 0; i < GRAPHLEN; i++)
  {
    j = dataBufPoint + i;
    if (j > GRAPHLEN - 1)
      j = dataBufPoint + i - GRAPHLEN;
    k = map(dataBuf[j], TEMPLOW, TEMPHI, tft.height() - y, tft.height() - y - sizeY);
    tft.drawPixel(x + i , constrain ((k), tft.height() - y - sizeY, tft.height() - y), color);
  }
}

int temp2Color(float temp)
{
  /*
  Now with these tft displays cool things are posible. Information can also be given at a glance in the form of color
  b0   b4 b5     b10  b11     b15
  Blue     Green         Red

  so a pattern 111 shifting through the 16 bits could change the color from cool blue to red hot.
  hu nice try but no bannana's
  I still need to optimize the colors for my diesel engine MD3B
  */
  temp = int(temp); //compairing floats is no good
  if (temp < BLUE_TEMP)
    return 0x001F; //BLUE
  if (temp < CYAN_TEMP)
    return 0x07FF; //CYAN
  if (temp < GREEN_TEMP)
    return 0x07E0; //green
  if (temp < TEMPALARM )
    return 0xFFE0;  //yellow
  return ST7735_RED;

}


void trip()
{
  if (int(getTemp) > TEMPALARM)
    digitalWrite(TEMPALARMPIN, HIGH);
  else
    digitalWrite(TEMPALARMPIN, LOW);

}

