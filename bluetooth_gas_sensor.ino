#include "BluetoothSerial.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
// Check if Bluetooth is available
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// Check Serial Port Profile
#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Port Profile for Bluetooth is not available or not enabled. It is only available for the ESP32 chip.
#endif
String device_name = "ESP32_Gas_Sensor";

// 핀 설정
const int lightSensorPin = 34; // 조도 센서 핀
const int mq135Pin = 35; // MQ-135 센서 핀
const int piezoPin = 26; // 피에조 스피커 핀

// 임계값 설정
const int lightThreshold = 10; // 조도 센서 임계값
const int gasThreshold = 500; // MQ-135 센서 임계값

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {

    }
};
// 여기까지
void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("ESP32_Gas_Sensor");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    ); 

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());


  const char *message = "Hello World says Neil";
  pCharacteristic->setValue((uint8_t *)message, strlen(message));
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("Waiting for a client connection to notify...");

}

void loop() {
  // Disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // Give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // Restart advertising
    Serial.println("Start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // Connecting
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
  const int gasValue = analogRead(mq135Pin);
  pCharacteristic->setValue((int&) gasValue);
  delay(500);
}