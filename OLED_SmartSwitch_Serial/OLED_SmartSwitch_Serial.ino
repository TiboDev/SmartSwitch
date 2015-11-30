/*********************************************************************
  This is an example for our Monochrome OLEDs based on SSD1306 drivers
  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98
  This example is for a 128x64 size display using I2C to communicate
  3 pins are required to interface (2 I2C and one reset)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!
  Written by Limor Fried/Ladyada  for Adafruit Industries.
  BSD license, check license.txt for more information
  All text above, and the splash screen must be included in any redistribution
*********************************************************************/
//#define DEBUG

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "principal.h"
#include "Rotary_Encoder.h"

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

unsigned int buf [10];
unsigned char lastScreen, lastCursor = 0;
unsigned char msgToSend = 0;
volatile byte pos = 0;
volatile boolean process_it = false;
CRotaryEncoder encoder;
bool choice = true;
bool writingDataFlag = false;
bool dataSendFlag = false;
unsigned char error = 0;

unsigned char hours = 23;
unsigned char minutes = 59;
unsigned char seconds = 55;
unsigned char wifi_status = 2;
bool wifi = false;
unsigned char price = 0;
unsigned int startTime = 0;
unsigned int endTime = 0;
unsigned int remainTime = 0;
unsigned int charge = 0;;


#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16

static const unsigned char PROGMEM wifi_0[] =
{ B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00100001, B00000000,
  B00010010, B00000000,
  B00001100, B00000000,
  B00001100, B00000000,
  B00010010, B00000000,
  B00100001, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000
};

static const unsigned char PROGMEM wifi_1[] =
{ B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00010000, B00000000,
  B00001000, B00000000,
  B01101000, B00000000,
  B01101000, B00000000,
  B00001000, B00000000,
  B00010000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000
};

static const unsigned char PROGMEM wifi_2[] =
{ B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00001000, B00000000,
  B00000100, B00000000,
  B00010010, B00000000,
  B00001010, B00000000,
  B01101010, B00000000,
  B01101010, B00000000,
  B00001010, B00000000,
  B00010010, B00000000,
  B00000100, B00000000,
  B00001000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000,
  B00000000, B00000000
};

static const unsigned char PROGMEM wifi_3[] =
{ B00000000, B00000000,
  B00000100, B00000000,
  B00000010, B00000000,
  B00001001, B00000000,
  B00000100, B10000000,
  B00010010, B10000000,
  B00001010, B10000000,
  B01101010, B10000000,
  B01101010, B10000000,
  B00001010, B10000000,
  B00010010, B10000000,
  B00000100, B10000000,
  B00001001, B00000000,
  B00000010, B00000000,
  B00000100, B00000000,
  B00000000, B00000000
};


// SPI interrupt routine
ISR (SPI_STC_vect)
{
  byte c = SPDR;  // grab byte from SPI Data Register

  //response SPI
  if (dataSendFlag)
  {
    SPDR = msgToSend;
    dataSendFlag = false;
#ifdef DEBUG
    Serial.print("SPI send : ");
    Serial.println(msgToSend, BIN);
#endif
  }
  else
    SPDR = 0;

  if (pos > 8)
    pos = 0;
  // add to buffer if room
  buf [pos++] = c;

#ifdef DEBUG
  Serial.print("Interrupt receive : ");
  Serial.print(c, DEC);
  Serial.print(" position : ");
  Serial.println(pos - 1);
#endif

  // example: newline means time to process buffer
  if (c == 253)
  {
    process_it = true;
    pos = 0;
  }

}  // end of inter*rupt routine SPI_STC_vect

// main loop - wait for flag set in interrupt routine

void setup()   {
  //#ifdef DEBUG
  Serial.begin(9600);
  Serial.print("\n\n\n----- BEGIN -----\n\n\n");
  //#endif

  /*SPI init*/
  // turn on SPI in slave mode
  SPCR |= bit (SPE);

  // have to send on master in, *slave out*
  pinMode(MISO, OUTPUT);

  // now turn on interrupts
  SPI.attachInterrupt();

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();

  // Clear the buffer.
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  delay(1000);
}

