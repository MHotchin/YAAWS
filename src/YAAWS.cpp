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

	void printRam()
	{
		Serial.print(F("Free mem: "));
		Serial.println(freeRam());
	}

	void quotedTrace(const char *p)
	{
		Serial.print(F("\""));
		Serial.print(p);
		Serial.println(F("\""));
	}
#else
#define TRACE(X) (void)0
#define IF_TRACE(X) (void)0

	void printRam()
	{}

	void quotedTrace(const char *)
	{}

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
}

//  Number of items in an array
#define COUNTOF(x) (sizeof(x)/sizeof(x[0]))

void YaawsCallback::ProcessFormData(
	const char *path, char *formData)
{}


#ifndef YAAWS_GET_IS_ALL_WE_NEED
void YaawsCallback::ProcessPostData(
	const char *path, EthernetClient &)
{}
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


YaawsCallback defaultCallback;

//  Different types of HTML Response headers supported.
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
	//  Text for all the different HTML Response headers. Except for 404, all
	//  the rest differ only by the value of the 'Content-Type' directive.
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




//  If this returns false, your webserver won't be working.  Check that your
//  ethernet and SD Card are working.
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

	// Use the file extension to decide the 'Content-type' in the HTML
	// response header.
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



//  Send the first part of the response - the HTML Response Header.  The
//  'Content-Type' directive is determined by the file's extension.
void YAAWS::SendResponseHeader()
{
	TRACE(F("SendResponseHeader"));
	constexpr size_t buffSize = 256;
	char buffer[buffSize + 1] = {0};
	ContinuationData &contData = _contData[_serviceIndex];


	//  404 is one piece, all others are made up of three pieces.
	if (_contData[_serviceIndex].rt != htm404)
	{
		strncpy_P(buffer, str200Header, buffSize);
	}

	strncat_P(buffer, pgm_read_ptr(&aResponses[_contData[_serviceIndex].rt]), buffSize);

	if (_contData[_serviceIndex].rt != htm404)
	{
		strncat_P(buffer, PSTR("\n"), buffSize);

		bool isCacheable = _contData[_serviceIndex].sdFile.isReadOnly();

		if (isCacheable)
		{
			strncat_P(buffer, strCacheable, buffSize);
		}
		else
		{
			strncat_P(buffer, strNonCacheable, buffSize);
		}

		//  If the file is not mutable, we can provide a Content-length:
		//  directive and an 'ETag' for cache validation.  Only mutable files
		//  will have this set at this point.
#ifndef YAAWS_NOTHING_EVER_CHANGES
		if (!contData.doFileAction)
#endif
		{
			strncat_P(buffer, PSTR("Content-length: "), buffSize);
			ltoa(_contData[_serviceIndex].sdFile.fileSize(), buffer + strlen(buffer), 10);
			strncat_P(buffer, PSTR("\n"), buffSize);


			//  TODO - Etag validation not yet implemented.
#ifndef YAAWS_NO_CACHE_FOR_YOU
			//strncat_P(buffer, PSTR("ETag: \""), buffSize);
			//ltoa(_contData[_serviceIndex].sdFile.fileSize(), buffer + strlen(buffer), 16);
			//strncat_P(buffer, PSTR("\"\n"), buffSize);
#endif
		}

		strncat_P(buffer, PSTR("\n"), buffSize);
	}

	buffer[buffSize] = '\0';

	FlashyFlashy ff;
	_contData[_serviceIndex].client.write(buffer);

	return;
}


//  Clean up our connection once we are done with it, whatever the reason.  Flush the data, mark the
//  connection as available
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


