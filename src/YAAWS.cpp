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
#include <Ethernet.h>
#include <SdFat.h>
#include <alloca.h>

#include "YAAWS.h"

extern int __heap_start, *__brkval;
namespace
{
	int freeRam()
	{

		int v;
		return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
	}

#ifndef YAAWS_HUSH_NOW
#define TRACE(X) Serial.print(millis()),Serial.print(F("  ")),Serial.println(X)
#define IF_TRACE(X) (X)
	void quotedTrace(const char *p)
	{
		Serial.print(F("\""));
		Serial.print(p);
		Serial.println(F("\""));
	}
#else
#define TRACE(X) (void)0
#define IF_TRACE(X) (void)0

#define quotedTrace(x) (void)0;

#endif
	class FlashyFlashy
	{
	public:
#ifndef YAAWS_NO_FLASHY_FLASHY
		FlashyFlashy();
		~FlashyFlashy();
#endif
	};

#ifndef YAAWS_NO_FLASHY_FLASHY
	FlashyFlashy::FlashyFlashy()
	{
		digitalWrite(LED_BUILTIN, HIGH);
	}

	FlashyFlashy::~FlashyFlashy()
	{
		digitalWrite(LED_BUILTIN, LOW);
	}
#endif

	// URl decoder, taken from
	// https://stackoverflow.com/questions/2673207/c-c-url-decode-library/19826808
	// License is CC-BY-SA https://creativecommons.org/licenses/by-sa/4.0/
	void urldecode2(const char *src, char *dst)
	{
		if (!src || !dst) return;

		char a, b;
		while (*src)
		{
			if ((*src == '%') &&
				((a = src[1]) && (b = src[2])) &&
				(isxdigit(a) && isxdigit(b)))
			{
				if (a >= 'a')
					a -= 'a' - 'A';
				if (a >= 'A')
					a -= ('A' - 10);
				else
					a -= '0';
				if (b >= 'a')
					b -= 'a' - 'A';
				if (b >= 'A')
					b -= ('A' - 10);
				else
					b -= '0';
				*dst++ = 16 * a + b;
				src += 3;
			}
			else if (*src == '+')
			{
				*dst++ = ' ';
				src++;
			}
			else
			{
				*dst++ = *src++;
			}
		}
		*dst++ = '\0';
	}

	//  In place URL decoder.  Decoding *never* producings a string larger than its input.
	void urldecode2(char *srcdst)
	{
		return urldecode2(srcdst, srcdst);
	}
}

//  Number of items in an array
#define COUNTOF(x) (sizeof(x)/sizeof(x[0]))

//  Default implementation accepts all inputs as valid.
bool YaawsCallback::ProcessFormData(
	const char *path, char *formData)
{
	return true;
}


#ifndef YAAWS_GET_IS_ALL_WE_NEED

//  Default implementation just passes the POST data as a URL query string to the GET
//  handler.
bool YaawsCallback::ProcessPostData(
	const char *path,
	EthernetClient &client,
	unsigned long contentLength)
{
	constexpr size_t bufferSize = 128;
	char buffer[bufferSize + 1];

	if (contentLength > bufferSize)
	{
		return false;
	}

	auto amountRead = client.readBytes(buffer, bufferSize);

	buffer[amountRead] = '\0';

	return ProcessFormData(path, buffer);
}
#endif

#ifndef YAAWS_NOTHING_EVER_CHANGES
bool YaawsCallback::IsMutable(
	const char *)
{
	return false;
}


bool YaawsCallback::FileAction(
	EthernetClient &, WebFileType &)
{
	return false;
}
#endif

