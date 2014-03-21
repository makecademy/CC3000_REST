/* 
  This a simple sketch that implements a REST API for Arduino (Uno/Mega/Due/Teensy)
  and the CC3000 WiFi chip. See the README file for more details.
 
  Written in 2014 by Marco Schwartz under a GPL license. 
*/

// Import required libraries
#include <Adafruit_CC3000.h>
#include <SPI.h>
#include <CC3000_MDNS.h>

// These are the pins for the CC3000 chip if you are using a breakout board
#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10

// Create CC3000 instance
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIV2);

// Your WiFi SSID and password                                         
#define WLAN_SSID       "yourSSID"
#define WLAN_PASS       "yourPassword"
#define WLAN_SECURITY   WLAN_SEC_WPA2

// The port to listen for incoming TCP connections 
#define LISTEN_PORT           80

// Server instance
Adafruit_CC3000_Server restServer(LISTEN_PORT);

// DNS responder instance
MDNSResponder mdns;

// Variables used for the REST API
String answer;
String command = "";
int pin;
int value;
boolean pin_selected = false;
boolean state_selected = false;
boolean command_selected = false;

void setup(void)
{  
  Serial.begin(115200);
  
  // Set up CC3000 and get connected to the wireless network.
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
  while (!cc3000.checkDHCP())
  {
    delay(100);
  }
  Serial.println();
  
  // Start multicast DNS responder
  if (!mdns.begin("arduino", cc3000)) {
    while(1); 
  }
   
  // Start server
  restServer.begin();
  Serial.println(F("Listening for connections..."));
}

void loop() {
  
  // Handle any multicast DNS requests
  mdns.update();
  
  // Try to get a client which is connected.
  Adafruit_CC3000_ClientRef client = restServer.available();
  if (client) {
    // Check if there is data available to read
    while (client.available()) {
       
    // Get the server answer
    char c = client.read();
    answer = answer + c;
    Serial.write(c);
        
    // Check if we are receveing useful data and process it
    if (c == '/' & state_selected == false) {
      
      // If the command is mode, and the pin is already selected    
      if (command == "mode" && pin_selected == true && state_selected == false) {
    
        // Trim answer      
        answer.trim();
       
        // Input command received ?     
        if (answer.startsWith("i")) {
         
          // Set pin to Input     
          pinMode(pin,INPUT);
              
          // Send feedback to client
          client.print(F("Setting pin D"));
          client.print(pin);
          client.print(F(" to "));
          client.println("input");
       }
       
       // Output command received ?     
       if (answer.startsWith("o")) {
         
         // Set to Output  
         pinMode(pin,OUTPUT);
              
         // Send feedback to client
         client.print(F("Setting pin D"));
         client.print(pin);
         client.print(F(" to "));
         client.println("output");
       }
       
       // Indicate that the state has been selected     
       state_selected = true;
            
     }
     
     // If a digital command has been received, process the data accordingly     
     if (command == "digital" && pin_selected == true && state_selected == false) {
       
       // Trim answer     
       answer.trim();
                
       // If it's a read command, read from the pin and send data back
       if (answer.startsWith("r")) {
         
         // Read from pin
         value = digitalRead(pin);
          
         // Send feedback to client
         client.print(F("Reading from pin D"));
         client.print(pin);
         client.print(F(" is at "));
         client.println(value);
       }
               
       else {
         
         // Get value we want to apply to the pin        
         value = answer.toInt();
         Serial.println("State " + answer + " set");
         
         // Apply on the pin      
         digitalWrite(pin,value);
 
         // Send feedback to client
         client.print(F("Pin D"));
         client.print(pin);
         client.print(F(" set to "));
         client.println(value);
       }
       
       // Declare that the state has been selected         
       state_selected = true;
               
     }
     
     // If analog command has been selected, process the data accordingly     
     if (command == "analog" && pin_selected == true && state_selected == false) {
                
       // Trim answer
       answer.trim();
                
       // If it's a read, read from the correct pin
       if (answer.startsWith("r")) {
         
         // Read analog value
         value = analogRead(pin);
          
         // Send feedback to client
         client.print(F("Analog read from pin A"));
         client.print(pin);
         client.print(F(" is at "));
         client.println(value);
       }
       
       // Else, write analog value        
       else {
         
         // Get value to apply to the output
         value = answer.toInt();
         Serial.println("Value " + answer + " set");
               
         // Write output value
         analogWrite(pin,value);
 
         // Send feedback to client
         client.print(F("Pin D"));
         client.print(pin);
         client.print(F(" set to analog value "));
         client.println(value);
       }
               
       state_selected = true;
               
     }
     
     // If the command is already selected, get the pin     
     if (command_selected == true && pin_selected == false) {
       pin = answer.toInt();
       Serial.println("Pin " + String(pin) + " selected");
       pin_selected = true;
     }
     
     // Digital command received ?    
     if (answer.startsWith("digital")) {
       Serial.println("Digital command received");
       command = "digital";
       command_selected = true;
     }
          
     // Mode command received ?
     if (answer.startsWith("mode")) {
       Serial.println("Mode command received");
       command = "mode";
       command_selected = true;
     }
          
     // Analog command received ?
     if (answer.startsWith("analog")) {
       Serial.println("Analog command received");
       command = "analog";
       command_selected = true;
     }
          
     answer = "";
     
     }
       
   }
     
    // Give the web browser time to receive the data
    delay(5);
    
    // Close the connection:
    client.close();
   
    // Reset variables for the next command
    answer = "";
    command = "";
    command_selected = false;
    pin_selected = false;
    state_selected = false;
    Serial.println("client disconnected");
    
  }
 
}
