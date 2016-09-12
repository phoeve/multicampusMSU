
#include <SPI.h>
#include <Ethernet.h>

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
                      {  1,   0,   0,  33,  26, 0, 0 },   // NPCC
                      {  2,   0,   0,  34,  26, 0, 0 },
                      {  3,   0,   0,  35,  26, 0, 0 },
                      {  4,   0,   0,  39,  26, 0, 0 },
                      {  5,   0,   0,  40,  26, 0, 0 },
                      {  6,   0,   0,  43,  26, 0, 0 },
                      {  7,   0,   0,  44,  26, 0, 0 },
                      {  8,   0,   0,  45,  26, 0, 0 },
                      { 10,   0,   0,  42,  26, 0, 0 },

                      { 11,  33,  27, 112,  26, 172,17,3,161, 3811 },   // BC
                      { 12,  34,  27, 112,  26, 172,17,3,161, 3811 },
                      { 13,  35,  27, 112,  26, 172,17,3,161, 3811 },
                      { 14,  39,  27, 112,  26, 172,17,3,161, 3811 },
                      { 15,  40,  27, 112,  26, 172,17,3,161, 3811 },
                      { 20,  42,  27, 112,  26, 172,17,3,161, 3811 },
                      {0,0,0,0,0,0,0,0,0,0}


};

byte platinumIp[] = {172,26,3,161};
unsigned int platinumPort = 52116;

//byte platinumIp[] = {192,168,33,58};
//byte platinumPort = 22;




#define GPI_PIN_ON    LOW             // If voltage is LOW (grounded), the dip switch is in the ON position. 
#define GPI_LOW_PIN   2
#define GPI_NUM_PINS  7

// MAC address 
byte MyMac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

IPAddress MyIP(192, 168, 33, 166);


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

  static unsigned int last_button = 0;
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
    Serial.print(val);
    if (val==GPI_PIN_ON){
      bitSet(button, pin-2);  
    } 
  }

  Serial.print("-------------------------\nbutton=");
  Serial.print(button);
  Serial.println("");

  if ((last_button != button) || !got_ack){      // Don't repeat if we received an ack on same request
    int row;
    
    row = button2row(button);
    Serial.print("row=");
    Serial.print(row);
    Serial.println("");

    if (row == -1)
      return;

    if (Buttons[row].ultirxIn){           // SWP-08 only if if row has info
      got_ack = sendSWP08Packet (Buttons[row].ultirxIp, Buttons[row].ultirxPort, Buttons[row].ultirxIn, Buttons[row].ultirxOut);
    }

    got_ack = sendLRCPacket (platinumIp, platinumPort, Buttons[row].platinumIn, Buttons[row].platinumOut);
  }
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

  EthernetClient client;
  char msg[128];

  sprintf(msg, "~XPOINT:S${%d};D${%d}\\", src, dest);
  Serial.print("Sending ...");
  Serial.print(msg);
  Serial.print("  ... To ");
  Serial.print(ip);
  Serial.print(":");
  Serial.println(port);
  
  if (client.connect(ip, port)) {
    Serial.println("connected");
    
    client.println("~XPOINT:S${SAT 1};D${MON 6}\\");

    for (int i=0; i<10; i++){
      while (client.available()) {
        char c = client.read();
        Serial.print(c);
      }
      if (false)
        break;
      //delay(100);       // wait 0.1 sec
    }
    client.stop();
    return true;
    
  }
    // if you didn't get a connection to the server:
  Serial.println("connection failed");
  return false;
}

#define SWP08_OPEN_FLAG 0xFF
// fixed length and checksum terminated

boolean sendSWP08Packet(IPAddress ip, unsigned int port, byte src, byte dest){


  EthernetClient client;
  byte msg[128];

  msg[0] = 0x02;                        // STX Start of message
  msg[1] = 2;                           // Connect command
  msg[2] = src;                         // source router port nummber
  msg[3] = dest;                        // destination router port number
  msg[4] = 3;                           // byte count
  msg[5] = msg[1] +msg[2] +msg[3] +msg[4];      // checksum
  msg[6] = 0x03;                         // ETX
  
  Serial.print("Sending ...");
  Serial.print("<STX>");
  for (int i=1; i<6; i++){
    Serial.print((int) msg[i]);
    Serial.print("-");
  }
  Serial.print("<ETX>");
  Serial.print("  ... To ");
  Serial.print(ip);
  Serial.print(":");
  Serial.println(port);
  
  if (client.connect(ip, port)) {
    Serial.println("connected");
    
    client.write(msg, 7);

    for (int i=0; i<10; i++){
      while (client.available()) {
        char c = client.read();
        Serial.write(c);
      }
      if (false)
        break;
      //delay(100);       // wait 0.1 sec
    }
    client.stop();
    return true;
    
  }
    // if you didn't get a connection to the server:
  Serial.println("connection failed");
  return false;

}


