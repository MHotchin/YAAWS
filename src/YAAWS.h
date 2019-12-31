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

#ifndef _YAAWS_h
#define _YAAWS_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif


#ifndef SdFat_h
#pragma GCC warning "Include 'SdFat.h' before including YAAWS.h"
#include <SdFat.h>
#endif

#ifndef ethernet_h_
#pragma GCC warning "Include 'Ethernet.h' before including YAAWS.h"
#include <Ethernet.h>
#endif

//  You can control how much functionality (and hence memory use) you have using the
//  #defines below.  By default, we provide all functionality.
//
//  Uncomment 'YAAWS_LEAN_AND_MEAN' for the bare minimum useful functionality
//  - GET and HEAD requests.
//  - GET style form handling (URL query strings)
//  - One connection at a time.
//  - pretty much all errors are just 404s.
//
//  Saves some memory, and about 1K code.  Works great for small static sites, using at
//  most simple forms.
//
//  Commented out, you get more functionality (in particular, POST style form processing)
//  better error messages, and more simultaneous connections.

// #define YAAWS_LEAN_AND_MEAN

//  You can enable these savings individually if you like.
#ifdef YAAWS_LEAN_AND_MEAN
#define YAAWS_ONE_STREAM_ONLY        //  Use only one connection
#define YAAWS_404_THE_ONE_TRUE_ERROR //  All errors reported as 404
#define YAAWS_GET_IS_ALL_WE_NEED     //  GET and HEAD support only
#define YAAWS_NOTHING_EVER_CHANGES   //  All files are immutable.
#define YAAWS_NO_FLASHY_FLASHY       //  Don't flash built-in LED on activity
#endif

//  This is a development switch.  You can comment it out if you like to see what the
//  web-server is doing.
#define YAAWS_HUSH_NOW               //  Disable internal tracing

// Uncomment these for individual savings.  The biggest RAM saving comes from enabling
// YAAWS_ONE_STREAM_ONLY.  The other save mostly flash (PROGMEM) memory, either from code
// savings or having fewer PROGMEM strings

// #define YAAWS_ONE_STREAM_ONLY			//  Use only one connection 
// #define YAAWS_404_THE_ONE_TRUE_ERROR		//  All errors reported as 404
// #define YAAWS_GET_IS_ALL_WE_NEED			//  GET and HEAD support only
// #define YAAWS_NOTHING_EVER_CHANGES		//  All files are immutable.
// #define YAAWS_NO_FLASHY_FLASHY			//  Don't flash built-in LED on activity

//  Web server will use this for its files.
typedef SdFile WebFileType;


//
//  Implements a simple web-server. You provide a SdFat object (containing the files for
//  the website), and an optional callback class to handle certain web-page events.
//
//  In your 'loop()' function, call 'ServiceWebServer'.  Each time it is called, it
//  checks for incoming requests, and start servicing the first one it receives.
//  Subsequent calls will continue to service the active requests - several calls may be
//  required to complete any given request.  Although this increases latency for any given
//  request, it reduces how much time each call takes.
//
//  Most calls that are servicing active requests should return in less than 10
//  milliseconds.  When idle, there is almost no overhead.
//
//  The webserver will satisfy file requests from the directory 'WWW' located at the root
//  of the SdCard object (by default, constructor can over-ride) .  The website on disk
//  may have any directory structure, and long filenames are supported by the current
//  version of SdFat.
//

class YaawsCallback;

typedef SdFileSystem<SdSpiCard> webSdCard;

class YAAWS
{
public:

	//  If you provide a 'webRoot' string, it MUST be declared in PROGMEM, e.g.  like this
	//  - PSTR("/WebStuff").  Default values are '/WWW' as the web root, and port 80.
	YAAWS(webSdCard &SdCard,
		  const char *webRoot = nullptr, uint16_t port = 80);

	YAAWS(webSdCard &SdCard, YaawsCallback &callback,
		  const char *webRoot = nullptr, uint16_t port = 80);

	//  Initialize the web server.  If this returns false, check that your Ethernet and SD
	//  card are working properly.
	bool begin();

	//  Call whenever you can to receive and service requests.  Typical call time is up to
	//  about 10 milliseconds on a 2650, *if* there is work to do.  When idle there is
	//  alomost no overhead.
	void ServiceWebServer();

	enum ResponseType : byte;
private:


	ResponseType GetResponseType(const char *filename);
	void SendResponseHeader();
	void FinishConnection();

	void SendSdFile();
	void ContinueRequest();

	void Return404(char *fileNameBuffer);
#ifndef YAAWS_404_THE_ONE_TRUE_ERROR
	void Return400BadRequest();
	void Return405MethodNotAllowed();
	void Return414UriTooLong();
#endif
	void AcceptIncoming();
	void AdvanceServiceIndex();
	const char *GetWebRoot();


