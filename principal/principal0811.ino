#include <Bridge.h>
#include <Console.h>
#include <FileIO.h>
#include <HttpClient.h>
#include <Mailbox.h>
#include <Process.h>
#include <YunClient.h>
#include <YunServer.h>
#include <math.h>
#include <SPI.h>
#include "time.h"
#include "principal.h"

const int PINRED = 5;
const int PINBLUE = 6;
const int PINGREEN = 7;
const int PINTRANS = 9;

/*
   3 states:
   0 inactive
   INCREASE
   DECREASE
*/
unsigned char potentiometer_state = 0;

// we define all the variables as int and float, but we can switch to double later in order to occupy less memory
int hourupdate = -1; // it is 0 if it isn't 2pm and 1 if it is
float watch = -1; // variable that knows what time it is
bool wifi = false; // it is 0 if the wifi doesn't work and 1 if it works    #modify in bool
float finalPrices[36]; // array with 34 elements, because every 2pm we get the prices from 2pm to midnight of the following day
int start_hour_best = -1; // it is the best hour suggested by the device
float average_price = -1; // average price estimated considering the best hours for charging the device (euros/hour)
float start_hour_user = 5; // the user can decide to start chargin in a different moment from the one suggested
float desired_duration = 20; // duration of the charging set by the user, time that the device needs to be charged
float time_passed = -1; // variable that counts the time starting from the beginning of the charging
bool flag = false;         //variable to catch if charging process has started
int plugged = -1; // it is 0 if nothing is plugged and 1 if something is. This value comes from the current sensor, that activates an INPUT pin (so actually we won't need this variable)

unsigned char programStat = STAT_START;

bool config_screen = false;
bool information_screen = false;
bool start = true;
bool end_screen = false;
bool buttonIsPressed = false;
bool process_screen = false;

int start_time = 0;

CTime timeCustom;


/*
   Part to get the synchronize time
*/

Process date;

/*
   Function to send message to Arduino board

   WARNING the value to send has to be max 255 !
*/
void printScreen(unsigned char sendCode, unsigned int toSend)
{
  unsigned char SPIresponse = 0;
  unsigned char buf;
  Serial.print("Send SPI code : ");
  Serial.print(sendCode);
  Serial.print(" data : ");
  Serial.println(toSend);

  SPIresponse = SPI.transfer (sendCode);
  Serial.print("SPI received : ");
  Serial.println(SPIresponse, BIN);
  switch (SPIresponse & 0b111)
  {
    case SPI_EN_MOVED:
      potentiometer_state = SPIresponse >> 3;
      break;
    case SPI_EN_PRESSED:
      if ((SPIresponse >> 3) == 1)
      {
        buttonIsPressed = true;
      }
      break;
    default:
      break;
  }

  delay(75);
  SPIresponse = SPI.transfer ((unsigned int)toSend);
  Serial.print("SPI received : ");
  Serial.println(SPIresponse, BIN);
  switch (SPIresponse & 0b111)
  {
    case SPI_EN_MOVED:
      potentiometer_state = SPIresponse >> 3;
      break;
    case SPI_EN_PRESSED:
      if ((SPIresponse >> 3) == 1)
      {
        buttonIsPressed = true;
      }
      break;
    default:
      break;
  }

  delay(75);
  SPIresponse = SPI.transfer (253);
  Serial.print("SPI received : ");
  Serial.println(SPIresponse, BIN);
  switch (SPIresponse & 0b111)
  {
    case SPI_EN_MOVED:
      potentiometer_state = SPIresponse >> 3;
      break;
    case SPI_EN_PRESSED:
      if ((SPIresponse >> 3) == 1)
      {
        buttonIsPressed = true;
      }
      break;
    default:
      break;
  }
  delay(75);
}

/*
   Refresh the time -should work
*/
void getTime(bool Display)
{
  if (!date.running())  {
    date.begin("date");
    date.addParameter("+%T");
    date.run();

    if (date.available() > 0) {
      String timeString = date.readString();

      int firstColon = timeString.indexOf(":");
      int secondColon = timeString.lastIndexOf(":");
      String hourString = timeString.substring(0, firstColon);
      String minString = timeString.substring(firstColon + 1, secondColon);
      String secString = timeString.substring(secondColon + 1);

      timeCustom.setHour(hourString.toInt());
      timeCustom.setMinute(minString.toInt());
      timeCustom.setSecond(secString.toInt());

      if (Display)
      {
        printScreen(UART_SECOND, timeCustom.getSecond());
        if (timeCustom.minuteChanged())
        {
          printScreen(UART_HOUR, timeCustom.getHour());
          printScreen(UART_MINUTE, timeCustom.getMinute());
        }
      }
    }
  }
}

/*
   Control LED
*/

void setLED(unsigned char R, unsigned char G, unsigned char B)
{
  digitalWrite(PINRED, R);
  digitalWrite(PINGREEN, G);
  digitalWrite(PINBLUE, B);
}

