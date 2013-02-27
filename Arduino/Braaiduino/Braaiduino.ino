// include the library code:
#include <SPI.h>
#include <WiFi.h>
#include <SD.h>
#include <LiquidCrystal595.h>


// Configuration variables for network / updating 
char ssid[] = "Penguinpowered";     //  your network SSID (name)
char pass[] = "Lemmings live here";    // your network password
IPAddress serverAddress(192,168,10,28);   // Address of the server to communicate with
String webServiceIP = "192.168.10.28";
int serverPort = 3000;

long displayTempUpdateInterval = 15000; // the amount of time (in millseconds) between performing temperature checks (30s)
long netTempUpdateInterval = 120000; // the amount of time between updating the server (180s / 3 min)

// initialize the library with the numbers of the interface pins
LiquidCrystal595 lcd(5,6,8);
int ambientTemperaturePin = A0;
int foodTemperaturePin = A4;
int pitTemperaturePin = A5;
const int startButtonPin = 3; 
const int endButtonPin = 2;

// Values to track button states and action states
int buttonCount = 0;
int startButtonState = 0;
int endButtonState = 0;
int cookState = 0;
boolean sdActive = false;

// Display interval and other info for output
unsigned long previousTempDisplayMillis = millis();
unsigned long previousTempNetMillis = millis();
String cookId = "0";   //cookId will be used throughout the life of the app to communicate with the webservice
String endStatus = "";
String lastTempDisplayed = "";


int wifiStatus = WL_IDLE_STATUS;     // the Wifi radio's status
WiFiClient client;
boolean lastConnected = false; 

/* setup
   Setup various pin modes, LCD display and ensure that we are connected to the wifi network
*/
void setup() {
  Serial.begin(115200);
  Serial.println(F(""));
  Serial.println(F("Arduino in setup()"));

  pinMode(startButtonPin, INPUT);
  pinMode(endButtonPin, INPUT);

  lcd.begin(16, 2);
  connectToNetwork();  
  
  // set up the LCD's number of columns and rows: 
  lcd.setCursor(0,0);
  lcd.print(F("Amb:  "));
   
  showCookState(0);
}

/* loop
   Each time we run through the loop, do the following
   - read start and end button states and act on these
   - get temperatures from probes
   - update webservice with temperatures if a cook is active
   - track how long since we last performed a check on temperatures
*/
void loop() {

  // Read start and end buttons and act on changes there
  checkStartEndButtons();
 
  
  lcd.setCursor(14, 0);
  updateTemperatures();
}


/* checkStartEndButtons()
Each time through the loop, we need to see if the user has pushed the start or end button. If they have, then we 
   - Start / end the cook by sending a REST request
   - Set cookState to show whether a cook is active or not right now
*/
void checkStartEndButtons() {
  startButtonState = digitalRead(startButtonPin);
  endButtonState = digitalRead(endButtonPin);
  if (startButtonState == LOW && cookState == 0) {
    cookState = 1;
    startCook(cookState);
  }
  
  if (endButtonState == LOW && cookState == 1) {
    cookState = 0;
    endCook(cookState);
  }
}


/* findWebserviceReturnId(string readString)
   Find the cookID in the content sent by the webservice after the cook button is pressed.

   readString contains the JSON portion of the response from the webservervice.
   The format of readString should be 
   {"result":INT}
   INT is the id field from the recently created row in the database
*/
String findWebserviceReturnId(String readString) {
  int resultLocation = readString.lastIndexOf("\"result\":");

  String tmpId = readString.substring(resultLocation + 9, readString.length());
  Serial.print(F("Webservice Return ID: "));
  Serial.println(tmpId);
  
  return tmpId;
}

/* findEndStatus
   Ensure that when the endButton was pressed, the webservice correctly ends the cook
  
   endStatus contains the JSON portion of the response from the webservice.
   The format of endStatus should be 
   {"id": INT, "status": "ended"}
   INT is the cookId that we get from findWebserviceReturnId
*/
String findEndStatus(String endStatus) {
  
  int resultLocation = endStatus.lastIndexOf("\"status\":");

  String tmpEndStatus = endStatus.substring(resultLocation + 10 , endStatus.length() - 1);
  Serial.println(F("END STATUS:"));
  Serial.println(tmpEndStatus);
  Serial.println(F("----"));

  lcd.setCursor(9, 1);
  lcd.print(F("ID:DONE"));
  if (tmpEndStatus != "ended") {
    showCookState(-1);
  }
  return tmpEndStatus;
}

