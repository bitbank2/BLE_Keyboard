//
// Connect to a @arturo182 BBQ10 keyboard and transmit
// the keys as a BLE HID keyboard device
//  
#include <ArduinoBLE.h>
#include <Wire.h>
// I2C address of the keyboard controller
#define KEYB_ADDR 0x1f
volatile static int bDisconnected = 0;

typedef struct keyboard_report_t {
    uint8_t report_id;
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t key_codes[6];
} keyboard_report_t;

keyboard_report_t kbd_report;

uint8_t report_descriptor[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x01,                    //   REPORT_ID (1)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0x01,                    //   USAGE_MINIMUM
    0x29, 0x7f,                    //   USAGE_MAXIMUM
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x01,                    //   INPUT (Cnst,Ary,Abs)
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x03,                    //   REPORT_SIZE (3)
    0x91, 0x01,                    //   OUTPUT (Cnst,Ary,Abs)
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0                           // END_COLLECTION
};

uint8_t pnpID[] = {0x02, 0x8a, 0x24, 0x66, 0x82,0x34,0x36};

BLEService hidService("00001812-0000-1000-8000-00805f9b34fb"); // BLE HID Service
BLEService deviceService("0000180a-0000-1000-8000-00805f9b34fb"); // BLE device information

BLEService battService("0000180f-0000-1000-8000-00805f9b34fb"); // BLE HID Battery Service

// BLE Device Information characteristic
BLECharacteristic pnpCharacteristic("00002a50-0000-1000-8000-00805f9b34fb", BLERead, sizeof(pnpID), true);

// BLE HID battery level characteristic
BLEByteCharacteristic hidBatteryLevelCharacteristic("00002a19-0000-1000-8000-00805f9b34fb", BLERead);

// BLE HID protocol mode characteristic
BLEByteCharacteristic hidProtocolModeCharacteristic("00002a4e-0000-1000-8000-00805f9b34fb", BLERead | BLEWriteWithoutResponse);

// BLE HID control point characteristic
BLEByteCharacteristic hidControlPointCharacteristic("00002a4c-0000-1000-8000-00805f9b34fb", BLEWriteWithoutResponse);

// BLE HID Report characteristic
BLECharacteristic hidReportCharacteristic((const char *)"00002a4d-0000-1000-8000-00805f9b34fb", BLERead | BLENotify, sizeof(kbd_report), true);

// BLE HID Report Map characteristic
BLECharacteristic hidReportMapCharacteristic((const char *)"00002a4b-0000-1000-8000-00805f9b34fb", BLERead, sizeof(report_descriptor), true);

int I2CReadRegister(uint8_t iAddr, uint8_t u8Register, uint8_t *pData, int iLen)
{
int i = 0;
    Wire.beginTransmission(iAddr);
    Wire.write(u8Register);
    Wire.endTransmission();
    Wire.requestFrom(iAddr, (uint8_t)iLen);
    while (i < iLen)
    {
        pData[i++] = Wire.read();
    }
    return (i > 0);
} /* I2CReadRegister() */
//
// Write I2C data
//
int I2CWrite(uint8_t iAddr, uint8_t *pData, int iLen)
{
int rc = 0;

  Wire.beginTransmission(iAddr);
  Wire.write(pData, (uint8_t)iLen);
  rc = !Wire.endTransmission();
    return rc;
} /* I2CWrite() */

//
// Check for keys waiting in the keyboard's internal queue
// and read 1 if available and it was pressed (not released)
//
unsigned char GetKey(void)
{
unsigned char c, ucTemp[4];

  c = 0; // assume no key
  I2CReadRegister(KEYB_ADDR, 0x4, ucTemp, 1); // read key count/status
  if ((ucTemp[0] & 0x3f)) // keys waiting
  {
    I2CReadRegister(KEYB_ADDR, 0x9, ucTemp, 2); // read key and state
    if (ucTemp[0] == 1) // pressed
      c = ucTemp[1];
  }
  return c;
} /* GetKey() */

//
// Set the keyboard backlight level
// to one of 4 brightness levels (0-3)
//
void SetBacklight(uint8_t ucBrightness)
{
uint8_t ucTemp[2];
const uint8_t ucLevel[] = {0,64,128,255}; // off,low,med,high
    
  ucTemp[0] = 0x85; // backlight
  ucTemp[1] = ucLevel[ucBrightness];
  I2CWrite(KEYB_ADDR, ucTemp, 2);

} /* SetBacklight() */

void InitKeyboard(void)
{
uint8_t ucTemp[2];

  ucTemp[0] = 0x88; // RESET
  I2CWrite(KEYB_ADDR, ucTemp, 1);
  delay(100);
  SetBacklight(2);
} /* InitKeyboard() */

void SendKeyReport(unsigned char ucKey)
{
  kbd_report.report_id = 1;
  if (ucKey != 0)
     ucKey -= 93; // HID A = 4 DEBUG
  kbd_report.key_codes[0] = ucKey;
  hidReportCharacteristic.writeValue((uint8_t *)&kbd_report, sizeof(kbd_report));  
} /* SendKeyReport() */

void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
}

void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
  bDisconnected = 1;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  
  Wire.begin();
  Wire.setClock(400000L);
  Serial.println("BLE HID Keyboard");
  InitKeyboard();
  Serial.println("BBQ10 keyboard initialized");
  // initialize the BLE hardware
  BLE.begin();
  Serial.println("BLE hardware initialized");
  // set advertised name, appearance and service UUID:
  BLE.setDeviceName("Nano_BBQ10");
  BLE.setAppearance(961); // BLE_APPEARANCE_HID_KEYBOARD
  BLE.setAdvertisedService(hidService);
  BLE.setConnectable(true);
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // add the characteristics to the services
  hidService.addCharacteristic(hidReportCharacteristic);
  hidService.addCharacteristic(hidReportMapCharacteristic);
  hidService.addCharacteristic(hidProtocolModeCharacteristic);
  hidService.addCharacteristic(hidControlPointCharacteristic);
  battService.addCharacteristic(hidBatteryLevelCharacteristic);
  deviceService.addCharacteristic(pnpCharacteristic);
  
  hidReportMapCharacteristic.writeValue((uint8_t *)report_descriptor, sizeof(report_descriptor));
  hidBatteryLevelCharacteristic.writeValue((uint8_t)100); // set to 100%
  hidProtocolModeCharacteristic.writeValue((uint8_t)1); // set to default value
  pnpCharacteristic.writeValue(pnpID, sizeof(pnpID));
  memset(&kbd_report, 0, sizeof(kbd_report));
  Serial.println("Writing empty key report");
  SendKeyReport(0);
  // Add the services we need
  BLE.addService(hidService);
  BLE.addService(battService);
  BLE.addService(deviceService);
  
} // setup()

void loop() {
unsigned char c;

  // start advertising
  Serial.println("BLE advertising started");
  BLE.advertise();

  // listen for BLE peripherals to connect:
  BLEDevice central = BLE.central();
 // if a central is connected to peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());
    while (central.connected()) {
      c = GetKey();
      if (c != 0)
      {
        Serial.print("Key = 0x"); Serial.println(c, HEX);
        SendKeyReport(c);
      }
      else
      {
        delay(10);
      }
      if (bDisconnected)
      {
        bDisconnected = 0;
        Serial.println("Starting over...");
        return;
      }
    } // while connected
  } // if connected
} // loop()
