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
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>

unsigned int buf [10];
unsigned char lastScreen;
volatile byte pos;
volatile boolean process_it;

#include "principal.h"

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);


unsigned char hours = 23;
unsigned char minutes = 59;
unsigned char seconds = 55;
unsigned char wifi_status = 5;
bool wifi = true;
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

// SPI interrupt routine
ISR (SPI_STC_vect)
{
  byte c = SPDR;  // grab byte from SPI Data Register

  // add to buffer if room
  if (pos < sizeof buf)
  {
    buf [pos++] = c;

    // example: newline means time to process buffer
    if (c == '\n')
      process_it = true;

  }  // end of room available
}  // end of interrupt routine SPI_STC_vect

// main loop - wait for flag set in interrupt routine

void setup()   {
  Serial.begin(9600);

  /*SPI init*/

  // turn on SPI in slave mode
  SPCR |= bit (SPE);

  // have to send on master in, *slave out*
  pinMode(MISO, OUTPUT);

  // get ready for an interrupt
  pos = 0;   // buffer empty
  process_it = false;

  // now turn on interrupts
  SPI.attachInterrupt();

  Serial.println("Start");

  // Initialisation for random number
  randomSeed(0);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(2000);

  // Clear the buffer.
  display.clearDisplay();
}

void loop() {

  int i = 0, code = 0, resultat = 0;
  unsigned int buffer_uart[10];

  if (process_it)
  {
    Serial.println("SPI receive\n");

    buf [pos] = 0;
    code = buf[0];
    resultat = buf[1];
    for (int i = 0; i < pos; i++)
      Serial.println(buf[i]);

    Serial.println();
    process_it = false;
    pos = 0;

    // Calculations maybe wrong
    switch (code)
    {
      case UART_HOUR:
        hours = resultat;
        break;

      case UART_MINUTE:
        minutes = resultat;
        break;

      case UART_SECOND:
        seconds = resultat;
        break;

      case UART_WIFI_STATUS:
        wifi = resultat;
        break;

      case UART_WIFI_RANGE:
        wifi_status = resultat;
        break;

      case UART_MENU_TYPE:
        lastScreen = resultat;
      default:
        break;
    }

    switch (lastScreen) {
      case MENU_TYPE_START:
        bestHourScreen();
        //startScreen();
        break;
      case MENU_TYPE_BEST_HOUR:
        bestHourScreen();
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
  // Define parameters for display (size, color, cursor)
  display.setTextSize(1);
  display.setTextColor(WHITE);
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
}

void smartswitch_display() {
  // Internet price, only for testing
  price = random(100, 9999);
  price = price / 100;

  // Display text
  display.setCursor(80, 10);
  display.print("Done!");
  display.setCursor(7, 24);
  display.print("Hello Smart Switch!");
  display.setCursor(13, 34);
  display.print("You are the best!");
  display.setCursor(13, 44);
  display.print("Price : ");
  display.print(price);
  display.print(" Eur");
  display.display();
}

void startScreen()
{
  display.setCursor(20, 17);
  display.print("Starting page");
  display.setCursor(7, 27);
  display.print("Enter the start time");
  display.setCursor(20, 44);
  display.print("0 : 00");
}

void bestHourScreen()
{
  display.setCursor(0, 15);
  display.print("Best hour to start");
  display.setCursor(0, 27);
  display.print("Do you agree ?");
  display.setCursor(20, 44);
  display.print("OK");
  display.setCursor(50, 44);
  display.print("back");
}

