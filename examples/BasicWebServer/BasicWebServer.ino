//  MIT License
//
//  Copyright(c) 2019 M Hotchin
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this
//  software and associated documentation files(the "Software"), to deal in the Software
//  without restriction, including without limitation the rights to use, copy, modify,
//  merge, publish, distribute, sublicense, and/or sell copies of the Software, andto
//  permit persons to whom the Software is furnished to do so, subject to the following
//  conditions :
//
//  The above copyright notice andthis permission notice shall be included in all copies
//  or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
//  PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
//  CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
//  OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SDFat.h>
#include <Ethernet.h>

#include <YAAWS.h>

//  Some SD card readers don't play well with other SPI devices.  In that case, we can use
//  software SPI for Sd card reader. #define ENABLE_SOFTWARE_SPI_CLASS=1 as a compiler
//  option, or edit SdFat/SdFatConfig.h
#if ENABLE_SOFTWARE_SPI_CLASS
constexpr byte SOFT_SCK_PIN = 5;
constexpr byte SOFT_MOSI_PIN = 6;
constexpr byte SOFT_MISO_PIN = 7;

SdFatSoftSpi<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> SD;
#else
SdFat SD;
#endif

//  Set these to match your hardware.  Pin 10 is also common for Ethernet.
constexpr byte SDCARD_CS = 4;
constexpr byte ETHERNET_CS = 53;

byte mac[] = {0xDF, 0xAD, 0xBE, 0xEF, 0xFE, 0xE1};

//  Default port (80), default callback handler with web-root "/WWW", no file
//  actions, no form processing.
YAAWS web(SD);

//  The SD card you use should have a 'WWW' directory at the root of the card, then some
//  files that you can use to test the web-server.  If you don't want to make up HTML,
//  just create some .TXT files, and retrieve then by name, ie 'http://<Ip
//  Address>/filename.txt'.

void setup()
{
	//  Set the SPI chip select pins all HIGH so that they don't interfere with each other
	//  during initialization.
	digitalWrite(SDCARD_CS, HIGH);
	pinMode(SDCARD_CS, OUTPUT);

	digitalWrite(ETHERNET_CS, HIGH);
	pinMode(ETHERNET_CS, OUTPUT);

	Serial.begin(115200);

	while (!Serial);  //  Wait for Serial to initialize

	Serial.print(F("YAAWS BasicWebServer example."));

	if (!SD.begin(SDCARD_CS))
	{
		Serial.println(F("Can't read SD card."));
	}

	Ethernet.init(ETHERNET_CS);

	//  Configure using DHCP address.
	if (!Ethernet.begin(mac, 10))
	{
		Serial.println(F("Ethernet initialization failed!"));
	}

	//  If you want a fixed address, then use this instead.  Make sure you use an IP
	//  address that is on your network!
	//IPAddress ip(192, 168, 2, 90);
	//Ethernet.begin(mac, ip);

	Serial.print(F("Local IP address: "));
	Serial.println(Ethernet.localIP());

	if (!web.begin())
	{
		Serial.println(F("Can't start web server"));
	}
	else
	{
		Serial.println(F("Web server started"));
	}
}


void loop()
{
	web.ServiceWebServer();
}