/* padTemp
   Pad temperatures to 3 characters
   This is required to clear spaces where a lager temperature would leave 
   characters on the screen when the next temp is displayed
*/
String padTemp(int temp) {
  String tempString = String(temp);
  
  if (temp < 100 && temp > 10) {
    tempString = tempString + " ";
  } else if (temp < 10) {
    tempString = tempString + "  ";
  }
 
 return tempString; 
}

/* showCookState
   Display the state of the cook on the LCD
   0 = OFF / No cook happening
   1 = ON / Cooking
   -1 = Failure of some kind
*/
void showCookState(int cookState) {
  lcd.setCursor(12, 0);
  if (cookState == 0) {
    lcd.print(F("OFF "));
  } else if (cookState == 1) {
    lcd.print(F("COOK"));
  } else if (cookState == -1) {
    lcd.print(F("FAIL"));
  }
}

/* startCook
   Carry out the actions to start a cook. This involves communicating with the webservice and updating
   a number of values and the LCD display.
   
   The main result from this is that we will have a cookId that can be used to send temperatures to the 
   webservice.
*/
void startCook(int cookState) {
  String startCookResponse = sendCookNetRequest("POST", "{\"action\":\"startcook\"}");
  cookId = findWebserviceReturnId(startCookResponse);

  // If we get a cookID of 0 back, something went wrong, so we need to show that the cook has failed  
  if (cookId == "0") {
    cookState = 0;
    showCookState(-1);
  } else {
    // Update the LCD with the cook ID
    String idString = "ID:" + cookId;
  
    // padLength is used to ensure that we display the ID on the LCD without leaving any existing text
    int padLength = 7 - idString.length();
    for (int i = 0; i < padLength; i++) {
      idString = idString + " ";
    }
    lcd.setCursor(9, 1);
    lcd.print(idString);
  }
  
  showCookState(cookState);
}

/* endCook
   Carry out the actions to end a cook. This involves communicating with the webservice and updating
   a number of values and the LCD display
*/
void endCook(int cookState) {
  String endCookResponse = sendCookNetRequest("PUT", "{\"action\":\"endcook\"}");
  endStatus = findEndStatus(endCookResponse);
  showCookState(cookState); 
}

/* updateTemperatures
   - get new temperatures from the probes, 
   - update the webservice if a cook is active
   - return the new number of milliseconds since startup
   
   Each time through the loop, this method is called, but we don't want to do the work each time
   so we only do it every number of milliseconds defined in displayTempUpdateInterval
*/
void updateTemperatures() {

  unsigned long currentMillis = millis();
  
  if(currentMillis - previousTempDisplayMillis > displayTempUpdateInterval) {  
    previousTempDisplayMillis = currentMillis;
    float ambient_tempFloat = getVoltage(ambientTemperaturePin);  
    ambient_tempFloat = ((ambient_tempFloat - .5) * 100);
    int ambient_temp = (int)ambient_tempFloat;

    int pit_temp = thermister_temp(analogRead(pitTemperaturePin));
    int food_temp = thermister_temp(analogRead(foodTemperaturePin));

    lastTempDisplayed = getTempToDisplay(lastTempDisplayed);

    Serial.print(F("Pit temp: "));
    Serial.println(pit_temp);
    
    Serial.print(F("Food temp: "));
    Serial.println(food_temp);

    Serial.print(F("Ambient temp: "));
    Serial.println(ambient_temp);
    
    Serial.println(F(""));
    
    lcd.setCursor(0, 0);
    
    if (lastTempDisplayed == "FOOD") {
      lcd.print(F("Food: "));
      if (food_temp == -67) {
        lcd.print("NP");
      } else {
        lcd.print(padTemp(food_temp));
      }
      lcd.setCursor(9, 0);
    } else if (lastTempDisplayed == "PIT") {
      lcd.print(F("Pit:  "));
      if (pit_temp == -67) {
        lcd.print("NP");
      } else {
        lcd.print(padTemp(pit_temp));
      }
      lcd.setCursor(9, 0);
    } else if (lastTempDisplayed == "AMBIENT") {
      lcd.print(F("Amb:  "));
      lcd.print(padTemp(ambient_temp));
      lcd.setCursor(9, 0);
    }
    
    if (cookState == 1 && (currentMillis - previousTempNetMillis > netTempUpdateInterval)) {   
      previousTempNetMillis = currentMillis;
      Serial.println(F("Updating webservice with temperatures"));
      
      String tempNetResponse = sendTempNetRequest(pit_temp, food_temp, ambient_temp, cookId);
      String tempId = findWebserviceReturnId(tempNetResponse);
      // If we get a cookID of 0 back, something went wrong, so we need to show that the cook has failed  
      if (tempId == "0") {
        // TODO - display something when a temperature update fails
      }
    }
  } 
  
}


