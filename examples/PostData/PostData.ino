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


byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02};

class PostHandler : public YaawsCallback
{
	bool ProcessPostData(const char *path, EthernetClient &,
						 unsigned long contentLength);
};


bool PostHandler::ProcessPostData(
	const char *path,
	EthernetClient &client,
	unsigned long contentLength)
{
	//  Since we control how the POST data is read in, we don't have the same limits on
	//  how much there can be.  For this demonstration, each name / value pair can be up
	//  to 128 bytes (URL encoded length), and you can have as many as you want.
	constexpr size_t buffSize = 128;
	char buffer[buffSize + 1];   //  Add one for the terminating NUL.

	Serial.print(F("Post data for page '"));
	Serial.print(path);
	Serial.println(F("'"));
	Serial.print(F("Content Length: "));
	Serial.println(contentLength);

	unsigned long totalRead = 0;

	//  Fill up our buffer with stuff to process.  We can avoid long timeouts by never
	//  asking for more than a total of 'contentLength' bytes of data.
	auto amountRead = client.readBytes(buffer, min(buffSize, contentLength));
	buffer[amountRead] = '\0';

	totalRead += amountRead;

	char *separator = nullptr;

	do
	{
		//  Look for the end of the first item in the buffer.
		separator = strchr(buffer, '&');

		if ((separator == nullptr) && (totalRead < contentLength))
		{
			//  Too much data for one set of values.
			return false;
		}

		if (separator != nullptr)
		{
			//  Convert the first set of values into a NUL terminated string.
			*separator = '\0';
		}

		if (*buffer != '\0')
		{
			//  Use a function in the base class to break up the name / value set.
			queryPair nameValuePair;

			getNextQueryPair(buffer, nameValuePair);

			//  Do whatever you need to do with this name / value pair.
			Serial.print(F("Name: '"));
			Serial.print(nameValuePair._name);
			Serial.print(F("'  Value: '"));
			Serial.print(nameValuePair._value != nullptr ? nameValuePair._value : "<null>");
			Serial.println(F("'"));

			if (separator != nullptr)
			{
				char *nextItem = separator + 1;
				auto length = strlen(nextItem);

				//  Move the remaining data to the beginning of the buffer...
				memmove(buffer, nextItem, length + 1);

				// ...and try to refill the end of the buffer
				if (totalRead < contentLength)
				{
					auto amountToRead = min(buffSize - length, contentLength - totalRead);
					amountRead = client.readBytes(buffer + length, amountToRead);

					buffer[length + amountRead] = '\0';
					totalRead += amountRead;
				}
			}
		}

	} while (separator != nullptr);

	return true;
}


PostHandler handler;

//  Default port (80), and the handler above.  Uses default web root.
YAAWS web(SD, handler);


// the setup function runs once when you press reset or power the board
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

	Serial.println(F("YAAWS PostData example"));

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