void loop() {
  unsigned char positionCursor = 0;
  encoder.updatePosition();
  positionCursor = encoder.getPosition();
  if (encoder.isButtonPressed() && !dataSendFlag)
  {
    writingDataFlag = true;
    msgToSend = 1;
    msgToSend <<= 3;
    msgToSend += SPI_EN_PRESSED;
    writingDataFlag = false;
    dataSendFlag = true;
  }

  if (process_it || positionCursor == INCREASE || positionCursor == DECREASE)
  {
    process_it = false;

#ifdef DEBUG
    Serial.print("SPI receive code : ");
    Serial.print(buf[0]);
    Serial.print(" data : ");
    Serial.println(buf[1]);
#endif

    // Calculations maybe wrong
    switch (buf[0])
    {
      case UART_HOUR:
        hours = buf[1];
        break;

      case UART_MINUTE:
        minutes = buf[1];
        break;

      case UART_SECOND:
        seconds = buf[1];
        break;

      case UART_WIFI_STATUS:
        wifi = buf[1];
        break;

      case UART_WIFI_RANGE:
        wifi_status = buf[1];
        break;

      case UART_MENU_TYPE:
        lastScreen = buf[1];
        break;

      case UART_START_TIME:
        startTime = buf[1];
        break;

      case UART_END_TIME:
        endTime = buf[1];
        break;

      case UART_REMAINING_TIME:
        remainTime = buf[1];
        break;

      case UART_CHARGING_DURATION:
        charge = buf[1];
        break;

      case UART_AVERAGE_PRICE:
        price = buf[1];
        break;

      case UART_CURSOR_SCREEN:
        lastCursor = buf[1];
        break;
      /*  case UART_ERROR:
          error = buf[1];*/

      default:
        break;
    }

    switch (lastScreen) {
      case MENU_TYPE_START:
        startScreen();
        break;

      case MENU_TYPE_CONFIG:
        if (positionCursor == INCREASE)
        {
          switch (lastCursor)
          {
            case CURSOR_START_TIME:
              if (startTime < 1440)
                startTime += 60;
              break;

            case CURSOR_RANGE_CHARGING:
              if (remainTime < 1440)
                remainTime += 60;
              break;

            case CURSOR_END_TIME:
              if (endTime < 1440)
                endTime += 60;
              break;

            case CURSOR_CHOICE:
              choice = true;
              break;
          }
        }
        else if (positionCursor == DECREASE)
        {
          switch (lastCursor)
          {
            case CURSOR_START_TIME:
              if (startTime >= 60)
                startTime -= 60;
              break;

            case CURSOR_RANGE_CHARGING:
              if (remainTime >= 60)
                remainTime -= 60;
              break;

            case CURSOR_END_TIME:
              if (endTime >= 60)
                endTime -= 60;
              break;

            case CURSOR_CHOICE:
              choice = false;
              break;
          }
        }
        
        if (!dataSendFlag && positionCursor != NEUTRAL)
        {
          writingDataFlag = true;
          msgToSend = positionCursor;
          msgToSend <<= 3;
          msgToSend += SPI_EN_MOVED;
          writingDataFlag = false;
          dataSendFlag = true;
        }
        
        configScreen();

        break;

      case MENU_TYPE_INFORMATION:
        informationScreen();
        break;

      case MENU_TYPE_PROCESS:
        processScreen();
        break;

      case MENU_TYPE_END:
        endScreen();
        break;

      case MENU_TYPE_ERROR:
        errorScreen();
        break;

      default:
        errorScreen();
        break;
    }

    informationScreen();

    display.display();
    display.clearDisplay();

  }
}

void informationScreen()
{
#ifdef DEBUG
  Serial.print("Time : ");
  Serial.print(hours);
  Serial.print(":");
  Serial.print(minutes);
  Serial.print(":");
  Serial.println(seconds);
  Serial.print("Average Price :");
  Serial.println(price);
#endif

  display.setCursor(0, 0);
  switch (wifi_status)
  {
    case 0:
      display.drawBitmap(0, 0, wifi_0, 10, 16, 1);
      break;

    case 1:
      display.drawBitmap(0, 0, wifi_1, 10, 16, 1);
      break;

    case 2:
      display.drawBitmap(0, 0, wifi_2, 10, 16, 1);
      break;

    case 3:
      display.drawBitmap(0, 0, wifi_3, 10, 16, 1);
      break;
  }

  // Display time
  display.setCursor(80, 0);
  if (hours < 10) display.print(0);
  display.print(hours);
  display.print(":");
  if (minutes < 10) display.print(0);
  display.print(minutes);
  display.print(":");
  if (seconds < 10) display.print(0);
  display.print(seconds);

  // Define parameters for display (size, color, cursor)
#ifdef DEBUG
  Serial.print("Wifi status : ");
  Serial.println(wifi);
#endif
}