// Network methods
// Includes WiFi network connectivity, HTTP request and page read methods
/* connectToNetwork
   Setup the SD part of the shield, and then connect to a wifi network
   
   - if we are able to connect to the network, we briefly display the SSID on the LCD then Net: OK
   - if we fail to connect to the network, we display Net: BAD on the LCD
*/
void connectToNetwork() { 
  
  Serial.println(F("Initializing Wifi..."));
  printMacAddress();
  
  Serial.print(F("Attempting to connect to WPA network: ["));
  Serial.print(ssid);
  Serial.println(F("]"));
  lcd.setCursor(0, 1);
  lcd.print(F("Connecting..."));

  wifiStatus = WiFi.begin(ssid, pass);
  
   // if you're not connected, stop here:
  if ( wifiStatus != WL_CONNECTED) {
    Serial.println(F("Couldn't get a wifi connection"));
    lcd.setCursor(0, 1);
    lcd.print(F("Net: BAD       "));
    while(true);
  }
  // if you are connected, print out info about the connection:
  else {
    Serial.println(F("Connected to network"));
    lcd.setCursor(0, 1);
    String ssidString = ssid;
    int padCount = 16 - ssidString.length();
    if (padCount > 0) {
      for (int i = 0; i < padCount; i++) {
        ssidString + ssidString + " ";        
      }
    }
    lcd.print(ssidString);
    delay(2000);  // Change this value to define how long the SSID is displayed on the LCD before we proceed 
    
    lcd.setCursor(0,1);   
    lcd.print(F("NET: OK          "));
  }
}