char *YaawsCallback::getNextQueryPair(
	char *queryString,
	queryPair &nameValuePair)
{
	nameValuePair._name = nullptr;
	nameValuePair._value = nullptr;

	if ((queryString == nullptr) || (*queryString == '\0'))
	{
		return nullptr;
	}

	char *retVal = nullptr;

	char *end = strchr(queryString, '&');

	if (end != nullptr)
	{
		*end = '\0';
		retVal = end + 1;
	}

	char *split = strchr(queryString, '=');

	if (split != nullptr)
	{
		*split = '\0';
		split++;
	}

	//  Now that things have been split apart at the markers, we can finally decode the
	//  strings.
	urlDecode(queryString);
	urlDecode(split);

	nameValuePair._name = queryString;
	nameValuePair._value = split;

	return retVal;
}

void YaawsCallback::urlDecode(
	char *encodedString)
{
	urldecode2(encodedString);
}

YaawsCallback defaultCallback;

//  All the different HTML 'Content-types' supported.
//
enum YAAWS::ResponseType : byte
{
	htm404,
	htm200,
	jpg200,
	gif200,
	png200,
	bmp200,
	ico200,
	svg200,
	txt200,
	js200,
	css200,
	csv200,
	eot200,
	woff200,
	woff2200,
	ttf200,
	default200,  //  Everything else
	UNKNOWN,     //  Response type not yet identified
	FINISHED     //  Response has been sent

};


namespace
{
	//  Text for all the different HTML Response headers. Except for 404, all the rest
	//  differ only by the value of the 'Content-Type' directive.
	const char str200Header[] PROGMEM =
		"HTTP/1.0 200 OK\n"
		"Server: YAAWS/1.0\n"
		"Connection: close\n"
		"Content-Type: ";

	const char str404[] PROGMEM =
		"HTTP/1.0 404 Not Found\n"
		"Content-Type: text/html\n"
		"Connection: close\n\n";

	//  Read-only files are marked cachable.
	const char strCacheable[] PROGMEM =
		"Cache-Control: public, max-age=604800\n";

	//  If not read-only, the files are not cachable.  If you update these files, the
	//  client will always get the latest version.
	const char strNonCacheable[] PROGMEM =
		"Cache-Control: no-cache, no-store, must-revalidate\n";

	const char strHtm200[] PROGMEM = "text/html";
	const char strJpg200[] PROGMEM = "image/jpeg";
	const char strGif200[] PROGMEM = "image/gif";
	const char strPng200[] PROGMEM = "image/png";
	const char strBmp200[] PROGMEM = "image/bmp";
	const char strIco200[] PROGMEM = "image/vnd.microsoft.icon";
	const char strSvg200[] PROGMEM = "image/svg+xml";
	const char strTxt200[] PROGMEM = "text/plain";
	const char strJs200[] PROGMEM = "text/javascript";
	const char strCss200[] PROGMEM = "text/css";
	const char strCsv200[] PROGMEM = "text/csv";
	const char strEot200[] PROGMEM = "application/vnd.ms-fontobject";
	const char strWoff200[] PROGMEM = "font/woff";
	const char strWoff2200[] PROGMEM = "font/woff2";
	const char strTtf200[] PROGMEM = "font/ttf";
	const char strDefault200[] PROGMEM = "application/octet-stream";


	//  Same order as ResponseType, above
	const char *const aResponses[] PROGMEM =
	{
		str404,
		strHtm200,
		strJpg200,
		strGif200,
		strPng200,
		strBmp200,
		strIco200,
		strSvg200,
		strTxt200,
		strJs200,
		strCss200,
		strCsv200,
		strEot200,
		strWoff200,
		strWoff2200,
		strTtf200,
		strDefault200
	};


	//  The diferent types of file extensions supported....
#define DECLARE_EXT(x) const char ext##x[] PROGMEM = #x;
	DECLARE_EXT(HTM);
	DECLARE_EXT(HTML);
	DECLARE_EXT(JPG);
	DECLARE_EXT(JPEG);
	DECLARE_EXT(GIF);
	DECLARE_EXT(PNG);
	DECLARE_EXT(ICO);
	DECLARE_EXT(BMP);
	DECLARE_EXT(SVG);
	DECLARE_EXT(TXT);
	DECLARE_EXT(LOG);
	DECLARE_EXT(JS);
	DECLARE_EXT(CSS);
	DECLARE_EXT(CSV);
	DECLARE_EXT(EOT);  //  Web Fonts
	DECLARE_EXT(WOFF); //  Web Fonts
	DECLARE_EXT(WOFF2); // Web Fonts
	DECLARE_EXT(TTF);

