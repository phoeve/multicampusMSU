
#include <SPI.h>
#include <Ethernet.h>

//  You can turn console messages off to improve performance
#define CONSOLE 1

struct route
{
  byte            button;
  byte            ultirxIn;
  byte            ultirxOut;
  byte            platinumIn;
  byte            platinumOut;
  byte            ultirxIp[4];
  unsigned int    ultirxPort;  
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

                      { 11,  32,  26, 112,  26, 172,17,3,137, 12345 },   // BC 8910 or 12345
                      { 12,  33,  26, 112,  26, 172,17,3,137, 12345 },
                      { 13,  34,  26, 112,  26, 172,17,3,137, 12345 },
                      { 14,  38,  26, 112,  26, 172,17,3,137, 12345 },
                      { 15,  39,  26, 112,  26, 172,17,3,137, 12345 },
                      { 20,  62,  26, 112,  26, 172,17,3,137, 12345 },  // 78 +48 +84 +73 +9 +63 +9 +27 
                      
                      {0,0,0,0,0,0,0,0,0,0}   // Table termintor
};

byte platinumIp[] = {172,26,3,161};
unsigned int platinumPort = 52116;


#define GPI_PIN_ON    HIGH             // If voltage is HIGH, the dip switch is in the ON position. 
#define GPI_LOW_PIN   2
#define GPI_NUM_PINS  7

// MAC address 
byte MyMac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};



IPAddress MyIP(172, 16, 3, 183);
// the dns server ip
IPAddress dnsServer(10, 16, 16, 10);
// the router's gateway address:
IPAddress gateway(172, 16, 1, 254);
// the subnet:
IPAddress subnet(255, 255, 0, 0);

void setup() {
  // start the Ethernet connection:
  Ethernet.begin(MyMac, MyIP, dnsServer, gateway, subnet);


#if CONSOLE
  // Open serial communications and wait for port to open:
  Serial.begin(57600);

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

//button = 20;

//  Serial.print(" ==> ");

  if ((last_button != button)){      // Don't repeat if we received an ack on same request
    int row;

//    Serial.print("---------------------------------\nbutton=");
//    Serial.println(button);
    
    row = button2row(button);
//    Serial.print("row=");
//    Serial.print(row);
//    Serial.println("");

    if (row == -1){        // button number out of bounds.
      delay(300);
      return;
    }



    if (Buttons[row].ultirxIn){           // SWP-08 only if if row has info
      got_ack = send7000Packet (Buttons[row].ultirxIp, Buttons[row].ultirxPort, Buttons[row].ultirxIn, Buttons[row].ultirxOut);
      //got_ack = sendLRCPacket (Buttons[row].ultirxIp, Buttons[row].ultirxPort, Buttons[row].ultirxIn, Buttons[row].ultirxOut);

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
  int ret= -1;

  sprintf(msg, "~XPOINT:D#{%d};S#{%d}\\", dest, src);

  char str[128];
  sprintf(str, "Sending ... %s ... To %d.%d.%d.%d:%u", msg, ip[0], ip[1], ip[2], ip[3], port);
  Serial.println(str);
  
  if (!client.connected()){
    if ((ret=client.connect(ip, port))) {
      Serial.println(ret);
      Serial.println("... connected ...");
      connected = true;
    }
    else{
          // if you didn't get a connection to the server:
      Serial.println("... connection failed.");
      return false;
    }
  }


  client.println(msg);
  Serial.println(" ... sent.\n\n");
  //client.stop();

//    for (int i=0; i<3; i++){
//      while (client.available()) {
//        char c = client.read();
//        Serial.print(c);
//      }
//      if (false)
//        break;
//      delay(1000);       // wait 0.1 sec
//    }
//  Serial.println(" ...bailing");
//  client.stop();
  return true;
    
}


boolean send7000Packet(IPAddress ip, unsigned int port, byte src, byte dest){


  static EthernetClient client;
  static IPAddress connectedIP(0,0,0,0);
  byte msg[16];
  char str[128];
  unsigned int sum=0;
  unsigned int cksum;

  sprintf(msg, "N0TI\t%02x\t%02x", dest, src);
  //sprintf(msg, "N0QI\t%04x", dest);

  for (int i=0; i< strlen(msg); i++){
    sum += msg[i];

    sprintf(str, "%d/%x ", msg[i], msg[i]);
    Serial.print(str);
  }
  
  cksum = 0x100 - (sum % 0x100);

  sprintf(str, " = %d/%x, cksum = %d/%x", sum,sum, cksum,cksum);
  Serial.println(str);

  sprintf (&msg[strlen(msg)], "%02x", cksum);   // concat ascii of hex cksum 

  sprintf(str, "Sending ... %s ... To %d.%d.%d.%d:%u", msg, ip[0], ip[1], ip[2], ip[3], port);
  Serial.println(str);

  if (client.connect(ip, port)) {
    Serial.println("... connected ...");
    Serial.write(msg, strlen(msg));
    client.write(0x01);                       // SOH
    client.write(msg, strlen(msg));
    client.write(0x04);                       // EOT
    Serial.println(" ... sent.\n\n");
    
//    for (int i=0; i<3; i++){
//      while (client.available()) {
//        char c = client.read();
//        Serial.print(c);
//      }
//      delay(1000);       // wait 0.1 sec
//    }
//    Serial.println(" ...bailing on ACK.");
    
    client.stop();
  }
  else{
        // if you didn't get a connection to the server:
    Serial.println("... connection failed.");
    return false;
  }
  
  return true;
}



boolean sameIP(IPAddress ip1, IPAddress ip2)
{
  return false;
  
  Serial.println("sameIP comparing");
  Serial.print(ip1);
  Serial.print(ip2);
  Serial.println("");
  for (int i=0; i<4; i++)
    if (ip1[i] != ip2[i])
      return false;

  return true;
}

