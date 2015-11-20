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
int midnight = -1; // it is 0 if it isn't midnight and 1 if it is
float watch = -1; // variable that knows what time it is
bool wifi = false; // it is 0 if the wifi doesn't work and 1 if it works    #modify in bool
float array_prices [24];
int start_hour_best = -1; // it is the best hour suggested by the device
float average_price = -1;
float start_hour_user = -1; // the user can decide to start chargin in a different moment from the one suggested
float desired_duration = -1; // duration of the charging set by the user
float time_passed = -1; // variable that counts the time starting from the beginning of the charging
bool flag = false;         //variable to catch if charging process has started
// int start_charging = -1; // it is 1 when the user decide to start charging, 0 otherwise
bool plugged = true; // it is 0 if nothing is plugged and 1 if something is. This value comes from the current sensor, that activates an INPUT pin (so actually we won't need this variable)

bool config_screen = false;
bool information_screen = false;
bool start = true;
bool end_screen = false;
bool buttonIsPressed = false;
bool process_screen = false;

int start_time = 0;

/*
 * Part to get the synchronize time
 */

Process date;
int hours = -1, minutes = -1, seconds = -1;
int lastSecond = 0, lastMinute = 0, lastHour = 0;

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
  if (lastSecond != seconds) {
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
        lastSecond = seconds;
        lastMinute = minutes;
        lastHour = hours;
        hours = hourString.toInt();
        minutes = minString.toInt();
        seconds = secString.toInt();

        if (Display)
        {
          if (lastHour != hours)
            printScreen(UART_HOUR, hours);
          if (lastMinute != minutes)
            printScreen(UART_MINUTE, minutes);
          if (lastSecond != seconds)
            printScreen(UART_SECOND, seconds);
        }
      }
    }
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
  if (hours >= 0)
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
// ATT : Is it possible to call a function in other function???
void device_settings ()
{
  if (!wifi) { // check the wifi only when the device downloads the prices, not for charging!
    digitalWrite (PINRED, HIGH);
    digitalWrite (PINGREEN, HIGH);
    Serial.print ("ERROR: The WIFI is not locked");
    while (!(wifi = getWifiStatus(false)))
      getTime(false);
    //Serial.print ("Now the wifi is working!"); // the alternative is to understand how to delete messages from the screen
    digitalWrite (PINRED, LOW);
    digitalWrite (PINGREEN, LOW);
  }
  // refresh array_prices with new prices (overwrite the array each day)


  void Start_Average (); // FUNCTION that displays the best our to start, so the user can decide to switch the device off and switch it on again when the time is close to the best hour
  // It also calculates the average price
  Serial.print ("Best hour to start: ");
  Serial.print (start_hour_best);
  Serial.print ("Average price: ");
  Serial.print (average_price);
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
  /* This is the Matlab code that we used to verify the functionality of this function. This code must be traslated in the right language for Arduino
  sum = 0;
  sum_best = -1;
  sum_old = -1;
  start_best_hour = -1;
  average_price = -1;
  charging_duration = 3;
  array_prices = [4,1,5,3,4,5,6,5,54,4,15,1,2,1,5,5,76,6,63,2,7,5,3,3];

  for i = 1 : (24-(charging_duration)+1)
    sum_old = sum;
    sum = 0;
    for y = 1:(charging_duration)
        sum = sum + array_prices (i+y-1);
    end


    if i == 1
        sum_best = sum;
    end

    if i ~= 1
       if (sum < sum_best)
           start_best_hour = i;
           sum_best = sum;

       end
    end

  end
    average_price = sum_best/charging_duration;


  */
}

/* syntax for Arduino but the code is not correct
 int i = -1;
 int y = -1;
 int sum = 0;
 int sum_best = -1;
 int sum_old = -1;
 for (i = 0; i < 24 - charging_duration + 1; i++) {
   sum_old = sum;
   for (y = 0; y < charging_duration; y++ ) {
     sum = sum + array_prices [i + y];
   }
   if (sum < sum_old) {
     start_hour_best = i;
     sum_best = sum;
   }
 }
 average_price = sum_best / charging_duration;

}*/

void loop() {
  if (start) {
    if (!wifi) { // check the wifi only when the device downloads the prices, not for charging!
      digitalWrite (PINRED, HIGH);
      digitalWrite (PINGREEN, HIGH);
      //Serial.println ("ERROR: The WIFI is not locked");
      /*while (!(wifi = getWifiStatus(false)))
        getTime(false);*/
      Serial.println ("Now the wifi is working!"); // the alternative is to understand how to delete messages from the screen
      digitalWrite (PINRED, LOW);
      digitalWrite (PINGREEN, LOW);
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
  }

  /*
   * First screen where the user configure the duration time
   */
  if (config_screen)
  {
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
  }

  /*
   * screen that display information about the best hour, the average price
   * user has to agree on charging
   */
  if (information_screen)
  {
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
  }

  if (process_screen)
  {
    process_screen = false;
    printScreen(UART_MENU_TYPE, MENU_TYPE_PROCESS);

    printScreen(UART_END_TIME, (int)(desired_duration + start_hour_best) % 24);
    printScreen(UART_START_TIME, start_hour_best);

    while (flag)
    {
      //diaplay the remaining time
      printScreen(UART_REMAINING_TIME, ((hours * 60 + minutes) - start_time + desired_duration) > 0 ? ((hours * 60 + minutes) - start_time + desired_duration) : desired_duration);
    }

    end_screen = true;
  }

  if (end_screen)
  {
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
  }

  if (!plugged) {
    digitalWrite (PINRED, HIGH);
    digitalWrite (PINGREEN, HIGH);
    Serial.println ("ERROR: Nothing is plugged, plug it ...");
    while (!plugged);
    Serial.println ("Now you have plugged a device!"); // the alternative is to understand how to delete messages from the screen
    digitalWrite (PINRED, LOW);
    digitalWrite (PINGREEN, LOW);
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
      digitalWrite (PINRED, HIGH);
      digitalWrite (PINGREEN, HIGH);
      digitalWrite (PINBLUE, HIGH);
      start_hour_user = -1;
      start_hour_best = -1;
      desired_duration = -1;
      time_passed = -1;
      flag = false;
      start = true; //to restart checking price
    }
  }

  //httpClient();
  getTime(true);
  printScreen(UART_WIFI_STATUS, getWifiStatus(false));
}