	struct ExtensionResponseType
	{
		const char *strExt;
		YAAWS::ResponseType rt;
	};

	// ...and the response type for each one.
	const ExtensionResponseType ext2rt[] PROGMEM =
	{
		{extHTM, YAAWS::htm200},
	{extHTML, YAAWS::htm200},
	{extJPG, YAAWS::jpg200},
	{extJPEG, YAAWS::jpg200},
	{extGIF, YAAWS::gif200},
	{extPNG, YAAWS::png200},
	{extICO, YAAWS::ico200},
	{extBMP, YAAWS::bmp200},
	{extSVG, YAAWS::svg200},
	{extTXT, YAAWS::txt200},
	{extLOG, YAAWS::txt200},
	{extJS, YAAWS::js200},
	{extJPG, YAAWS::js200},
	{extCSS, YAAWS::css200},
	{extCSV, YAAWS::csv200},
	{extEOT, YAAWS::eot200},
	{extWOFF, YAAWS::woff200},
	{extWOFF2, YAAWS::woff2200},
	{extTTF, YAAWS::ttf200}
	};

	constexpr size_t NumExtensions = COUNTOF(ext2rt);
}



YAAWS::YAAWS(
	webSdCard &SdCard,
	YaawsCallback &callback,
	const char *webRoot,
	uint16_t port)
	: _server(port), _SdCard(SdCard), _callback(callback), _webRoot(webRoot),
	_activeConnections(0)

{
#ifndef YAAWS_ONE_STREAM_ONLY
	_serviceIndex = 0;
#endif
}




YAAWS::YAAWS(
	webSdCard &SdCard,
	const char *webRoot,
	uint16_t port)
	: _server(port), _SdCard(SdCard), _callback(defaultCallback), _webRoot(webRoot),
	_activeConnections(0)
{
#ifndef YAAWS_ONE_STREAM_ONLY
	_serviceIndex = 0;
#endif
}




//  If this returns false, your webserver won't be working.  Check that your ethernet and
//  SD Card are working.
bool YAAWS::begin(void)
{
#ifndef YAAWS_NO_FLASHY_FLASHY
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);
#endif

	_server.begin();

	if ((bool)_server)
	{
		TRACE(F("YAAWS is listening"));
	}

	return ((bool)_server && (Ethernet.hardwareStatus() != EthernetNoHardware) &&
		(_SdCard.volumeBlockCount() > 0));


}

//  Check the filename and return the extension type.
YAAWS::ResponseType YAAWS::GetResponseType(const char *fileName)
{
	// Find start of filename, then the extension if it exists
	const char *nameStart = strrchr(fileName, '/');
	const char *extPos = strrchr(nameStart ? nameStart : fileName, '.');

	// Use the file extension to decide the 'Content-type' in the HTML response header.
	if (extPos)
	{
		extPos++;

		for (size_t i = 0; i < NumExtensions; i++)
		{
			//  Get the extension data from PROGMEM...
			ExtensionResponseType extrt;
			memcpy_P(&extrt, &ext2rt[i], sizeof(extrt));

			// ... and see if it matches.
			if (strcasecmp_P(extPos, extrt.strExt) == 0)
			{
				return extrt.rt;
			}
		}
	}

	//  Didn't recognize this file type, so use the default

	TRACE(F("Using default file type!"));

	return YAAWS::default200;
}



