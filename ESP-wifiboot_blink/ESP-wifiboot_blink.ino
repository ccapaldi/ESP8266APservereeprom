// This code is for an ESP8266 Nodemcu module and has been tested on it. 
// The code will read the eeprom to see if a wifi host has been written to it (ssid).
// If not then it will run the web server code and respond to the AP username and password.
// To access http://192.168.0.4/ will dump a list of wifi networks that it finds and provide 
// a form that lets the user enter one along with the password. 
// if you reset the device it will branch into the main loop and disable the web server. 
// That branch is commented out so it will always boot up the webserver until you uncomment it below. 
//
// Written by C.Capaldi as a way to test read/write eeprom from web server. 


#include "ESP8266WiFi.h"
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <EEPROM.h>

MDNSResponder mdns;
WiFiServer server(80);


// Access point name and password
const char* ssid = "ESPTesting";
const char* pass = "password";


int loopCount = 0; // variable to increment each time through the loop
int serverTrigger = 10;

// global string to hold the html for the list of web servers
String st;

// Variables the define the location of data in the eeprom
// example 0 through 31 for the ssid and position 32 to 64 for the password. 
// This will make it easy to add other permanent things like the mqtt server in the future
int PromSize = 512; // the size of the eeprom that we will initialize. 

// wifi host name
int SSIDstart = 0;
int SSIDsize = 32; // 0-31 for first storage location

// wifi password
int PASSstart=32;
int PASSsize = 64; // 32-95 for the second location





// end of eeprom setup code

// Set up the built in LED to blink
int LEDPin = 16;



void setup() {
  Serial.begin(115200);

  // Do the eeprom setup here
  EEPROM.begin(PromSize);
 
 

  
  delay(10);
  Serial.println();
  Serial.println();
  // Set the control pin for output
  pinMode(LEDPin, OUTPUT);
  //
  Serial.println("Startup");
  
  // read eeprom for ssid and pass
  // upon initial run there shouldn't be anything written in the prom
  Serial.println("Reading EEPROM ssid");

  // I have assigned position 0-31 (32 Bytes) to hold the wifi network name or ssid
  String esid = readProm(SSIDstart, SSIDsize);
 
  Serial.print("SSID: ");
  Serial.println(esid);


  // read the prom for the password
  Serial.println("Reading EEPROM pass");

  // I have set position 32-96 (64 Bytes) to hold the password. 
  
  String epass = readProm(PASSstart, PASSsize);
 
  Serial.println(epass);  

  // This is the test that determines if the ssid was initially blank
  if ( esid.length() > 1 ) {
      // test esid 
      WiFi.begin(esid.c_str(), epass.c_str());
      if ( testWifi() == 20 ) { 
        // we have an ssid stored so we just bypass the web server and
        // branch into the main loop here by commenting out the line below.
        // if commented it will just keep rebooting into AP mode with a webserver
        
         // launchWeb(0);
          return;
      }
  }
  setupAP(); 
}

int testWifi(void) {
  int c = 0;
  Serial.println("connected value");
  Serial.println(WL_CONNECTED);
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) { return(20); } 
    delay(1000);
    Serial.print(WiFi.status());    
    c++;
  }
  Serial.println("Connect timed out, opening AP");
  return(10);
} 

void launchWeb(int webtype) {
          Serial.println("");
          Serial.println("WiFi connected");
          Serial.println(WiFi.localIP());
          Serial.println(WiFi.softAPIP());
         
          Serial.println("mDNS responder started");
          // Start the server
          server.begin();
          Serial.println("Server started");   
          int b = 20;
          int c = 0;
          while(b == 20) { 
             b = mdns1(webtype);
           }
}

void setupAP() {
  
  //WiFi.mode(WIFI_STA);
 WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
     {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      delay(10);
     }
  }
  Serial.println(""); 
  st = "<ul>";
  for (int i = 0; i < n; ++i)
    {
      // Create the HTMP for each WiFi network that it finds
      st += "<li>";
      st +=i + 1;
      st += ": ";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
      st += "</li>";
    }
  st += "</ul>";
  delay(100);
  WiFi.softAP(ssid,pass);
  Serial.println("Setting up Access Point");
  Serial.println("");
  launchWeb(1);
  Serial.println("over");
}