void setLED(unsigned char color)
{
  Serial.print("Switch on light : ");
  Serial.println(color, DEC);
  switch (color)
  {
    case RED:
      setLED(HIGH, LOW, LOW);
      break;
    case GREEN:
      setLED(LOW, HIGH, LOW);
      break;
    case BLUE:
      setLED(LOW, LOW, HIGH);
      break;
    case YELLOW:
      setLED(HIGH, HIGH, LOW);
      break;
    case WHITE:
      setLED(HIGH, HIGH, HIGH);
      break;
    case NONE:
      setLED(LOW, LOW, LOW);
      break;
  }
}

/*
   Process of Wifi
*/
bool getWifiStatus(bool Display)
{
  if (Display)
  {
    Process wifiCheck;
    wifiCheck.runShellCommand("/usr/bin/pretty-wifi-info.lua");

    while (wifiCheck.available() > 0) {
      char c = wifiCheck.read();
      Serial.print(c);
    }
  }

  //if hours are still equal to -1 it means Wifi is not locked
  if (timeCustom.isTimeSet())
    return true;
  else
    return false;
}

/*
   Function HTTP
*/

void httpClient()
{
  HttpClient client;
  // Make a HTTP request:
  client.get("http://slsoptimizationservice.cloudapp.net/json/reply/PriceRequest");

  int sum = 0; // a varible which is going to increase to get the right spot in the string
  int counter = 0; // counter which is going to increase until one hour price is known
  int hoursknown = 0; // is calculating how many hours are known
  unsigned int temp = 0;
  unsigned char prices[36]; // a sting array for putting prices
  Serial.print("Made request to SLS server ");
  while (client.available()) // when recieving data from server
  {
    char c = client.read(); // is reading one character

    if (counter == 5) // this is controlling if counter has reached number 5, if it is it is going to reset
    {
      int k = 0;

      while (prices[k] != 254 && k < temp - 1)
      {
        k++;
      }
      if (k == temp - 1)
        k++;

      finalPrices[hoursknown] = 0;
      //unité
      for (int j = 0; j < k; j++)
      {
        int tempPuissance = 1;
        for (int m = 0; m < k - j - 1; m++)
        {
          tempPuissance *= 10;

        }
        finalPrices[hoursknown] += prices[j] * tempPuissance;
      }

      //décimale
      for (int j = k + 1; j < temp; j++)
      {
        float tempPuissance = 1;
        for (int m = 0; m < j - k; m++)
        {
          tempPuissance /= 10.0;
        }
        finalPrices[hoursknown] += prices[j] * tempPuissance;
      }
      temp = 0;
      counter = 0;
      sum = 0;
      hoursknown++;
    }
    // In here there are 8 if statements which are checking in every cycle the the read string value
    // When reading order is right then it has read /value + two spaces/ and the from 8 is starting the number
    if (c == 'v')
    {
      sum = 1;
    }
    else if (c == 'a'& sum == 1)
    {
      sum = 2;
    }
    else if (c == 'l' & sum == 2)
    {
      sum = 3;
    }
    else if (c == 'u' & sum == 3)
    {
      sum = 4;
    }
    else if (c == 'e' & sum == 4)
    {
      sum = 5;
    }
    else if (sum == 5)
    {
      sum = 6;
    }
    else if (sum == 6)
    {
      sum = 7;
    }
    else if (sum == 7)
    {
      if (c != ' ' && c != '}')// in here it is checking if read value is not a space
      {
        prices[temp++] = c - 48; // if it is not the number is added to the string
      }
      counter++; // counter is increased by 1
      if (c == '}')
        counter = 5;
    }
    else // if the c value is not matching then it is reseted
    {
      sum = 0;
    }
  }
  Serial.flush();
}


void routine()
{
  getTime(true);
  if (timeCustom.minuteChanged())
    printScreen(UART_WIFI_STATUS, getWifiStatus(false));
  if (!plugged) {
    setLED (YELLOW);
    Serial.println ("ERROR: Nothing is plugged, plug it ...");
    while (!plugged);
    Serial.println ("Now you have plugged a device!"); // the alternative is to understand how to delete messages from the screen
    setLED(NONE);
  }
}

void setup() {
  Serial.begin(9600);   //Initialize serial connection
  Serial.println("Starting bridge and serial ...\n");

  delay(1000);

  Bridge.begin();       //initialize the bridge with linux part


  // Put SCK, MOSI, SS pins into output mode
  // also put SCK, MOSI into LOW state, and SS into HIGH state.
  // Then put SPI hardware into Master mode and turn SPI on
  SPI.begin ();

  // Slow down the master a bit
  SPI.setClockDivider(SPI_CLOCK_DIV8);

  //initialize pin
  pinMode (PINRED, OUTPUT);
  pinMode (PINGREEN, OUTPUT);
  pinMode (PINBLUE, OUTPUT);
  pinMode (RESET_ARDUINO_PIN, OUTPUT);

  //reset arduino board
  digitalWrite(RESET_ARDUINO_PIN, LOW);

  // pinMode (PINBUTTON, INPUT);

  //Run an initial process to get the time
  if (!date.running())  {
    date.begin("date");
    date.addParameter("+%T");
    date.run();
  }

  //reset arduino board
  digitalWrite(RESET_ARDUINO_PIN, HIGH);

  Serial.println("\n\n\n\n--------  END INIT -------\n\n\n\n");
}

