#include <Bridge.h>
#include <Console.h>
#include <FileIO.h>
#include <HttpClient.h>
#include <Mailbox.h>
#include <Process.h>
#include <YunClient.h>
#include <YunServer.h>

#include <math.h>


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
float charging_duration = -1; // time that the device needs to be charged


// FUNCTION PROTOTYPES
void Start_Average (void);
void device_settings (void);
void getTime(void);
bool getWifiStatus(bool);



/*
   Part to get the synchronize time
*/

Process date;
int hours, minutes, seconds;
int lastSecond = -1;


void setup() {
  Serial.println ("Starting bridge and serial ...\n");
  Serial.begin (9600);   //Initialize serial connection
  Bridge.begin ();       //initialize the bridge with linux part

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
  device_settings ();
}

void loop() {
  
  getTime();

  // CHECKING IF IT IS MIDNIGHT
  
  //# Why if (midnight == 1) ? 
  // Answer: if the divice remains ON for some days we should get the new prices every day at midnight. It is not enough to put this part only in the settings (I added it in the void setpu)
  if (midnight == 1) {
    // Function for the device settings
    device_settings ();
  }


  // if (pin main button == 0), then exit the code


  // CHECKING IF THERE IS SOMETHING PLUGGED
  if (plugged == 0) {
    digitalWrite (PINRED, HIGH);
    digitalWrite (PINGREEN, HIGH);
    Serial.print ("ERROR: Nothing is plugged");
    while (!plugged);
    Serial.print ("Now you have plugged a device!"); // the alternative is to understand how to delete messages from the screen
    digitalWrite (PINRED, LOW);
    digitalWrite (PINGREEN, LOW);
  }



  // START CHARGING

  //#I do not understand why is it for ? you are doing the same thing in both conditions
  //Answer: the user can decide to make the charging starting at the best hour suggested or at another hour.
  //# If the user doens't modify the variable start_hour_user it means that he wants to start charging at the best hour, otherwise he puts the hour at which he wants to start in the variable.

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




  // END CHARGING
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


}








// FUNCTIONS

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

// Function that checks the wifi, gets the prices, display the best hour to start and the average price
// ATT : Is it possible to call a function in other function???
void device_settings () 
{
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


    void Start_Average (); // FUNCTION that displays the best our to start, so the user can decide to switch the device off and switch it on again when the time is close to the best hour
    // It also calculates the average price
    Serial.print ("Best hour to start: ");
    Serial.print (start_hour_best);
    Serial.print ("Average price: ");
    Serial.print (average_price);
}


// Function to refresh the time
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
        if (hours == 0 && minutes == 0 && seconds == 0) {
          midnight = 1;
        }
      }
    }
  }
}


// Function that processes wifi status - not finished
bool getWifiStatus()
{
  Process wifiCheck;
  wifiCheck.runShellCommand("/usr/bin/pretty-wifi-info.lua");

  return false;
}