// start of webserver code
int mdns1(int webtype)
{
  // Check for any mDNS queries and send responses
  mdns.update();
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return(20);
  }
  Serial.println("");
  Serial.println("New client");

  // Wait for data from client to become available
  while(client.connected() && !client.available()){
    delay(1);
   }
  
  // Read the first line of HTTP request
  String req = client.readStringUntil('\r');
  
  // First line of HTTP request looks like "GET /path HTTP/1.1"
  // Retrieve the "/path" part by finding the spaces
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
    Serial.print("Invalid request: ");
    Serial.println(req);
    return(20);
   }
  req = req.substring(addr_start + 1, addr_end);
  Serial.print("Request: ");
  Serial.println(req);
  client.flush(); 
  String s;
  if ( webtype == 1 ) {
      if (req == "/")
      {
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>AP webserver test device ";
        s += ipStr;
        s += "<p>";
        s += st;
        s += "<form method='get' action='a'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");
      }
      else if ( req.startsWith("/a?ssid=") ) {
        // /a?ssid=blahhhh&pass=poooo
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
        String qsid; 
        qsid = req.substring(8,req.indexOf('&'));
        Serial.println(qsid);
        Serial.println("");
        String qpass;
        String cleanPass;
        qpass = req.substring(req.lastIndexOf('=')+1);
        Serial.println(qpass);
        Serial.println("");
        
        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
          {
            EEPROM.write(i, qsid[i]);
            Serial.print("Wrote: ");
            Serial.println(qsid[i]); 
          }
        Serial.println("Cleaning password");
        // sub routine to parse string and clean out ascii values
        qpass = getCleanPass(qpass);
        Serial.println("writing eeprom pass:"); 
        for (int i = 0; i < qpass.length(); ++i)
          {
            EEPROM.write(32+i, qpass[i]);
            Serial.print("Wrote: ");
            Serial.println(qpass[i]); 
          }    
        EEPROM.commit();
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 ";
        s += "Found ";
        s += req;
        s += "<p> saved to eeprom... reset to boot into new wifi</html>\r\n\r\n";
      }
      else
      {
        s = "HTTP/1.1 404 Not Found\r\n\r\n";
        Serial.println("Sending 404");
      }
  } 
  else
  {
      if (req == "/")
      {
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266";
        s += "<p>";
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");
      }
      else if ( req.startsWith("/cleareeprom") ) {
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266";
        s += "<p>Clearing the EEPROM<p>";
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");  
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
        EEPROM.commit();
      }
      else
      {
        s = "HTTP/1.1 404 Not Found\r\n\r\n";
        Serial.println("Sending 404");
      }       
  }
  client.print(s);
  Serial.println("Done with client");
  return(20);
}


void loop() {
  // put your main code here, to run repeatedly
  // blink the LED to let us know you are alive
  Serial.write("LED off\n");
  digitalWrite(LEDPin, HIGH);
  
  delay(1000);
  Serial.write("LED on\n");
  digitalWrite(LEDPin, LOW);
  
  delay(1000);
  Serial.println(loopCount);
  Serial.println(serverTrigger);
  
  if( loopCount >= serverTrigger ) {
    setupAP();
    loopCount=0;
  }
 loopCount++;
  
  
}


// read prom function  st= start byte, tb=total bytes returns a string witht he result of the read
String readProm(int st, int tb){
  
  Serial.println("reading prom");
  Serial.print("Starting at ");
  Serial.println(st);
  Serial.print("Total bytes ");
  Serial.println(tb);
  
  String Result;
  for (int i = st; i < tb; ++i)
    {
      Result += char(EEPROM.read(i));
    }

  return Result;
}



// password cleaning routine
// if you use characters other than alpha numeric, you must substitute 
// the character from the html string
String getCleanPass(String qp) {
  String message = "Cleaning " + qp;
  Serial.println(message);
  // replace %23 or %40 with appropriate characters 
  qp.replace("%23","#");
  qp.replace("%40","@");
  message = "Cleaned " + qp;
  Serial.println(message);
  return(qp);
}
