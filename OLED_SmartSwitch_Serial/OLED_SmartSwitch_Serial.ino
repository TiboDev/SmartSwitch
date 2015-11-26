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
#define DEBUG
#define ENABLE_SPI

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "principal.h"

unsigned int buf [10];
unsigned char lastScreen;
volatile byte pos = 0;
volatile boolean process_it = false;

/*#define NUMFLAKES 10
  #define XPOS 0
  #define YPOS 1
  #define DELTAY 2*/

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

unsigned char hours = 23;
unsigned char minutes = 59;
unsigned char seconds = 55;
unsigned char wifi_status = 5;
bool wifi = false;
float price = 0;

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


#ifdef ENABLE_SPI
// SPI interrupt routine
ISR (SPI_STC_vect)
{
  byte c = SPDR;  // grab byte from SPI Data Register

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
  if (c == 254)
  {
    process_it = true;
    pos = 0;
  }

}  // end of inter*rupt routine SPI_STC_vect
#endif

// main loop - wait for flag set in interrupt routine

void setup()   {
#ifdef DEBUG
  Serial.begin(9600);
  Serial.print("\n\n\n----- BEGIN -----\n\n\n");
#endif

  /*SPI init*/
#ifdef ENABLE_SPI
  // turn on SPI in slave mode
  SPCR |= bit (SPE);

  // have to send on master in, *slave out*
  pinMode(MISO, OUTPUT);

  // now turn on interrupts
  SPI.attachInterrupt();

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
#endif

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
  if (process_it)
  {
#ifdef ENABLE_SPI
    process_it = false;
#endif

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

      default:
        break;
    }

    switch (lastScreen) {
      case MENU_TYPE_START:
        startScreen();
        break;

      case MENU_TYPE_BEST_HOUR:
        bestHourScreen();
        break;

      default:
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
#endif

  display.setCursor(0, 0);
  switch (wifi)
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

  display.setCursor(20, 17);
  display.print("Starting page");
  display.setCursor(7, 27);
  display.print("Enter the start time");
  display.setCursor(20, 44);
  display.print("0 : 00");
}

void bestHourScreen()
{
#ifdef DEBUG
  Serial.println("\nBest hour screen");
#endif

  display.setCursor(0, 15);
  display.print("Best hour to start");
  display.setCursor(0, 27);
  display.print("Do you agree ?");
  display.setCursor(20, 44);
  display.setTextColor(BLACK, WHITE);
  display.print("OK");
  display.setTextColor(WHITE);
  display.setCursor(50, 44);
  display.print("back");
}

