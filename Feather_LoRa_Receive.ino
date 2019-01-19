//Feather M0 w/ LoRa
//Heavily based off Adafruit example for Testing

#include <SPI.h>
#include <RH_RF95.h>

//Radio Setup for Feather M0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

//Operation frequency- must match for network as well as unit capabilities
#define RF95_FREQ 915.0

//Radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

//blinky for receipt of message- useful to headless range testing
#define LED 13

volatile uint32_t idNum;
char message[50];
String bleh;

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(115200);
  delay(100);
  Serial.println("Feather LoRa RX Test");

  //reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK");

  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  //Transmitter power
  rf95.setTxPower(23, false);

  //Get identity
  chipId();
}

void loop() {
  if (rf95.available()) {
    //If radio is operational
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (rf95.recv(buf, &len)) {
      //Message is available
      digitalWrite(LED, HIGH);
      RH_RF95::printBuffer("Received: ", buf, len);
      Serial.print("Got: "); Serial.println((char*)buf);
      Serial.print("RSSI: "); Serial.println(rf95.lastRssi(), DEC);

      //Split into constituent parts
      bleh = (char*)buf;
      String recvID = getValue(bleh, ',', 0);
      Serial.println(recvID);
      String datum = getValue(bleh, ',', 1);
      Serial.println(datum);
      
      //Is message for me?
      if (recvID == "ff04281c") {
        Serial.println("Is for me");

        //Decide what to do with message
        if (datum == "Acknowledged") {
          //Acknowledgement of ID
          Serial.println("Received acknowledgement, no reply");
          digitalWrite(LED, LOW);
        }
        else {
          Serial.println("Unhandled Exception");
        }
      }
      else if (recvID == "Request ID") {
        //Initial ID Request
        Serial.println("Request for ID received");
        sprintf(message, "%8x", idNum);
        rf95.send((uint8_t *) message, sizeof(message));
        rf95.waitPacketSent();
        Serial.println("Sent a reply");
        digitalWrite(LED, LOW);
      }
    }
  }
}

void chipId() {
  //Assembles the SAMD processor chip ID value
  volatile uint32_t val1, val2, val3, val4;
  volatile uint32_t *ptr1 = (volatile uint32_t *)0x0080A00C;
  val1 = *ptr1;
  volatile uint32_t *ptr = (volatile uint32_t *)0x0080A040;
  val2 = *ptr;
  ptr++;
  val3 = *ptr;
  ptr++;
  val4 = *ptr;
  
  //Prints the full chip ID if following block is uncommented; otherwise just outputs last values.
  /** Serial.print("chip id: 0x");
  char buf[33];
  sprintf(buf, "%8x%8x%8x%8x", val1, val2, val3, val4);
  Serial.println(buf); */
  idNum = val4;
}

String getValue(String data, char separator, int index)
{
  
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}