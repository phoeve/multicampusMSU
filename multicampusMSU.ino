
#include <SPI.h>
#include <Ethernet.h>

//  You can turn console messages off to improve performance
#define CONSOLE 1

struct route
{
  byte  button;
  byte  ultirxIn;
  byte  ultirxOut;
  byte  platinumIn;
  byte  platinumOut;
  byte  ultirxIp[4];
  unsigned int  ultirxPort;  
} Buttons[] = {
                      {  1,   0,   0,  35,  26, 0, 0 },   // NPCC
                      {  2,   0,   0,  36,  26, 0, 0 },
                      {  3,   0,   0,  37,  26, 0, 0 },
                      {  4,   0,   0,  38,  26, 0, 0 },
                      {  5,   0,   0,  39,  26, 0, 0 },
                      {  6,   0,   0,  43,  26, 0, 0 },
                      {  7,   0,   0,  44,  26, 0, 0 },
                      {  8,   0,   0,  45,  26, 0, 0 },
                      { 10,   0,   0,  121,  26, 0, 0 },

                      { 11,  33,  27, 112,  26, 172,17,3,161, 3811 },   // BC
                      { 12,  34,  27, 112,  26, 172,17,3,161, 3811 },
                      { 13,  35,  27, 112,  26, 172,17,3,161, 3811 },
                      { 14,  39,  27, 112,  26, 172,17,3,161, 3811 },
                      { 15,  40,  27, 112,  26, 172,17,3,161, 3811 },
                      { 20,  42,  27, 112,  26, 172,17,3,161, 3811 },
                      
                      {0,0,0,0,0,0,0,0,0,0}   // Table termintor
};

byte platinumIp[] = {172,26,3,161};
unsigned int platinumPort = 52116;

//byte platinumIp[] = {192,168,33,58};
//byte platinumPort = 22;


#define GPI_PIN_ON    HIGH             // If voltage is HIGH, the dip switch is in the ON position. 
#define GPI_LOW_PIN   2
#define GPI_NUM_PINS  7

// MAC address 
byte MyMac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

IPAddress MyIP(172, 26, 3, 183);


void setup() {
  // start the Ethernet connection:
  Ethernet.begin(MyMac, MyIP);

#if CONSOLE
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
#endif

  // give the Ethernet shield a second to initialize
  //delay(1000);

}


void loop() {

  static unsigned int last_button = -1;
  static boolean got_ack = false;
  unsigned int button = 0;
  
        //
        // Read GPI and interpret which button was pushed (Arduino pins 2-11)
        //
  button = 0;
  for (int pin=GPI_LOW_PIN; pin<(GPI_LOW_PIN +GPI_NUM_PINS) ; pin++)
  {
    unsigned char val=0;
    pinMode(pin, INPUT);           // set pin to input
    digitalWrite(pin, HIGH);       // turn on pullup resistor
    
    val = digitalRead(pin);
    //Serial.print(val);
    if (val==GPI_PIN_ON){
      bitSet(button, pin-2);  
    } 
  }
  button++;


//  Serial.print(" ==> ");

  if ((last_button != button)){      // Don't repeat if we received an ack on same request
    int row;

    Serial.print("---------------------------------\nbutton=");
    Serial.println(button);
    
    row = button2row(button);
//    Serial.print("row=");
//    Serial.print(row);
//    Serial.println("");

    if (row == -1){        // button number out of bounds.
      delay(300);
      return;
    }

    if (Buttons[row].ultirxIn){           // SWP-08 only if if row has info
      got_ack = sendSWP08Packet (Buttons[row].ultirxIp, Buttons[row].ultirxPort, Buttons[row].ultirxIn, Buttons[row].ultirxOut);
    }

    got_ack = sendLRCPacket (platinumIp, platinumPort, Buttons[row].platinumIn, Buttons[row].platinumOut);
  }
  
  last_button = button;     // Only send message once.
  
}

int button2row(byte button)
{
  int i;
  
  for (i=0; Buttons[i].button!=0; i++)
    if(button == Buttons[i].button)
      return i;

  return -1;
}

//  ~XPOINT:S${SAT 1};D${MON 6}\

#define LRC_OPEN_FLAG "~"
#define LRC_CLOSE_FLAG "\\"   // really just single slash - gotta escape for compiler.

boolean sendLRCPacket(IPAddress ip, unsigned int port, byte src, byte dest){

  static EthernetClient client;
  static boolean connected = false;
  char msg[128];

  sprintf(msg, "~XPOINT:D#{%d};S#{%d}\\", dest, src);

  char str[128];
  sprintf(str, "Sending ... %s ... To %d.%d.%d.%d:%u\n", msg, ip[0], ip[1], ip[2], ip[3], port);
  Serial.println(str);

  if (!connected){
    if (client.connect(ip, port)) {
      Serial.println("connected");
      connected = true;
    }
    else{
          // if you didn't get a connection to the server:
      Serial.println("connection failed");
      return false;
    }
  }
  
  Serial.print(msg);
  Serial.print(" - sent ... listening ...");

  client.println(msg);

//    for (int i=0; i<3; i++){
//      while (client.available()) {
//        char c = client.read();
//        Serial.print(c);
//      }
////      if (false)
////        break;
//      //delay(1000);       // wait 0.1 sec
//    }
  //Serial.println(" ...bailing");
  //client.stop();
  return true;
    
}

#define SWP08_OPEN_FLAG 0xFF
// fixed length and checksum terminated

boolean sendSWP08Packet(IPAddress ip, unsigned int port, byte src, byte dest){


  EthernetClient client;
  static IPAddress connectedIP(0,0,0,0);
  byte msg[128];

  msg[0] = 0x02;                        // STX Start of message
  msg[1] = 2;                           // Connect command
  msg[2] = src;                         // source router port nummber
  msg[3] = dest;                        // destination router port number
  msg[4] = 3;                           // byte count
  msg[5] = msg[1]+msg[2]+msg[3]+msg[4]; // checksum
  msg[6] = 0x03;                         // ETX

  Serial.print("Sending ... ");
  Serial.print("<STX>");
  for (int i=1; i<6; i++){
    Serial.print((int) msg[i]);
    Serial.print("-");
  }
  Serial.print(msg[5],BIN);
  Serial.print("<ETX>");
  Serial.print("... To ");
  Serial.print(ip);
  Serial.print(":");
  Serial.println(port);

                // If the ip is different from the previous, stop/connect
  if (!sameIP(connectedIP, ip)){
    if (connectedIP[0] != 0)     // Disconnect previous if there is one.
      client.stop();
    if (client.connect(ip, port)) {
      Serial.println("connected");
    }
  }
    
  client.write(msg, 7);

//    for (int i=0; i<10; i++){
//      while (client.available()) {
//        char c = client.read();
//        Serial.write(c);
//      }
//      if (false)
//        break;
//      //delay(100);       // wait 0.1 sec
//    }
  
  return true;
    
    // if you didn't get a connection to the server:
//  Serial.println("connection failed");
//  return false;

}

boolean sameIP(IPAddress ip1, IPAddress ip2)
{
  for (int i=0; i<4; i++)
    if (ip1[i] != ip2[i])
      return false;

  return true;
}

