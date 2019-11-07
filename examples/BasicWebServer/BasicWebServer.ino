#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SDFat.h>
#include <Ethernet.h>

#include <YAAWS.h>

//  Some SD card readers don't play well with other SPI devices.  In that case,
//  we can use software SPI for Sd card reader. #define
//  ENABLE_SOFTWARE_SPI_CLASS=1 as a compiler option, or edit
//  SdFat/SdFatConfig.h
#if ENABLE_SOFTWARE_SPI_CLASS
constexpr byte SOFT_SCK_PIN = 5;
constexpr byte SOFT_MOSI_PIN = 6;
constexpr byte SOFT_MISO_PIN = 7;

SdFatSoftSpi<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> SD;
#else
SdFat SD;
#endif

//
constexpr byte SDCARD_CS = 4;

constexpr byte ETHERNET_CS = 53;

IPAddress ip(192, 168, 2, 90);
byte mac[] = {0xDF, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

//  Default port (80), default callback handler with web-root "/WWW/", no file
//  actions, no form processing.
YAAWS web(SD);


// the setup function runs once when you press reset or power the board
void setup()
{
	//  Set the SPI chip select pins all HIGH so that they don't interfere with
	//  each other during initialization.
	digitalWrite(SDCARD_CS, HIGH);
	pinMode(SDCARD_CS, OUTPUT);

	digitalWrite(ETHERNET_CS, HIGH);
	pinMode(ETHERNET_CS, OUTPUT);

	Serial.begin(115200);

	while (!Serial);  //  Wait for Serial to initialize
	
	Wire.begin();
	Wire.setClock(3400000);

	if (!SD.begin(SDCARD_CS))
	{
		Serial.println(F("Can't read SD card."));
	}
	else
	{
		Serial.print(F("Card Capacity: "));
		Serial.print(SD.card()->cardCapacity());
		Serial.println(F(" 512 byte sectors"));
	}

	Ethernet.init(ETHERNET_CS);
	Ethernet.begin(mac, ip);

	if (!web.begin())
	{
		Serial.println(F("Can't start web server"));
	}
	else
	{
		Serial.println(F("Web server started"));
	}

}

// the loop function runs over and over again until power down or reset
void loop()
{
	web.ServiceWebServer();
}
