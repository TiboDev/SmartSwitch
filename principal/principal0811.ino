// ALICE ultima modifica 23/11
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

const int PINRED = 11;
const int PINBLUE = 12;
const int PINGREEN = 13;
const int PINTRANS = 9;

const int SHIPSELECT = 10;

/*
 * 3 states:
 * 0 inactive
 * INCREASE
 * DECREASE
 */
unsigned char potentiometer_state = 0;

// we define all the variables as int and float, but we can switch to double later in order to occupy less memory
int hourupdate = -1; // it is 0 if it isn't 2pm and 1 if it is
float watch = -1; // variable that knows what time it is
bool wifi = false; // it is 0 if the wifi doesn't work and 1 if it works    #modify in bool
float array_prices [34]; // array with 34 elements, because every 2pm we get the prices from 2pm to midnight of the following day
int start_hour_best = -1; // it is the best hour suggested by the device
float average_price = -1; // average price estimated considering the best hours for charging the device (euros/hour)
float start_hour_user = -1; // the user can decide to start chargin in a different moment from the one suggested
float desired_duration = -1; // duration of the charging set by the user, time that the device needs to be charged
float time_passed = -1; // variable that counts the time starting from the beginning of the charging
bool flag = false;         //variable to catch if charging process has started
// int start_charging = -1; // it is 1 when the user decide to start charging, 0 otherwise
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
 * Part to get the synchronize time
 */

Process date;

/*
 * Function to send message to Arduino board
 *
 * WARNING the value to send has to be max 255 !
 */
void printScreen(unsigned char sendCode, unsigned int toSend)
{
  // enable Slave Select
  digitalWrite(10, LOW);    // SS is pin 10
  digitalWrite(13, HIGH);

  // send test string
  SPI.transfer (sendCode);
  SPI.transfer (toSend);

  SPI.transfer ('\n');

  // disable Slave Select
  digitalWrite(10, HIGH);

  delay (300);
  digitalWrite(13, LOW);
  delay (150);
}

/*
 * Refresh the time -should work
 */
void getTime(bool Display)
{
  if (timeCustom.secondChanged()) {
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
          if (timeCustom.hourChanged())
            printScreen(UART_HOUR, timeCustom.getHour());
          if (timeCustom.minuteChanged())
            printScreen(UART_MINUTE, timeCustom.getMinute());
        }
      }
    }
  }
}

/*
 * Control LED
 */

void setLED(unsigned char R, unsigned char G, unsigned char B)
{
  analogWrite(PINRED, R);
  analogWrite(PINGREEN, G);
  analogWrite(PINBLUE, B);
}

void setLED(unsigned char color)
{
  switch (color)
  {
    case RED:
      setLED(254, 0, 0);
      break;
    case GREEN:
      setLED(0, 254, 0);
      break;
    case BLUE:
      setLED(0, 0, 254);
      break;
    case WHITE:
      setLED(254, 254, 254);
      break;
    case BLACK:
        setLED(0, 0, 0);
      break;
  }
}

/*
 * Process of Wifi
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
 * Function HTTP
 */

void httpClient()
{
  // Initialize the client library
  YunClient client;

  String host = "slsoptimizationservice.cloudapp.net";
  IPAddress server(104, 45, 93, 96); // or number format IPAddress server(XX,XX,XX,XX)
  String path = "/json/reply/OptimizeProducer";
  String body = "{\"producer\":{\"maximumPower\":[1,1,1,1,1,1,1,1,0],\"minimumPower\":0}, \"source\":{\"initialLevel\":0,\"maximumLevel\":0,\"minimumLevel\":0,\"finalLevel\":0}, \"storage\":{\"initialLevel\":0.5,\"finalLevel\":1,\"minimumLevel\":0,\"maximumLevel\":1}, \"deviceID\":\"tcFYbMlB7O5NlijlhZQK0neMmwW9gARvowitomosS0U%3d\"}";           // String representation of JSON body

  if (client.connect(server, 80)) {
    Serial.println("connected");
    delay(2500);

    client.println("POST " + path + " HTTP/1.1");
    client.println("Host: " + host);
    client.print("Content-length:");
    client.println("Connection: Close");
    client.println("Content-Type: application/json;");
    client.println();
    client.println(body);
  }

  // if there are incoming bytes available
  // from the server, read them and print them:
  while (client.available()) {
    char c = client.read();
    Serial.print(c);
  }
  Serial.flush();
}

// Function that checks the wifi, gets the prices, display the best hour to start and the average price
void device_settings ()
{
  if (!wifi) { // check the wifi only when the device downloads the prices, not for charging!
    setLED(YELLOW);
    Serial.print ("ERROR: The WIFI is not locked");
    while (!(wifi = getWifiStatus(false)))
      getTime(false);
    //Serial.print ("Now the wifi is working!"); // the alternative is to understand how to delete messages from the screen
    setLED(BLACK);
  }
  // refresh array_prices with new prices (overwrite the array each day)


  void Start_Average (); // FUNCTION that displays the best our to start, so the user can decide to switch the device off and switch it on again when the time is close to the best hour
  // It also calculates the average price
  Serial.print ("Best hour to start: ");
  Serial.print (start_hour_best);
  Serial.print ("Average price: ");
  Serial.print (average_price);
}

void routine()
{
  getTime(true);
  printScreen(UART_WIFI_STATUS, getWifiStatus(false));
}