//  Send the first part of the response - the HTML Response Header.  The 'Content-Type'
//  directive is determined by the file's extension.
void YAAWS::SendResponseHeader()
{
	TRACE(F("SendResponseHeader"));
	constexpr size_t buffSize = 256;
	char buffer[buffSize + 1] = {0};
	ContinuationData &contData = _contData[_serviceIndex];


	//  404 is one piece, all others are made up of three pieces.
	if (contData.rt != htm404)
	{
		strncpy_P(buffer, str200Header, buffSize);
	}

	strncat_P(buffer, pgm_read_ptr(&aResponses[contData.rt]), buffSize);

	if (contData.rt != htm404)
	{
		strncat_P(buffer, PSTR("\n"), buffSize);

		bool isCacheable = contData.sdFile.isReadOnly();

		if (isCacheable)
		{
			strncat_P(buffer, strCacheable, buffSize);
		}
		else
		{
			strncat_P(buffer, strNonCacheable, buffSize);
		}

		//  If the file is not mutable, we can provide a Content-length: directive.  Only
		//  mutable files will have this set at this point.
#ifndef YAAWS_NOTHING_EVER_CHANGES
		if (!contData.doFileAction)
#endif
		{
			strncat_P(buffer, PSTR("Content-length: "), buffSize);
			ltoa(_contData[_serviceIndex].sdFile.fileSize(), buffer + strlen(buffer), 10);
			strncat_P(buffer, PSTR("\n"), buffSize);

			//  TODO - Etag validation not yet implemented.
		}

		strncat_P(buffer, PSTR("\n"), buffSize);
	}

	buffer[buffSize] = '\0';

	FlashyFlashy ff;
	contData.client.write(buffer);

	return;
}


//  Clean up our connection once we are done with it, whatever the reason.  Flush the
//  data, mark the connection as available
void YAAWS::FinishConnection()
{
	ContinuationData &contData = _contData[_serviceIndex];

	FlashyFlashy ff;

	if (contData.client.connected())
	{
		contData.client.flush();
		contData.client.stop();

		contData.sdFile.close();
	}

	_activeConnections &= ~(1 << _serviceIndex);

	TRACE(F("Request complete."));
}


//  Send the actual file to the requestor.  Will return before sending the whole file,
//  called repeatedly to keep things going.
void YAAWS::SendSdFile()
{
	ContinuationData &contData = _contData[_serviceIndex];
	int amountToWrite = contData.client.availableForWrite();

	if (amountToWrite > 0)
	{
		//  Maximum we will write in one go. If we have only one client, use as much of
		//  the underlying Ethernet frame size as possible.  Otherwise, limit to a
		//  multiple of the SD sector size to reduce SD card redundant reads. 
		constexpr int maxBufferSize = (MAX_CLIENTS == 1) ? 1400 : 1024;

		amountToWrite = min(amountToWrite, maxBufferSize);

#ifndef YAAWS_ONE_STREAM_ONLY
		//  If we want aligned reads, then make sure we are actually aligned.
		int alignment = (maxBufferSize - (contData.sdFile.curPosition() % 512));
		amountToWrite = min(amountToWrite, alignment);
#endif

		//  Leaving 100 bytes seems to work fine.
		//  TO-DO - Fine tune this somehow?
		const int stackAvailable = freeRam() - 100;
		const int sdFileLeft = contData.sdFile.available();

		amountToWrite = min(amountToWrite, stackAvailable);
		amountToWrite = min(amountToWrite, sdFileLeft);

		if (amountToWrite > 0)
		{
			byte *pBuffer = alloca(amountToWrite);

			FlashyFlashy ff;

			amountToWrite = contData.sdFile.read(pBuffer, amountToWrite);
			contData.client.write(pBuffer, amountToWrite);
		}
		else
		{
			FinishConnection();
		}
	}
}

// In general, we don't want Service calls to take *too* long. If a request is waiting,
// then the first call will receive it, next will send back the response header. After
// that, each call will transmit part of the response file.
void YAAWS::ContinueRequest(void)
{
	ContinuationData &contData = _contData[_serviceIndex];

	if (contData.rt != FINISHED)
	{
		SendResponseHeader();
		contData.rt = FINISHED;
	}
#ifndef YAAWS_NOTHING_EVER_CHANGES
	else if (contData.doFileAction)
	{
		contData.doFileAction =
			_callback.FileAction(contData.client, contData.sdFile);
	}
#endif
	else
	{
		SendSdFile();

		if (contData.sdFile.available() == 0)
		{
			IF_TRACE(Serial.println(F("SendSdFile() completed")));
			FinishConnection();
		}
	}

	return;
}


