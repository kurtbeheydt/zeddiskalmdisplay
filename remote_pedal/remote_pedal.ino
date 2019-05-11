#include <BLEDevice.h>
//#include "BLEScan.h"

static BLEUUID serviceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E"); 
static BLEUUID    charUUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");

BLEScan* pBLEScan;
static BLEAddress *pServerAddress;
static boolean doConnect = false;
static boolean clientConnected = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;

int scanTime = 10; //In seconds

const uint16_t statusLedPin = 2;
uint16_t buttonPins[] = {13, 12, 14, 27, 26, 25, 33, 21, 32};
char* buttonMessage[] = {"1#", "1m", "0A", "0B", "0C", "0D", "0E", "0F", "0G"};
uint16_t buttonStatus[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
uint16_t buttonCount = 9;
uint16_t messageId = 0;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  Serial.print("Notify callback for characteristic ");
  for (int i = 0; i < length; i++) {
    Serial.print(pData[i]);
    Serial.print(" ");
  }
  Serial.println();
}

class MyClientCallbacks: public BLEClientCallbacks {
    void onConnect(BLEClient* pClient) {
      clientConnected = true;
      digitalWrite(statusLedPin, HIGH);
      Serial.println("connected");
    };

    void onDisconnect(BLEClient* pClient) {
      clientConnected = false;
      digitalWrite(statusLedPin, LOW);
      Serial.println("disconnected");
    }
};

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());
    
    // We have found a device, let us now see if it contains the service we are looking for.
    // String My_BLE_Address = "cc:50:e3:8c:e0:a2";
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(serviceUUID)) {
      Serial.print("Found our device!  address: ");
      Serial.println(advertisedDevice.getAddress().toString().c_str());
      advertisedDevice.getScan()->stop();
      
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;
    }
  }
};

bool connectToServer(BLEAddress pAddress) {
  Serial.print("Forming a connection to ");
  Serial.println(pAddress.toString().c_str());

  BLEClient* pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallbacks());
  
  Serial.println(" - Created client");

  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    return false;
  }
  Serial.println(" - Found our service");


  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    return false;
  }
  Serial.println(" - Found our characteristic");
  pRemoteCharacteristic->writeValue("0CONN");
  pRemoteCharacteristic->writeValue("1ECTED!");
}

void sendText(char* message) {
  if (clientConnected) {
    Serial.printf("*** Sent Value: %d ***\n", messageId);
    pRemoteCharacteristic->writeValue(message);
    messageId++;
  } else {
    Serial.print("*** Value to be sent, but not connected: ");
    Serial.print(message);
    Serial.println(" ***");
  }
}

void checkButtonPush(const uint16_t buttonId) {
  uint8_t newStatus = !digitalRead(buttonPins[buttonId]);
  
  if (buttonStatus[buttonId] == 0) {
    if (newStatus == 1) {
      buttonStatus[buttonId] = 1;
      
      Serial.print("pressed button id: ");
      Serial.println(buttonId);

      digitalWrite(statusLedPin, !clientConnected);
      
      if (buttonId == 0) {
        sendText("0 ");
        sendText("1 ");
      } else {
        sendText(buttonMessage[buttonId]);
      }
    }
  } else {
    if (newStatus == 0) {
      buttonStatus[buttonId] = 0;
      Serial.print("released button id: ");
      Serial.println(buttonId);

      digitalWrite(statusLedPin, clientConnected);
    }  
  }
}
void setup() {
  pinMode (statusLedPin, OUTPUT);
  
  for (uint16_t i = 0; i < buttonCount; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  
  Serial.begin(115200);
  Serial.println("pedalBoard initiating ...");

  BLEDevice::init("PEDAL");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);

  Serial.println("pedalBoard started ...");
}

void loop() {
  if (doConnect == true) {
    if (connectToServer(*pServerAddress)) {
      Serial.println("SUCCESS: connected to display.");
    } else {
      Serial.println("ERROR: failed to connect to display.");
    }
    doConnect = false;
  }

  for (uint16_t i = 0; i < buttonCount; i++) {
    checkButtonPush(i);
  }

  if (!clientConnected) {
    Serial.println("Scanning...");
    pBLEScan->start(scanTime);
    Serial.println("Scan done!");
  }
}