void setup() {
  Serial.begin(9600);   //Initialize serial connection
  Bridge.begin();       //initialize the bridge with linux part

  Serial.println("Starting bridge and serial ...\n");

  pinMode(10, OUTPUT);
  digitalWrite(SHIPSELECT, HIGH);  // ensure SS stays high for now

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
  // pinMode (PINBUTTON, INPUT);

  //Run an initial process to get the time
  if (!date.running())  {
    date.begin("date");
    date.addParameter("+%T");
    date.run();
  }

  Serial.println("endInit");
}

// Function that finds the best hour to start charging and the average price
void Start_Average ()
{
  float sum = 0;
  float sum_best = -1;
  float sum_old = -1;

  for (int i = 1; i = 34 - desired_duration + 1; i++) {
    sum_old = sum;
    sum = 0;
    for (int y = 1; y = desired_duration; y++) {
      sum = sum + array_prices[i + y - 1];
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
}


void loop() {

  switch (programStat)
  {
    case STAT_START:
      if (!wifi) { // check the wifi only when the device downloads the prices, not for charging!
        setLED(YELLOW);
        //Serial.println ("ERROR: The WIFI is not locked");
        /*while (!(wifi = getWifiStatus(false)))
          getTime(false);*/
        Serial.println ("Now the wifi is working!"); // the alternative is to understand how to delete messages from the screen
        setLED(BLACK);
      }
      // refresh array_prices with new prices (overwrite the array each day)

      // FUNCTION that displays the best our to start, so the user can decide to switch the device off and switch it on again when the time is close to the best hour
      Serial.println ("Best hour to start: ");
      Serial.println (start_hour_best);

      // FUNCTION that calculates the average price
      Serial.println ("Average price: ");
      Serial.println (average_price);
      printScreen(UART_MENU_TYPE, MENU_TYPE_START); //display the starting screen
      start = false;
      config_screen = false;
      break;

    /*
    * First screen where the user configure the duration time
    */
    case STAT_CONFIG:
      desired_duration = 0;
      while (!buttonIsPressed)
      {
        if (potentiometer_state == INCREASE)
        {
          if (desired_duration < 254)
          {
            desired_duration++;
            potentiometer_state == 0;
            printScreen(UART_CONFIG_DURATION, desired_duration);
          }
        }
        else
        {
          if (potentiometer_state == DECREASE)
          {
            if (desired_duration > 0)
            {
              desired_duration--;
              potentiometer_state == 0;
              printScreen(UART_CONFIG_DURATION, desired_duration);
            }
          }
        }
      }
      config_screen = false;
      information_screen = true;
      break;

    /*
    * screen that display information about the best hour, the average price
    * user has to agree on charging
    */
    case STAT_INFORMATION:
      bool Cursor = true;
      printScreen(UART_MENU_TYPE, MENU_TYPE_BEST_HOUR);
      printScreen(UART_AVERAGE_PRICE, average_price);
      printScreen(UART_CHARGING_DURATION, desired_duration);
      while (!buttonIsPressed)
      {
        if (potentiometer_state == DECREASE)
        {
          printScreen(UART_CURSOR_SCREEN, CURSOR_OK);
          Cursor = true;
        }
        else
        {
          if (potentiometer_state == INCREASE)
          {
            printScreen(UART_CURSOR_SCREEN, CURSOR_RETURN);
            Cursor = false;
          }
        }
      }

      information_screen = false;
      if (Cursor)
        process_screen = true;
      else
        config_screen = true;
      break;

    /*
     * The car is charging
     */
    case STAT_PROCESS:
      process_screen = false;
      printScreen(UART_MENU_TYPE, MENU_TYPE_PROCESS);

      printScreen(UART_END_TIME, (int)(desired_duration + start_hour_best) % 24);
      printScreen(UART_START_TIME, start_hour_best);

      while (flag)
      {
        //diaplay the remaining time
        int remainingTime = (timeCustom.getHour() * 60 + timeCustom.getMinute()) - start_time + desired_duration;
        printScreen(UART_REMAINING_TIME, remainingTime > 0 ? remainingTime : desired_duration);
      }

      end_screen = true;
      break;

    /*
     * Process terminated
     */
    case STAT_END:
      bool Cursor = true;
      end_screen = false;
      printScreen(UART_MENU_TYPE, MENU_TYPE_END);

      while (!buttonIsPressed)
      {
        if (potentiometer_state == DECREASE)
        {
          printScreen(UART_CURSOR_SCREEN, CURSOR_OK);
          Cursor = true;
        }
        else
        {
          if (potentiometer_state == INCREASE)
          {
            printScreen(UART_CURSOR_SCREEN, CURSOR_RETURN);
            Cursor = false;
          }
        }
      }
      break;
  }
// ALICE Are all the serial print still useful? Some of them are already written above..
  if (!plugged) {
    setLED (YELLOW);
    Serial.println ("ERROR: Nothing is plugged, plug it ...");
    while (!plugged);
    Serial.println ("Now you have plugged a device!"); // the alternative is to understand how to delete messages from the screen
    setLED(BLACK);
  }

  //#I do not understand why is it for ?
  // you are doing the same thing in both conditions
  if (watch == start_hour_best)
  {
    digitalWrite (PINTRANS, HIGH);
    flag = false;
  }

  if (flag) {
    Serial.println ("Remaining time: ");
    Serial.println (desired_duration - time_passed);

    if (desired_duration == time_passed) {
      digitalWrite (PINTRANS, LOW);
      //Serial.println ("Charging finished");
      setLED(WHITE); // ALICE when do we have to switch off this LEDs?
      start_hour_user = -1;
      start_hour_best = -1;
      desired_duration = -1;
      time_passed = -1;
      flag = false;
      start = true; //to restart checking price
    }
  }
}