	EthernetServer _server;
	webSdCard &_SdCard;
	YaawsCallback &_callback;
	const char *_webRoot;

#ifndef YAAWS_ONE_STREAM_ONLY
	//  4 works on all 5X00 chips.  5500 might support more, but do you really need to?
	static constexpr size_t MAX_CLIENTS = 4;
#else
	static constexpr size_t MAX_CLIENTS = 1;
#endif
	//  Data we need to allow the request to be 'continued'.  In particular, the file is
	//  served in numerous chunks, one at each call to 'ServiceWebServer'.
	struct ContinuationData
	{
		EthernetClient client;  // Connection to the client (requestor)
		WebFileType sdFile;     // File to be returned.
		ResponseType rt;        // 'Content-type' of the file.
#ifndef YAAWS_NOTHING_EVER_CHANGES
		bool doFileAction;      //  Do we need to continue calling FileAction()
#endif
	};

	static constexpr byte clientsMask = (1 << MAX_CLIENTS) - 1;
	ContinuationData _contData[MAX_CLIENTS];
	byte _activeConnections;    //  Bit mask showing which connections are active.
#ifndef YAAWS_ONE_STREAM_ONLY
	byte _serviceIndex;         //  Connection we are servicing
#else
	static constexpr byte _serviceIndex = 0;
#endif

};


//  Request processing is split into 4 phases, two of which are provided by the callback.
//
//  First, if the request has any form submission parameters, these are passed to the
//  callback to be processed.  
//  Second, depending of the extension of the file requested, an HTML Response header is
//  generated and send back to the requester.  
//  Third, the callback may be given the opportunity process or modify the requested file.
//
//  Finally, the remaining part of the file (usually all of it) is sent back to the
//  requestor in chunks.
//
//  *Optional* callback so you can respond to certain website events.  You would derive
//  from this if you want to process form data, or want to make dynamic HTML pages.
//
//  'ProcessFormData' is called when a form is submitted with a method of 'GET',
//  'ProcessPostData' is called when a form is submitted using 'POST'.  'GET' data is
//  passed directly, 'POST' data is read by the callback from the 'EthernetClient' object
//  provided.
//
//  'IsMutable' is used to determine which files are available for 'FileAction'.
//  Read-only files are *never* mutable.
//
//  'FileAction' is called after the HTML Response header has been sent, but before the
//  requested file, if the file is mutable (i.e., if 'IsMutable' returned true).
//
//  In 'FileAction', you can examine or even modify the response, but you MUST send any
//  part of the file you read and do not want to discard to the requestor yourself,
//  through the EthernetClient object.  If you return 'true', you will continue to be
//  called until you return 'false', so you don't have to perform all your processing in
//  one go. Once you return 'false', the webserver will handle sending any remaining part
//  of the file, and you will not be called again for that request.
//

class YaawsCallback
{
public:

	//  For form data you get the path to the file it was submitted to, and the query
	//  string (still URL encoded).  You can use 'getNextQueryPair' (below) to split the
	//  query string into name / value pairs.  
	//
	//  Return 'true' to allow the requested file to be sent back to the client, return
	//  'false' to report HTTP Error 400 (Bad Request) to the client.
	//
	//  Default accepts all input, does nothing, and return 'true'.
	virtual bool ProcessFormData(const char *path, char *FormData);

#ifndef YAAWS_GET_IS_ALL_WE_NEED
	//  Similar to ProcessFormData, except for 'POST' requests.  The default action is to
	//  read the POST parameters into a buffer, then pass them to 'ProcessFormData',
	//  above.  Override this if your post data is too long for the default handler.
	virtual bool ProcessPostData(const char *path, EthernetClient &client,
								 unsigned long contentLength);
#endif

	//  If you want dynamic HTML, this is the place for you.  The first function is called
	//  by the web server to detemine what files might be changable.  In order to be
	//  changable, the file must *not* be read-only, and 'IsMutable' must return true.  In
	//  that case, 'FileAction' (below) will be called before the file is returned to the
	//  client.
#ifndef YAAWS_NOTHING_EVER_CHANGES
	virtual bool IsMutable(const char *path);

	virtual bool FileAction(EthernetClient &client, WebFileType &file);
#endif

protected:
	//  Some utility functions you might find useful.  Declared in the base class like
	//  this makes them available to any derived class.

	struct queryPair
	{
		const char *_name;
		const char *_value;
	};

	//  Breaks apart a query string into name / value pairs.  Initially, call with the
	//  original query string.  Use the return value on subsequent invokations to move
	//  along the query string.  You're done when it return 'nullptr'.
	//
	//  The 'queryPair' structure will have pointers to both the name and the value
	//  components.  Each will be already URL decoded.
	//  - if the value is 'nullptr', then the original query string didn't have a value
	//    for that name.
	//  - the value can also be a zero length string.
	static char *getNextQueryPair(char *queryString, queryPair &nameValuePair);

	//  Decodes a URL encoded string in place.  A decoded string is *never* longer than
	//  the original string, so we don't need to know how long the buffer is.
	static void urlDecode(char *encodedString);
};

#endif

