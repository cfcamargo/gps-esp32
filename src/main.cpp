#define MONITOR_SPEED 9600
#define GPS_TX 26
#define GPS_RX 27
#define GPS_SERIAL_SPEED 9600
#define GPS_SATTELITES_TO_TRUST 7
#define UPDATE_PERIOD_MS 500
#define SOFT_SERIAL_INVERT false
#define SOFT_SERIAL_BUF_SIZE 256

#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID "a074e742-c933-40a2-b503-118d042d5bb1"
#define GLL_CHARACTERISTIC_UUID "c9580287-61a5-41b8-8271-df99f7446c92"
#define GGA_CHARACTERISTIC_UUID "1aa65db3-9ae5-4058-8639-0981fb724d39"
#define GSV_CHARACTERISTIC_UUID "10e2e8f6-16cb-460f-921d-a0e39186ecc0"
#define GSA_CHARACTERISTIC_UUID "c7be6b7a-a832-4596-aad0-68799ab85ca3"
#define UNIFIED_CHARACTERISTIC_UUID "96269f2b-d2c9-42f7-b169-f598830ad52f"

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristicGGA;
BLECharacteristic *pCharacteristicGLL;
BLECharacteristic *pCharacteristicGSV;
BLECharacteristic *pCharacteristicGSA;
BLECharacteristic *pCharacteristicUnified;

class ServerCallbacks : public BLEServerCallbacks
{

	void onConnect(BLEServer *pServer)
	{
		Serial.print("Someone connected on BLE!");
	}

	void onDisconnect(BLEServer *pServer)
	{
		Serial.print("Someone disconnected on BLE!");
		pServer->startAdvertising();
	}
};

// Loop-Multitask time control
unsigned long previousGpsUpdate = 0;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss;

typedef struct
{
	bool trust;
	double latitude;
	double longitude;
	double altitude;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint8_t day;
	uint8_t month;
	int16_t year;
	uint32_t satellites;
	uint32_t age;
} gpsCoordinate;

gpsCoordinate gc;

void setup()
{
	Serial.begin(MONITOR_SPEED);
	ss.begin(GPS_SERIAL_SPEED, SWSERIAL_8N1, GPS_RX, GPS_TX, SOFT_SERIAL_INVERT, SOFT_SERIAL_BUF_SIZE);

	BLEDevice::init("GeoFarm GPS");
	pServer = BLEDevice::createServer();
	pServer->setCallbacks(new ServerCallbacks());
	pService = pServer->createService(SERVICE_UUID);
	pCharacteristicGGA = pService->createCharacteristic(
			GGA_CHARACTERISTIC_UUID,
			BLECharacteristic::PROPERTY_READ);
	pCharacteristicGLL = pService->createCharacteristic(
			GLL_CHARACTERISTIC_UUID,
			BLECharacteristic::PROPERTY_READ);
	pCharacteristicGSA = pService->createCharacteristic(
			GSA_CHARACTERISTIC_UUID,
			BLECharacteristic::PROPERTY_READ);
	pCharacteristicGSV = pService->createCharacteristic(
			GSV_CHARACTERISTIC_UUID,
			BLECharacteristic::PROPERTY_READ);
	pCharacteristicUnified = pService->createCharacteristic(
			UNIFIED_CHARACTERISTIC_UUID,
			BLECharacteristic::PROPERTY_READ);
	pService->start();
	BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
	pAdvertising->addServiceUUID(SERVICE_UUID);
	pAdvertising->setScanResponse(true);
	pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
	pAdvertising->setMinPreferred(0x12);
	BLEDevice::startAdvertising();
}

char gll[600] = "";
char gga[600] = "";
char gsa[600] = "";
char gsv[600] = "";
char line[600] = "";
int pos = 0;

void loop()
{
	//updatePositionTask();
	while (ss.available() > 0)
	{
		char c = ss.read();
		gps.encode(c);
		if (gps.location.isUpdated())
		{
				gc.latitude = gps.location.lat();
				gc.longitude = gps.location.lng();
				gc.altitude = gps.altitude.meters();
				gc.day = gps.date.day();
				gc.month = gps.date.month();
				gc.year = gps.date.year();
				gc.hour = gps.time.hour();
				gc.minute = gps.time.minute();
				gc.second = gps.time.second();
				gc.satellites = gps.satellites.value();
				gc.age = gps.date.age();
				gc.trust = true;
				char value[200] = "";
				memset(value, '\0', 200);
				sprintf(value, "{ latitude: %2.7f, longitude: %2.7f, altitude: %2.7f, sattelites: %d }", gc.latitude, gc.longitude, gc.altitude, gc.satellites);
				pCharacteristicUnified->setValue(value);
				Serial.println(value);
		}

		line[pos] = c;
		if (pos > 0 && line[pos] == '\n')
		{
			char start[7] = "";
			strncpy(start, line, 6);
			if (strcmp(start, "$GPGLL") == 0)
			{
				strcpy(gll, line);
				Serial.println(gll);
				pCharacteristicGLL->setValue(gll);
			}
			else if (strcmp(start, "$GPGGA") == 0)
			{
				strcpy(gga, line);
				Serial.println(gga);
				pCharacteristicGGA->setValue(gga);
			}
			else if (strcmp(start, "$GPGSA") == 0)
			{
				strcpy(gsa, line);
				Serial.println(gsa);
				pCharacteristicGSA->setValue(gsa);
			}
			else if (strcmp(start, "$GPGSV") == 0)
			{
				strcpy(gsv, line);
				Serial.println(gsv);
				pCharacteristicGSV->setValue(gsv);
			}
			memset(line, '\0', 600);
			pos = 0;
		}
		else
		{
			pos++;
		}
	}
}