//  Uses 'filename' as a caller provided buffer for building the filename.  Reduces max
//  stack usage by re-using already allocated space.  We'll just assume things fit.
void YAAWS::Return404(char *fileName)
{
	TRACE(F("404!"));
	strcpy_P(fileName, GetWebRoot());

	strcat_P(fileName, PSTR("/404.html"));

	ContinuationData &contData = _contData[_serviceIndex];

	contData.sdFile.open(fileName, O_READ);
	contData.rt = htm404;

	//  If there is no custom 404 file, send a canned response.
	if (!contData.sdFile.isOpen())
	{
		FlashyFlashy ff;

		contData.client.print(F(
			"HTTP/1.0 404 Not Found\n"
			"Content-Type: text/html\n"
			"Connection: close\n\n"
			"<HTML>\n"
			"<HEAD>\n"
			"<title>Page Not Found</title>\n"
			"</HEAD>\n"
			"<BODY>\n"
			"<h1>Error 404</h1>\n"
			"<br>Page Not Found\n"
			"</BODY>\n"
			"</HTML>\n\n"));

		FinishConnection();
	}

	//  Normal file processing will send the 404 file back to the client.
}

#ifndef YAAWS_404_THE_ONE_TRUE_ERROR
void YAAWS::Return400BadRequest()
{
	FlashyFlashy ff;

	_contData[_serviceIndex].client.print(F(
		"HTTP/1.0 400 Bad Request\n"
		"Content-Type: text/html\n"
		"Connection: close\n\n"
		"<HTML>\n"
		"<HEAD>\n"
		"<title>Bad Request</title>\n"
		"</HEAD>\n"
		"<BODY>\n"
		"<h1>Error 400</h1>\n"
		"<br>The Request is malformed.\n"
		"</BODY>\n"
		"</HTML>\n\n"));

	FinishConnection();
}


void YAAWS::Return405MethodNotAllowed()
{
	FlashyFlashy ff;

	_contData[_serviceIndex].client.print(F(
		"HTTP/1.0 405 Method Not Allowed\n"
		"Content-Type: text/html\n"
#ifndef YAAWS_GET_IS_ALL_WE_NEED		
		"Allow: GET, HEAD, POST\n"
#else
		"Allow: GET, HEAD\n"
#endif
		"Connection: close\n\n"
		"<HTML>\n"
		"<HEAD>\n"
		"<title>Method Not Allowed</title>\n"
		"</HEAD>\n"
		"<BODY>\n"
		"<h1>Error 405</h1>\n"
		"<br>The Request Method is invalid for this resource.\n"
		"</BODY>\n"
		"</HTML>\n\n"));

	FinishConnection();
}


void YAAWS::Return414UriTooLong()
{
	FlashyFlashy ff;

	_contData[_serviceIndex].client.print(F(
		"HTTP/1.0 414 URI Too Long\n"
		"Content-Type: text/html\n"
		"Connection: close\n\n"
		"<HTML>\n"
		"<HEAD>\n"
		"<title>Request Too Long</title>\n"
		"</HEAD>\n"
		"<BODY>\n"
		"<h1>Error 414</h1>\n"
		"<br>The Request is too long.  Limit is 128 characters\n"
		"</BODY>\n"
		"</HTML>\n\n"));

	FinishConnection();
}
#endif

namespace
{
	enum RequestType
	{
		rtGet,
		rtHead,
		rtPost,
		rtUnknown
	};

	const char strGet[] PROGMEM = "GET /";
	const char strHead[] PROGMEM = "HEAD /";
#ifndef YAAWS_GET_IS_ALL_WE_NEED
	const char strPost[] PROGMEM = "POST /";
#endif
	struct Request
	{
		RequestType rt;
		const char *rs;
	};