// Function that finds the best hour to start charging and the average price
void Start_Average ()
{
  Serial.println("Starting calculate average ...");
  float sum = 0;
  float sum_best = -1;
  float sum_old = -1;
  desired_duration = 10;

  Serial.print("Calculating ... ");
  for (int i = 0; i < 34 - desired_duration; i++) {
    sum_old = sum;
    sum = 0;
    for (int y = 0; y < desired_duration; y++) {
      sum = sum + finalPrices[i + y];
    }
    if (i == 1) {
      sum_best = sum;
    }
    else {
      if (sum < sum_best) {
        start_hour_best = i;
        sum_best = sum;
      }
    }
  }
  average_price = sum_best / desired_duration;
  Serial.print("Sum best : ");
  Serial.print(sum_best);
  Serial.print(" desired duration : ");
  Serial.print(desired_duration);
  Serial.print(" Average : ");
  Serial.println(average_price);
}


void loop() {
  bool Cursor = false;

  switch (programStat)
  {
    case STAT_START:
      printScreen(UART_MENU_TYPE, MENU_TYPE_START); //display the starting screen
      if (!wifi) { // check the wifi only when the device downloads the prices, not for charging!
        routine();
        Serial.println("switch on light");
        Serial.println("Waiting for Wifi ...");
        setLED(RED);
        while (!(wifi = getWifiStatus(false)))
          printScreen(UART_MENU_TYPE, MENU_TYPE_ERROR); //display the starting screen
        getTime(false);
        Serial.println (" done"); // the alternative is to understand how to delete messages from the screen
        setLED(NONE);
      }
      printScreen(UART_MENU_TYPE, MENU_TYPE_START); //display the starting screen

      //update programm
      printScreen(UART_HOUR, timeCustom.getHour());
      printScreen(UART_MINUTE, timeCustom.getMinute());
      httpClient();
      // refresh finalPrices with new prices (overwrite the array each day)

      programStat = STAT_CONFIG;
      break;

    /*
      First screen where the user configure the duration time
    */
    case STAT_CONFIG:
      Serial.println("Config screen");
      desired_duration = 0;
      printScreen(UART_MENU_TYPE, MENU_TYPE_CONFIG); //display the starting screen
      printScreen(UART_CURSOR_SCREEN, CURSOR_START_TIME); //display the starting screen

      while (!buttonIsPressed)
      {
        routine();
      }
      buttonIsPressed = false;

      printScreen(UART_CURSOR_SCREEN, CURSOR_RANGE_CHARGING);

      while (!buttonIsPressed)
      {
        routine();
      }
      buttonIsPressed = false;

      printScreen(UART_CURSOR_SCREEN, CURSOR_END_TIME); 

      while (!buttonIsPressed)
      {
        routine();
      }
      buttonIsPressed = false;

      printScreen(UART_CURSOR_SCREEN, CURSOR_CHOICE); 

      while (!buttonIsPressed)
      {
        routine();
      }
      buttonIsPressed = false;

      Start_Average ();

      printScreen(UART_AVERAGE_PRICE, average_price);
      printScreen(UART_CHARGING_DURATION, desired_duration);

      programStat = STAT_INFORMATION;
      break;

    /*
      screen that display information about the best hour, the average price
      user has to agree on charging
    */
    case STAT_INFORMATION:
      Serial.println("Info screen");
      Cursor = true;
      printScreen(UART_MENU_TYPE, MENU_TYPE_INFORMATION);
      printScreen(UART_AVERAGE_PRICE, average_price);
      printScreen(UART_CHARGING_DURATION, desired_duration);
      while (!buttonIsPressed)
      {
        routine();
      }
      buttonIsPressed = false;

      information_screen = false;
      if (Cursor)
        programStat = STAT_PROCESS;
      else
        programStat = STAT_CONFIG;
      break;

    /*
       The car is charging
    */
    case STAT_PROCESS:
      Serial.println("Progress screen");
      process_screen = false;
      printScreen(UART_MENU_TYPE, MENU_TYPE_PROCESS);

      printScreen(UART_END_TIME, (int)(desired_duration + start_hour_best) % 24);
      printScreen(UART_START_TIME, start_hour_best);

      while (flag)
      {
        routine();
        //diaplay the remaining time
        int remainingTime = (timeCustom.getHour() * 60 + timeCustom.getMinute()) - start_time + desired_duration;
        printScreen(UART_REMAINING_TIME, remainingTime > 0 ? remainingTime : desired_duration);
      }
      while (!buttonIsPressed)
      {
        routine();
      }
      buttonIsPressed = false;

      programStat = STAT_END;
      break;

    /*
       Process terminated
    */
    case STAT_END:
      Serial.println("End screen");
      Cursor = true;
      end_screen = false;
      printScreen(UART_MENU_TYPE, MENU_TYPE_END);

      while (!buttonIsPressed)
      {
        routine();
      }
      buttonIsPressed = false;
      break;
  }
}

