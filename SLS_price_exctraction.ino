/*
  Yún HTTP Client

 This example for the Arduino Yún shows how create a basic
 HTTP client that connects to the internet and downloads
 content. In this case, you'll connect to the Arduino
 website and download a version of the logo as ASCII text.

 created by Tom igoe
 May 2013

 This example code is in the public domain.

 http://arduino.cc/en/Tutorial/HttpClient

 */

#include <Bridge.h>
#include <HttpClient.h>
#include <YunClient.h>
#include <StandardCplusplus.h>
#include <serstream>
#include<vector>
#include<iterator>
//std::vector <vector<int> > grid;
//std::vector<int> col(5,3);
void setup() 
{
  // Bridge takes about two seconds to start up
  // it can be helpful to use the on-board LED
  // as an indicator for when it has initialized
  pinMode(13, OUTPUT); //setting 13th pin as output
  digitalWrite(13, LOW); //setting 13th pin low state
  Bridge.begin(); // starting yun bridge
  Serial.print("Bridge has begun");
  digitalWrite(13, HIGH); // setting the 13th pin as high

  Serial.begin(9600); // starting serial communication
  //grid.push_back(col);

  while (!Serial); // wait for a serial connection
}

void loop() 
{
  // Initialize the client library
  HttpClient client;
  // Make a HTTP request:
  client.get("http://slsoptimizationservice.cloudapp.net/json/reply/PriceRequest");
  
  int sum = 0; // a varible which is going to increase to get the right spot in the string
  int counter = 0; // counter which is going to increase until one hour price is known
  int hoursknown = 0; // is calculating how many hours are known
  std::string prices[36]; // a sting array for putting prices
  Serial.print("Made request to SLS server ");
  while (client.available()) // when recieving data from server
  {
    char c = client.read(); // is reading one character
    
    Serial.print(c); //is printing out the one character
    if (counter == 5) // this is controlling if counter has reached number 5, if it is it is going to reset
    {
      counter = 0;
      sum = 0;
      hoursknown++;
      //Serial.print(',');
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
      if (c != ' ')// in here it is checking if read value is not a space
      {
        prices[hoursknown].push_back(c); // if it is not the number is added to the string
        //std::cout << c;
        //Serial.print(c);
      }
      counter++; // counter is increased by 1
      //prizes.append(data[i]);
    }
    else // if the c value is not matching then it is reseted
    {
      sum=0;
    }
  }
  //double elprize = atof(prizes[0].c_str()); 
  std::vector<double> iprices; // creating an vector for prices
  for (int i = 0; i < hoursknown; i++)//it's a loop where it is reading every hour price into the vector
  {
    //char* pEnd;
    //num = atof(prizes[hoursknown].c_str());
    iprices.push_back(atof(prices[i].c_str())); // this is converting string to double and putting into vector
    Serial.print("The price is on hour ");
    Serial.print(i); 
    Serial.print(" from now: ");
    Serial.println(iprices[i]);
  }
  Serial.flush();
  delay(5000);
}

