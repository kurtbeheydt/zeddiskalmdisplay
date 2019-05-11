#include <MD_MAX72xx.h>
#include <SPI.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLEAdvertisedDevice.h>

#define statusLedPin 2

/*
 * display shizzle
 */

#define BUF_SIZE      75  // text buffer size
#define CHAR_SPACING  1   // pixels between characters

// Define the number of devices we have in the chain and the hardware interface
#define USE_GENERIC_HW 1
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define CLK_PIN   21//13
#define DATA_PIN  5//11
#define CS_PIN1   18//10
#define CS_PIN2   19//9

struct LineDefinition {
  MD_MAX72XX  mx;                 // object definition
  char    message[BUF_SIZE];      // message for this display
  boolean newMessageAvailable;    // true if new message arrived
};

struct LineDefinition Line[] =
{
  { MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN1, MAX_DEVICES), " F*CK", true },
  { MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN2, MAX_DEVICES), "CNCPT", true }
};

#define MAX_LINES   (sizeof(Line) / sizeof(LineDefinition))

/*
 * BLE shizzle
 */

#define DEVICE_NAME "KAREN"

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
BLECharacteristic *pRxCharacteristic;
BLEAdvertising *pAdvertising;

//BLEDescriptor *pDescriptor;
bool deviceConnected = false;
bool deviceNotifying = true;
uint8_t messageId = 0;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

void readBle(std::string rxValue) {
  static int8_t putIndex = -1;
  static uint8_t putLine = 0;
  char c;
  bool lineFinished = false;

  for (int i = 0; i < rxValue.length(); i++) {
    Serial.print(rxValue[i]);
    c = (char)rxValue[i];
    if (putIndex == -1) {  // first character should be the line number
      if ((c >= '0') && (c < '0' + MAX_LINES)) {
        putLine = c - '0';
        putIndex = 0;
      }
    } else if ((c == '\n') || (putIndex >= BUF_SIZE-3)) { // end of message character or full buffer
      Serial.print("-end");
      lineFinished = true;
      // put in a message separator and end the string
      Line[putLine].message[putIndex] = '\0';
      // restart the index for next filling spree and flag we have a message waiting
      putIndex = -1;
      Line[putLine].newMessageAvailable = true;
    } else {
      // Just save the next char in next location
      Line[putLine].message[putIndex++] = c;
    }
  }
  if (!lineFinished) {
    lineFinished = true;
    Line[putLine].message[putIndex] = '\0';
    putIndex = -1;
    Line[putLine].newMessageAvailable = true;
  }
  
  Serial.println();
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      digitalWrite(statusLedPin, HIGH);
      Serial.println("connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      digitalWrite(statusLedPin, LOW);
      Serial.println("disconnected");
    }
};

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }
        
        Serial.println();
        Serial.println("*********");
        readBle(rxValue);
      }
    }
};

bool initBLE() {
  BLEDevice::init(DEVICE_NAME);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX, 
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pTxCharacteristic->addDescriptor(new BLE2902());

  pRxCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pRxCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
  //pDescriptor->setCallbacks(new MyDescriptorCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(BLEUUID(SERVICE_UUID));
  BLEAdvertisementData oScanResponseData = BLEAdvertisementData();
  pAdvertising->setScanResponseData(oScanResponseData);
  pAdvertising->start();

  return true;
}

void printText(uint8_t lineID, uint8_t modStart, uint8_t modEnd, char *pMsg) {
  uint8_t   state = 0;
  uint8_t   curLen;
  uint16_t  showLen;
  uint8_t   cBuf[8];
  int16_t   col = ((modEnd + 1) * COL_SIZE) - 1;

  Line[lineID].mx.control(modStart, modEnd, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  do {
    switch(state) {
      case 0: // Load the next character from the font table
        // if we reached end of message, reset the message pointer
        if (*pMsg == '\0') {
          showLen = col - (modEnd * COL_SIZE);  // padding characters
          state = 2;
          break;
        }

        // retrieve the next character form the font file
        showLen = Line[lineID].mx.getChar(*pMsg++, sizeof(cBuf)/sizeof(cBuf[0]), cBuf);
        curLen = 0;
        state++;
        // !! deliberately fall through to next state to start displaying

      case 1: // display the next part of the character
        Line[lineID].mx.setColumn(col--, cBuf[curLen++]);

        // done with font character, now display the space between chars
        if (curLen == showLen) {
          showLen = CHAR_SPACING;
          state = 2;
        }
        break;

      case 2: // initialize state for displaying empty columns
        curLen = 0;
        state++;
        // fall through

      case 3: // display inter-character spacing or end of message padding (blank columns)
        Line[lineID].mx.setColumn(col--, 0);
        curLen++;
        if (curLen == showLen)
          state = 0;
        break;

      default:
        col = -1;   // this definitely ends the do loop
    }
  } while (col >= (modStart * COL_SIZE));

  Line[lineID].mx.control(modStart, modEnd, MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

void setup() {
  pinMode(statusLedPin, OUTPUT);
  digitalWrite(statusLedPin, LOW);
  
  Serial.begin(9600);
  Serial.print("\n[MD_MAX72XX ");
  Serial.print(MAX_LINES);
  Serial.print(" Line Message Display]\n");

  for (uint8_t i=0; i<MAX_LINES; i++) {
    Line[i].mx.begin();
  }

  if (!initBLE()) {
    Serial.println("Bluetooth init failed");
  }

  Serial.println("The device started, now you can pair it with bluetooth!");
}

void loop() {
  for (uint8_t i = 0; i < MAX_LINES; i++) {
    if (Line[i].newMessageAvailable) {
      printText(i, 0, MAX_DEVICES - 1, Line[i].message);
      Line[i].newMessageAvailable = false;
    }
  }
}