//  Send the actual file to the requestor.  Will return before sending the whole
//  file, called repeatedly to keep things going.
void YAAWS::SendSdFile()
{
	ContinuationData &contData = _contData[_serviceIndex];
	int amountToWrite = contData.client.availableForWrite();

	if (amountToWrite > 0)
	{
		//  Maximum we will write in one go. If we have only one client, use as
		//  much of the undlying Ethernet frame size as possible.  Otherwise,
		//  limit to a multiple of the SD sector size to reduce SD card
		//  redundant reads. 
		constexpr int maxBufferSize = (MAX_CLIENTS == 1) ? 1400 : 1024;

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

// In general, we don't want Service calls to take *too* long. If a request is waiting, then the
// first call will receive it, next will send back the response header. After that, each call will
// transmit part of the response file.
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


//  Uses 'filename' as a caller provided buffer for building the filename.  Reduces max stack usage
//  by re-using already allocated space.  We'll just assume things fit.
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
	const char strPost[] PROGMEM = "POST /";

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

		for (auto i = 0; i < COUNTOF(aRequests); i++)
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
}


//  Initial processing of any request:
//   - Read the request from the client.
//   - Validate request type
//   - Validate file to return
//   - Process any form data
void YAAWS::AcceptIncoming()
{
	// Everything we need is on the first line, before the 'HTTP' keyword. Once we've
	// located the keyword, we can throw everything else away.
	constexpr size_t buffSize = 128;
	char inputFileName[buffSize + 1] = {0};

	inputFileName[buffSize] = '\0';

	ContinuationData &contData = _contData[_serviceIndex];

	//  Well isn't THAT special.  I'm seeing cases where the connection
	//  does not arrive with the payload.  If you wait long enough, it
	//  seems to show up.  Up to 30 ms seems possible.
	if ((contData.rt == UNKNOWN) && !contData.client.available())
	{
		//  There's no timeout on this - we will wait as long as the connection is held open.
		TRACE(F("Awaiting incoming payload"));
		return;
	}


	//  Start building the filename to be returned.
	strcpy_P(inputFileName, GetWebRoot());

	size_t inputOffset = strlen(inputFileName);
	char *pRequestStart = inputFileName + inputOffset;

	//  Read in incoming request until buffer is full, or no more data, or end of
	//  line, whatever comes first.
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

	//  Look for the "HTTP" marker.  URL is encoded, so marker should never be
	//  in the URL.
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


	//  Determines the type of the request, then moves the URI down.  This appends it to the webroot
	//  we initialized with (above), and gives us the filename.
	RequestType rt = GetRequestType(pRequestStart);

	//  A 'HEAD' request is just a 'GET' without the actual payload.  In that case set the file
	//  position to the end of the file, normal processing takes care of the rest.
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
	//  "\r\n\r\n".  Data after that may be needed for processing (e.g., POST
	//  requests)
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

		//  Input buffer now has a NUL terminated filename, then a NUL terminated string of form
		//  data.  

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


	if (inputFileName[strlen(inputFileName) - 1] == '/')
	{
		//  Path but no filename, use default
		TRACE(F("Adding default filename"));

		strcat_P(inputFileName, PSTR("index.html"));

		//  We might have stomped over form data.  New rule! - default files can't have form data!
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
	//  You can apply FileAction only to mutable files.
	contData.doFileAction =
		!contData.sdFile.isReadOnly() && _callback.IsMutable(inputFileName);
#endif

	//  Determine 'Content-type' of the file.
	contData.rt = GetResponseType(inputFileName);

	if (skipFileData)
	{
		//  Implement HEAD by seeking immediately to the end of the file.
		contData.sdFile.seekEnd();
	}

	//  Process form data.  We assume that only 'GET' requests have data in the request header that
	//  needs to be processed.
	if (rt == rtGet && FormDataString != nullptr)
	{
		TRACE(F("Form data:"));

		IF_TRACE(Serial.println(FormDataString));
		_callback.ProcessFormData(inputFileName, FormDataString);
	}

#ifndef YAAWS_GET_IS_ALL_WE_NEED
	if (rt == rtPost)
	{
		//  TODO - do we need the value of the 'Content-length' header to do this reliably?
		_callback.ProcessPostData(inputFileName, contData.client);
	}
#endif

	//  We've processed any form data.  More calls to 'ServiceWebServer' will send the HTTP Response
	//  Header and the file back.
}




//  Move to the next active connection, to be serviced on the next call.
void YAAWS::AdvanceServiceIndex()
{
#ifndef YAAWS_ONE_STREAM_ONLY
	if ((_activeConnections != 0))
	{
		//  No need to look for the 'next' connection if this one is the only
		//  one.
		if (_activeConnections != (1 << _serviceIndex))
		{
			for (auto i = 0; i < MAX_CLIENTS; i++)
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
		for (auto i = 0; i < MAX_CLIENTS; i++)
		{
			if (!(_activeConnections & (1 << i)))
			{
				ContinuationData &contData = _contData[i];

				contData.client = _server.accept();

				if (contData.client.connected())
				{
					//contData.client.setConnectionTimeout(100);

					//  If there are no other active connections, make this one
					//  the next to be serviced.
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

	if (_activeConnections && (1 << _serviceIndex))
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