	const Request aRequests[] PROGMEM =
	{
		{rtGet, strGet},
	{rtHead, strHead},
#ifndef YAAWS_GET_IS_ALL_WE_NEED
	{rtPost, strPost},
#endif
	};


	//  Figure out the request type and move the URI down.
	RequestType GetRequestType(char *str)
	{
		IF_TRACE(Serial.println(str));

		RequestType rt = rtUnknown;

		for (size_t i = 0; i < COUNTOF(aRequests); i++)
		{
			//  Get the request type data from PROGMEM...
			Request r;

			memcpy_P(&r, &aRequests[i], sizeof(r));

			size_t len = strlen_P(r.rs);

			// ... and see if it matches.
			if (memcmp_P(str, r.rs, len) == 0)
			{
				rt = r.rt;
				char *filename = str + len - 1;

				memmove(str, filename, strlen(filename) + 1);

				break;
			}
		}

		//  Do nothing for unknown request types
		return rt;
	}

	const char contentLengthMarker[] PROGMEM = "content-length: ";
}




//  Initial processing of any request:
//   - Read the request from the client.
//   - Validate request type
//   - Validate file to return
//   - Process any form data
void YAAWS::AcceptIncoming()
{
	// TO-DO - better buffer size for this?
	constexpr size_t buffSize = 128;
	char inputFileName[buffSize + 1] = {0};

	inputFileName[buffSize] = '\0';

	ContinuationData &contData = _contData[_serviceIndex];

	//  Well isn't THAT special.  I'm seeing cases where the connection does not arrive
	//  with the payload.  If you wait long enough, it seems to show up.  Up to 30 ms
	//  seems possible.
	if ((contData.rt == UNKNOWN) && !contData.client.available())
	{
		//  There's no timeout on this - we will wait as long as the connection is held
		//  open.
		TRACE(F("Awaiting incoming payload"));
		return;
	}

	//  Start building the filename to be returned.
	strcpy_P(inputFileName, GetWebRoot());

	size_t inputOffset = strlen(inputFileName);
	char *pRequestStart = inputFileName + inputOffset;

	//  Read in incoming request until buffer is full, or no more data, or end of line,
	//  whatever comes first.
	while (contData.client.available() && (inputOffset < COUNTOF(inputFileName) - 1))
	{
		inputFileName[inputOffset] = contData.client.read();

		if (inputFileName[inputOffset] == '\n')
			break;

		inputOffset++;
	}

	inputFileName[inputOffset + 1] = '\0';

	if (inputFileName[inputOffset] != '\n')
	{
		//  If we never saw the line ending then punt
#ifndef YAAWS_404_THE_ONE_TRUE_ERROR
		Return414UriTooLong();
#else
		Return404(inputFileName);
#endif
		return;
	}

	//  Look for the "HTTP" marker.  URL is encoded, so marker should never be in the URL.
	char *end = strstr_P(inputFileName, PSTR(" HTTP"));

	if (end == nullptr)
	{
#ifndef YAAWS_404_THE_ONE_TRUE_ERROR
		Return400BadRequest();
#else
		Return404(inputFileName);
#endif
		return;
	}

	//  Re-terminate the request at the marker.  Now all we have is a NUL terminated URI.
	*end = '\0';

	//  Determines the type of the request, then moves the URI down.  This appends it to
	//  the webroot we initialized with (above), and gives us the filename.
	RequestType rt = GetRequestType(pRequestStart);

	//  A 'HEAD' request is just a 'GET' without the actual payload.  In that case set the
	//  file position to the end of the file, normal processing takes care of the rest.
	bool skipFileData = false;

	switch (rt)
	{
	case rtGet:
		break;

	case rtHead:
		skipFileData = true;
		break;

#ifndef YAAWS_GET_IS_ALL_WE_NEED
	case rtPost:
		break;
#endif

	default:
		TRACE(F("Unknown request type"));
#ifndef YAAWS_404_THE_ONE_TRUE_ERROR
		Return405MethodNotAllowed();
#else
		Return404(inputFileName);
#endif
		return;

	}

#ifndef YAAWS_GET_IS_ALL_WE_NEED
	//  The end of the HTML request header is marked by the 4 character sequence
	//  "\r\n\r\n".  Data after that may be needed for processing (e.g., POST requests)
	//  We also want the 'Content-Length' value if there is one.
	const char *pContentLengthMatch = contentLengthMarker;
	bool fGetLength = false;
	unsigned long contentLength = 0;

	if (rt == rtPost)
	{
		int HeaderMarker = 0;

		while (contData.client.available() && HeaderMarker != 4)
		{
			byte c = contData.client.read();

			switch (HeaderMarker)
			{
			case 0:
			case 2:
				if (c == '\r') HeaderMarker++; else HeaderMarker = 0;
				break;

			case 1:
			case 3:
				if (c == '\n') HeaderMarker++; else HeaderMarker = 0;
				break;
			}

			//  If we found the the 'Content-length' marker, then the following digits
			//  define the length.
			if (fGetLength)
			{
				if (isdigit(c))
				{
					contentLength = contentLength * 10 + (c - '0');
				}
				else
				{
					//  All done
					fGetLength = false;
				}
			}

			//  Spec says case-insensitive!
			byte l = tolower(c);

			//  Keep track of how far into the marker we've matched.
			if (l == pgm_read_byte(pContentLengthMatch))
			{
				pContentLengthMatch++;

				//  If we've reached the end of the string, then we can read the length
				//  value (above).
				if (pgm_read_byte(pContentLengthMatch) == '\0')
				{
					fGetLength = true;
				}
			}
			else
			{
				//  No match, start over...
				pContentLengthMatch = contentLengthMarker;

				//  ... but see if we've started a new match with this character
				if (l == pgm_read_byte(pContentLengthMatch))
				{
					pContentLengthMatch++;
				}
			}

		}

		if (HeaderMarker != 4)
		{
#ifndef YAAWS_404_THE_ONE_TRUE_ERROR
			Return400BadRequest();
#else
			Return404(inputFileName);
#endif
			return;
		}
	}
#endif

	char *FormDataString = strchr(inputFileName, '?');

	if (FormDataString != nullptr)
	{
		//  Split the string into the filename and the form data.
		*FormDataString = '\0';
		FormDataString++;

		//  Input buffer now has a NUL terminated filename, then a NUL terminated string
		//  of form data.  

		//  No sense 'processing' an empty query string.
		if (*FormDataString == '\0')
		{
			FormDataString = nullptr;
		}
		else
		{
			IF_TRACE(Serial.println(FormDataString));
		}
	}

	//  Only decode the filename!  If you decode the query string, it may then contain
	//  extra '=' or '&' characters, which will hopelessly mess up parsing the query
	//  string.
	urldecode2(inputFileName);

	if (inputFileName[strlen(inputFileName) - 1] == '/')
	{
		//  Path but no filename, use default
		TRACE(F("Adding default filename"));

		strcat_P(inputFileName, PSTR("index.html"));

		//  We might have stomped over form data.  New rule! - default files can't have
		//  form data!
		FormDataString = nullptr;
	}

	TRACE(F("Requested file:"));
	IF_TRACE(quotedTrace(inputFileName));

	if (!contData.sdFile.open(inputFileName, O_READ))
	{
		{
			TRACE(F("Unknown file"));

			//  Unknown file or filetype
			Return404(inputFileName);
			return;
		}
	}

#ifndef YAAWS_NOTHING_EVER_CHANGES
	//  You can apply 'FileAction' only to mutable files.  We supply the URL path, NOT the
	//  full file-system path.
	contData.doFileAction =
		!contData.sdFile.isReadOnly() && _callback.IsMutable(pRequestStart);
#endif

	//  Determine 'Content-type' of the file.
	contData.rt = GetResponseType(inputFileName);

	if (skipFileData)
	{
		//  Implement HEAD by seeking immediately to the end of the file.
		contData.sdFile.seekEnd();
	}

	//  Process form data.  We assume that only 'GET' requests have data in the request
	//  header that needs to be processed.
	if ((rt == rtGet) && (FormDataString != nullptr))
	{
		TRACE(F("Form data:"));

		IF_TRACE(Serial.println(FormDataString));
		if (!_callback.ProcessFormData(pRequestStart, FormDataString))
		{
#ifndef YAAWS_404_THE_ONE_TRUE_ERROR
			Return400BadRequest();
#else
			Return404(inputFileName);
#endif
			return;
		}
	}

#ifndef YAAWS_GET_IS_ALL_WE_NEED
	if (rt == rtPost)
	{
		if (!_callback.ProcessPostData(pRequestStart, contData.client, contentLength))
		{
#ifndef YAAWS_404_THE_ONE_TRUE_ERROR
			Return400BadRequest();
#else
			Return404(inputFileName);
#endif
			return;
		}
	}
#endif

	//  We've processed any form data.  More calls to 'ServiceWebServer' will send the
	//  HTTP Response Header and the file back.
}




