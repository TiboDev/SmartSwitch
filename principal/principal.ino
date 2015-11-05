#include <Bridge.h>
#include <Console.h>
#include <FileIO.h>
#include <HttpClient.h>
#include <Mailbox.h>
#include <Process.h>
#include <YunClient.h>
#include <YunServer.h>

# include <math.h>


const int PINRED = 11;
const int PINBLUE = 12;
const int PINGREEN = 13;
const int PINTRANS = 9;
//const int PINBUTTON = ;

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
int plugged = -1; // it is 0 if nothing is plugged and 1 if something is. This value comes from the current sensor, that activates an INPUT pin (so actually we won't need this variable)

/*
 * Part to get the synchronize time
 */

Process date;
int hours, minutes, seconds;
int lastSecond = -1;

/*
 * Refresh the time -should work
 */
void getTime()
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
        hours = hourString.toInt();
        minutes = minString.toInt();
        lastSecond = seconds;
        seconds = secString.toInt();
      }
    }
  }
}

/*
 * Process of Wifi - not finished
 */
bool getWifiStatus()
{
  Process wifiCheck;
  wifiCheck.runShellCommand("/usr/bin/pretty-wifi-info.lua");

  return false;
}

void setup() {
  Serial.println("Starting bridge and serial ...\n");
  Serial.begin(9600);   //Initialize serial connection
  Bridge.begin();       //initialize the bridge with linux part

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
}

void loop() {
  //# Why if (midnight == 1) ?
  if (midnight == 1) {
    if (!wifi) { // check the wifi only when the device downloads the prices, not for charging!
      digitalWrite (PINRED, HIGH);
      digitalWrite (PINGREEN, HIGH);
      Serial.print ("ERROR: The WIFI is not locked");
      while (!(wifi = getWifiStatus()));
      Serial.print ("Now the wifi is working!"); // the alternative is to understand how to delete messages from the screen
      digitalWrite (PINRED, LOW);
      digitalWrite (PINGREEN, LOW);
    }
    // refresh array_prices with new prices (overwrite the array each day)

    // FUNCTION that displays the best our to start, so the user can decide to switch the device off and switch it on again when the time is close to the best hour
    Serial.print ("Best hour to start: ");
    Serial.print (start_hour_best);

    // FUNCTION that calculates the average price
    Serial.print ("Average price: ");
    Serial.print (average_price);
  }


  // if (pin main button == 0), then exit the code


  if (plugged == 0) {
    digitalWrite (PINRED, HIGH);
    digitalWrite (PINGREEN, HIGH);
    Serial.print ("ERROR: Nothing is plugged");
    while (!plugged);
    Serial.print ("Now you have plugged a device!"); // the alternative is to understand how to delete messages from the screen
    digitalWrite (PINRED, LOW);
    digitalWrite (PINGREEN, LOW);
  }

  //#I do not understand why is it for ?
  // you are doing the same thing in both conditions
  if (start_hour_user == -1) {
    if (watch == start_hour_best) {
      digitalWrite (PINTRANS, HIGH);
      flag = true;
    }
  }
  else {
    if (watch == start_hour_user) {
      digitalWrite (PINTRANS, HIGH);
      flag = true;
    }
  }

  if (flag) {
    Serial.print ("Remaining time: ");
    Serial.print (desired_duration - time_passed);

    if (desired_duration == time_passed) {
      digitalWrite (PINTRANS, LOW);
      Serial.print ("Charging finished");
      digitalWrite (PINRED, HIGH);
      digitalWrite (PINGREEN, HIGH);
      digitalWrite (PINBLUE, HIGH);
      start_hour_user = -1;
      start_hour_best = -1;
      desired_duration = -1;
      time_passed = -1;
      flag = false;
    }
  }

  getTime();

}