/* printMacAddress
   Finds the WiFi shield's mac address and prints it to the serial output. This is used for debugging
*/
void printMacAddress() {
  // the MAC address of your Wifi shield
  byte mac[6];                    

  // print your MAC address:
  WiFi.macAddress(mac);
  Serial.print(F("MAC: "));
  Serial.print(mac[5],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.println(mac[0],HEX);
}

/* readPage
   Read the response from the webservice and return the JSON component of the response.
   We assume that the JSON response starts at the first '{' and ends at the first '}'
   
   This was taken from http://bildr.org/2011/06/arduino-ethernet-client/
*/
String readPage() {
  // Set the string position to 0 and clear the inString memory
  boolean startRead = false;
  char inString[64];
  int stringPos = 0;
  memset(&inString, 0, 64 ); 
  
  while(true){
    if (client.available()) {
      char c = client.read();

      if (c == '{' ) { 
        startRead = true; //Ready to start reading the JSON part 
      } else if(startRead) {
        if(c != '}'){ //'}' is our ending character
          inString[stringPos] = c;
          stringPos ++;
        } else {
          //got what we need here! We can disconnect now
          startRead = false;
          client.stop();
          client.flush();
          Serial.println(F("disconnecting."));
          Serial.println(inString);
          Serial.println("Leaving to readPage ... ");
          return inString;
        }
      }
    }
  }
  Serial.println("Leaving to readPage ... ");
  return inString;
}

/* sendCookNetRequest
   Send a network request to the webservice to either start or end a cook.
    - the type of request (start cook or end cook) is defined by the method input
    - The content to send is sent by the calling method
*/
String sendCookNetRequest(String method, String content) {
  String requestString = "";
  
  if (method == "PUT") {
    requestString = "PUT /cookinfo/" + cookId + " HTTP/1.1";
  } else if (method == "POST") {
    requestString = "POST /cookinfo HTTP/1.1";
  }
  
  if (client.connect(serverAddress, serverPort)) {
    Serial.println("Sending request to Braai REST service: " + requestString);
    client.println(requestString);
    client.println("Host: " + webServiceIP);
    client.println(F("User-Agent: braaiduino"));
    client.println(F("Content-type: application/json"));
    client.println(F("Connection: close"));
    client.print(F("Content-Length: "));
    client.println(content.length());
    client.println();
    client.println(content);

    String netOutput = readPage();
    return netOutput;

  } else {
    // if you couldn't make a connection:
    Serial.println(F("connection failed"));
    Serial.println(F("disconnecting."));
    client.stop();
    return "NETFAILED";
  }
}

/* sendTempNetRequest
   Send a network request to the webservice to either start or end a cook.
   - temp1 = pit_temp
   - temp2 = food_temp
   - temp3 = ambient_temp
*/
String sendTempNetRequest(int pit_temp, int food_temp, int ambient_temp, String cookId) {
  if (pit_temp == -67) {
    pit_temp=0;
  }
  
  if (food_temp == -67) {
    food_temp = 0;
  }
  
  String content = "{\"action\":\"add\", \"cookid\": \"" + cookId+ "\", \"temp1\": \"" + pit_temp 
      + "\", \"temp2\": \"" + food_temp + "\", \"temp3\": \"" + ambient_temp + "\"}";

  String requestString = "POST /tempinfo HTTP/1.1";
  
  if (client.connect(serverAddress, serverPort)) {
    Serial.println("Sending request to Braai REST service: " + requestString);
    client.println(requestString);
    client.println("Host: " + webServiceIP);
    client.println(F("User-Agent: braaiduino"));
    client.println(F("Content-type: application/json"));
    client.println(F("Connection: close"));
    client.print(F("Content-Length: "));
    client.println(content.length());
    client.println();
    client.println(content);

    String netOutput = readPage();
    return netOutput;

  } else {
    // if you couldn't make a connection:
    Serial.println(F("connection failed"));
    Serial.println(F("disconnecting."));
    client.stop();
    return "NETFAILED";
  }

}

// Temperature related methods
// These include conversions and reading thermistor information
void CelsiusToFarenheit(float const tCel, float &tFar) {
	tFar = tCel * 1.8 + 32;
}

float getVoltage(int pin){
 return (analogRead(pin) * .004882814); //converting from a 0 to 1023 digital range
                                        // to 0 to 5 volts (each 1 reading equals ~ 5 millivolts
}

/* thermister_temp
   Get the temperature from a thermister pin
   Taken from http://hruska.us/tempmon/BBQ_Controller.pde
*/
int thermister_temp(int aval) {
  // Taken from http://hruska.us/tempmon/BBQ_Controller.pde
  double R, T;

  // These were calculated from the thermister data sheet
  //	A = 2.3067434E-4;
  //	B = 2.3696596E-4;
  //	C = 1.2636414E-7;
  //
  // This is the value of the other half of the voltage divider
  //	Rknown = 22200;
  
  // Do the log once so as not to do it 4 times in the equation
  //	R = log(((1024/(double)aval)-1)*(double)22200);
  R = log((1 / ((1024 / (double) aval) - 1)) * (double) 22200);
  
  //lcd.print("A="); lcd.print(aval); lcd.print(" R="); lcd.print(R);
  // Compute degrees C
  T = (1 / ((2.3067434E-4) + (2.3696596E-4) * R + (1.2636414E-7) * R * R * R)) - 273.25;
  // return degrees F
  // return ((int) ((T * 9.0) / 5.0 + 32.0));
  return int(T);

}

/* getTempToDisplay
   lastTempDisplayed is the last temperature that we displayed ("", "FOOD", "PIT" or "AMBIENT")
   Work out which the next temperature to display is 
*/
String getTempToDisplay(String lastTempDisplayed) {
  String nextTempToDisplay = "";
  
  if (lastTempDisplayed == "" || lastTempDisplayed == "AMBIENT") {
    nextTempToDisplay = "FOOD";
  } else if (lastTempDisplayed == "FOOD") {
    nextTempToDisplay = "PIT";
  } else if (lastTempDisplayed == "PIT") {
    nextTempToDisplay = "AMBIENT";
  }
  return nextTempToDisplay;
  
}