//  Move to the next active connection, to be serviced on the next call.
void YAAWS::AdvanceServiceIndex()
{
#ifndef YAAWS_ONE_STREAM_ONLY
	if ((_activeConnections != 0))
	{
		//  No need to look for the 'next' connection if this one is the only one.
		if (_activeConnections != (1 << _serviceIndex))
		{
			for (size_t i = 0; i < MAX_CLIENTS; i++)
			{
				_serviceIndex++;
				_serviceIndex %= MAX_CLIENTS;

				if (_activeConnections & (1 << _serviceIndex))
				{
					return;
				}
			}
		}
	}
	else
	{
		//  No active connections.
		_serviceIndex = 0;
	}
#endif
}


const char *
YAAWS::GetWebRoot()
{
	return (const char *)(_webRoot ? _webRoot : PSTR("/WWW"));
}


void YAAWS::ServiceWebServer(void)
{
#ifndef YAAWS_ONE_STREAM_ONLY
	if ((_activeConnections & clientsMask) != clientsMask)
	{
		//  At least one unused connection exists
		for (size_t i = 0; i < MAX_CLIENTS; i++)
		{
			if (!(_activeConnections & (1 << i)))
			{
				ContinuationData &contData = _contData[i];

				contData.client = _server.accept();

				if (contData.client.connected())
				{
					//  If there are no other active connections, make this one the next
					//  to be serviced.
					if (_activeConnections == 0)
					{
						_serviceIndex = i;
					}

					//  Mark connection as active.
					_activeConnections |= (1 << i);
					contData.rt = UNKNOWN;
#ifndef YAAWS_NOTHING_EVER_CHANGES
					contData.doFileAction = true;
#endif
				}
				else
				{
					//  No need to continue looking for incoming connections.
					break;
				}
			}
		}
	}
#else  //  Only one stream
	if (_activeConnections == 0)
	{
		ContinuationData &contData = _contData[_serviceIndex];
		contData.client = _server.accept();

		if (contData.client.connected())
		{
			_activeConnections |= (1 << _serviceIndex);
			contData.rt = UNKNOWN;
#ifndef YAAWS_NOTHING_EVER_CHANGES
			contData.doFileAction = true;
#endif
		}
	}
#endif

	if (_activeConnections & (1 << _serviceIndex))
	{
		ContinuationData &contData = _contData[_serviceIndex];

		if (!contData.client.connected())
		{
			TRACE(F("Connection unexpectedly closed"));

			FinishConnection();
		}
		else
		{
			if (contData.rt == UNKNOWN)
			{
				AcceptIncoming();
			}
			else
			{
				ContinueRequest();
			}
		}
	}

	AdvanceServiceIndex();
}



