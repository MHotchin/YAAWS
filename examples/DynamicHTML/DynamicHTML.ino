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

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SDFat.h>
#include <Ethernet.h>

#include <YAAWS.h>

//  Some SD card readers don't play well with other SPI devices.  In that case, we can use
//  software SPI for SD card reader. #define ENABLE_SOFTWARE_SPI_CLASS=1 as a compiler
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

byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02};

//
// In order to change the contents of a file, that file must be read-write on the SD card,
// and you must provide a callback with the following two methods.  'IsMutable' is called
// to determine which files you want to change, and 'FileAction' is called to do the
// actual work.
class DynamicHTMLHandler : public YaawsCallback
{
public:
	DynamicHTMLHandler();

	bool IsMutable(const char *path);
	bool FileAction(EthernetClient &, WebFileType &);

private:
	//  If you process over multiple calls, you'll need to keep your state in the handler.
	int _stepNumber;  //  What 'step' of processing we are on.
};

DynamicHTMLHandler::DynamicHTMLHandler()
	: _stepNumber(0)
{}


bool DynamicHTMLHandler::IsMutable(
	const char *path)
{
	Serial.print(F("IsMutable() called for: "));
	Serial.println(path);

	auto val = strcasecmp_P(path, PSTR("/analog.html"));

	if (val == 0)
		_stepNumber = 0;

	return (val == 0);
}


//  The HTML we will emit once the 'special' file is requested.
const char Html1[] PROGMEM =
"<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">"
"<html>"
"<head>"
"<title>Analog Pins</title>"
"</head>"
"<body>"
"<div style=\"width:400px\">"
"<h1>Analog Pin Values";

const char Html2[] PROGMEM =
"</h1>"
"<hr>"
"<strong>"
"<span style=\"float:left\">Analog Pin</span><span style=\"float:right\">Value</span>"
"</strong>"
"<br>";


const char Html3[] PROGMEM =
"<hr>"
"</div>"
"</body>"
"</html>";

//  We use this special type to tell the 'print' routines that the text is actually in
//  PROGMEM.
typedef const __FlashStringHelper *fsh;


//  The first time this is called, the HTTP response header has already been sent to the
//  client, and the file is positioned at the beginning, with nothing sent.  Once you
//  return 'false', the server will take care of sending the rest of the file and closing
//  the connection, and you will not be called any more for this request.
bool DynamicHTMLHandler::FileAction(
	EthernetClient &client,
	WebFileType &file)
{

	//  If you have multiple files you want to change in different ways, you can check the
	//  file name here (using file.getName()),then process the files differently.

	//  The file we are changing doesn't have to have anything in it!
	//  We can replace the file entirely, it just has to exist.

	//  You can do as much or as little work as you like each time you are called, keeping
	//  in mind that long operations will block the rest of your sketch.
	//
	//  For demonstration purposes, we split the processing here into 3 phases.
	if (_stepNumber == 0)
	{
		client.print((fsh)Html1);
		_stepNumber++;

		return true;
	}

	if (_stepNumber == 1)
	{
		client.print((fsh)Html2);

		for (auto i = A0; i < A5; i++)
		{
			auto val = analogRead(i);

			client.print(F("<span style=\"float:left\">"));
			client.print(i);
			client.print(F("</span><span style=\"float:right\">"));
			client.print(val);
			client.print(F("</span><br>"));
		}
		_stepNumber++;
		return true;
	}

	if (_stepNumber == 2)
	{
		client.print((fsh)Html3);
		_stepNumber++;

		//  Just skip over the file entirely!
		file.seekEnd();
	}

	//  Now let the server do the rest.
	return false;
}

DynamicHTMLHandler handler;

//  Default port (80), and the handler above.  Uses default web root and default (none)
//  form handling.
YAAWS web(SD, handler);

//  The SD card you use should have a 'WWW' directory at the root of the card, then some
//  files that you can use to test the web-server.  The 'WebSite' directory under
//  'Examples' has some files you can use.
//
//  For this example, you will need 'analog.html' in your web root directory.  It can
//  contain whatever you like, or even contain nothing at all for this example.

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

	Serial.println(F("YAAWS DynamicHTML example"));


	Ethernet.init(ETHERNET_CS);

	//  Configure using DHCP address.
	if (!Ethernet.begin(mac))
	{
		Serial.println(F("Ethernet initialization failed!"));
	}

	//  If you want a fixed address, then use this instead.  Make sure you use an IP
	//  address that is on your network!  

	//  IPAddress ip(192, 168, 2, 90);
	//  Ethernet.begin(mac, ip);

	Serial.print(F("Local IP address: "));
	Serial.println(Ethernet.localIP());

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