void startScreen()
{
#ifdef DEBUG
  Serial.println("\nStart screen");
#endif
  display.setCursor(10, 20);
  display.print("Init Wi-Fi...");
  display.setCursor(10, 40);
  display.print("request electricity price");
}

void configScreen()
{
  unsigned char y = 5;
#ifdef DEBUG
  Serial.println("\nCongifuration screen");
#endif
  /*------ Display start time ----*/
  display.setCursor(0, y += 12);
  if (lastCursor == CURSOR_START_TIME)
  {
    display.setTextColor(BLACK, WHITE);
    display.print("Start time: ");
    display.setTextColor(WHITE);
  }
  else
    display.print("Start time: ");

  display.print(startTime / 60);
  display.print(":");
  if (startTime % 60 < 10) display.print(0);
  display.print(startTime % 60);
  Serial.println(startTime);
  display.setCursor(0, y += 12);

  /*------- Display the remaining time -------------*/
  if (lastCursor == CURSOR_RANGE_CHARGING)
  {
    display.setTextColor(BLACK, WHITE);
    display.print("Remaining time: ");
    display.setTextColor(WHITE);
  }
  else
    display.print("Remaining time: ");

  display.print(remainTime / 60);
  display.print(":");
  if (remainTime % 60 < 10) display.print(0);
  display.print(remainTime % 60);

  display.setCursor(0, y += 12);
  if (lastCursor == CURSOR_END_TIME)
  {
    display.setTextColor(BLACK, WHITE);
    display.print("End time: ");
    display.setTextColor(WHITE);
  }
  else
    display.print("End time: ");

  display.print(endTime / 60);
  display.print(":");
  if (endTime % 60 < 10) display.print(0);
  display.print(endTime % 60);
  display.setCursor(40, 55);
  if (lastCursor == CURSOR_CHOICE)
  {
    if (choice)
    {
      display.setTextColor(BLACK, WHITE);
      display.print("OK");
      display.setTextColor(WHITE);
      display.setCursor(80, 55);
      display.print("back");
    }
    else
    {
      display.setTextColor(WHITE);
      display.print("OK");
      display.setTextColor(BLACK, WHITE);
      display.setCursor(80, 55);
      display.print("back");
      display.setTextColor(WHITE);
    }
  }
  else
  {
    display.print("OK");
    display.setCursor(80, 55);
    display.print("back");
  }
}

void processScreen()
{
#ifdef DEBUG
  Serial.println("\nProcess screen");
#endif
  display.setTextColor(WHITE);
  display.setCursor(35, 20);
  display.print("In charge");
  display.setCursor(20, 35);
  display.print("Remaining time:");

  display.setCursor(35, 45);
  if (charge / 60 < 10) display.print(0);
  display.print(charge / 60);
  display.print(":");
  if (charge % 60 < 10) display.print(0);
  display.print(charge % 60);
}

void endScreen()
{
#ifdef DEBUG
  Serial.println("\nEnd screen");
#endif
  display.setCursor(10, 17);
  display.print("Process finished");
  display.setCursor(10, 33);
  display.print("Price:");
  display.print(price);
  display.print(" Eur");

  /*display.setCursor(10, 30);
    display.print("start time:");*/
  /*if (startHour < 10) display.print(0);
    display.print(startHour);
    display.print(":");
    if (startMinute < 10) display.print(0);
    display.print(startMinute);*/
  display.setCursor(7, 45);
  display.print("Time of charge:");
  //display.setCursor(10, 40);
  if (remainTime / 60 < 10) display.print(0);
  display.print(remainTime / 60);
  display.print(":");
  if (remainTime % 60 < 10) display.print(0);
  display.print(remainTime % 60);

  display.setCursor(40, 55);
  if (choice)
  {
    display.setTextColor(BLACK, WHITE);
    display.print("OK");
    display.setTextColor(WHITE);
    display.setCursor(80, 55);
    display.print("back");
  }
  else
  {
    display.setTextColor(WHITE);
    display.print("OK");
    display.setTextColor(BLACK, WHITE);
    display.setCursor(80, 55);
    display.print("back");
  }
  display.setTextColor(WHITE);
}

void errorScreen()
{
  display.setCursor(10, 32);
  display.print("Waiting ...");
  /*if (error == 1) display.print("Warning: WI-FI is unlocked");
    if (error == 2) display.print("Warning: Nothing plugged");*/
}


