
#include <SPI.h>
#include <Ethernet.h>


#define Northpoint 10
#define Buckhead 20
#define BrownsBridge 30
#define Woodstock 40
#define Gwinett 50

#define CONSOLE 1

#define GPI_PIN_ON LOW             // If voltage is LOW (grounded), the dip switch is in the ON position. 

#define GPI_LOW_PIN 2
#define GPI_NUM_PINS 10

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 33, 166);

// Enter the IP address of the server you're connecting to:
IPAddress Peter(192, 168, 33, 58);

IPAddress NorthPointIP (172, 26, 3, 161);
IPAddress BrownsBridgeIP (0, 0, 0, 0);
IPAddress BuckheadIP (172, 17, 3, 161);
IPAddress WoodstockIP (192, 168, 33, 58);
IPAddress GwinettIP (192, 168, 33, 58);

EthernetClient client;

unsigned int gpi_value = 0;

void setup() {
  // start the Ethernet connection:
  Ethernet.begin(mac, ip);

#if CONSOLE
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
#endif


  // give the Ethernet shield a second to initialize:
  delay(1000);

}

void loop() {

  
  int i;
  unsigned char val=0;

  
  Serial.println("connecting...");

  // if you get a connection, report back via serial:
  if (client.connect(Peter, 22)) {
    Serial.println("connected");
  } else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
  }

  //
  // Read GPI and interpret which campus / camera (Arduino pins 2-11)


  
  for (i=GPI_LOW_PIN; i<(GPI_LOW_PIN +GPI_NUM_PINS) ; i++)
  {
    pinMode(i, INPUT);           // set pin to input
    digitalWrite(i, HIGH);       // turn on pullup resistor
    
    val = digitalRead(i);
    Serial.print(val);
    if (val==GPI_PIN_ON)
      bitSet(gpi_value, GPI_NUM_PINS -i -1);   
  }

  switch (gpi_value/10){
    case Northpoint:
      break;
    case Buckhead:
      break;
    case BrownsBridge:
      break;
    case Woodstock:
      break;
    case Gwinett:
      break;
    default:
      ret = client.connect(Peter, 22);
      break;
      
  }
  
  // if there are incoming bytes available
  // from the server, read them and print them:
  while (client.available()) {
    char c = client.read();
    Serial.print(c);
  }


  Serial.println();
  Serial.println("disconnecting.");
  client.stop();

}

