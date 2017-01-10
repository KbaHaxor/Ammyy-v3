#ifndef _AA_PROTOCOL_H__INCLUDED_
#define _AA_PROTOCOL_H__INCLUDED_


#pragma pack(push, 1)
struct UINT24
{
	UINT24() {}
	
	UINT24(UINT32 _c) 
	{
		*this = _c; //call operator =
	}	

	//__forceinline operator UINT32() const { return (c1<<16) | (c2<<8) | c3; }
	
	__forceinline UINT24& operator = (const UINT32 _c)
	{
		const UINT8* c = (const UINT8*)&_c;
		*((UINT16*)&d[0]) = *((UINT16*)c+0);	//d[0] = c[0]; d[1] = c[1];
		*((UINT8 *)&d[2]) = *((UINT8 *)c+2);	//d[2] = c[2];
		return *this;
	}	
	
	__forceinline operator UINT32() const
	{
		UINT32 r=0;
		UINT8* c = (UINT8*)&r;
		*((UINT16*)c+0) = *((UINT16*)&d[0]);	//c[0] = d[0]; c[1] = d[1];
		*((UINT8 *)c+2) = *((UINT8 *)&d[2]); 	//c[2] = d[2];
		return r;
	}

	__forceinline bool operator ==(const UINT24& v2) const
	{
		if (*((WORD*)&d[0]) != *((WORD*)&v2.d[0])) return false;
		return  (d[2]==v2.d[2]);
	}

	__forceinline bool operator!=(const UINT24& v2) { return  (!(*this==v2)); }
	//bool __forceinline operator!=(const UINT24& v1, const UINT24& v2) { return  (!(v1==v2)); }


private:
	char d[3];
};


/*
 * Once the initial handshaking is done, all messages start with a type byte,
 * (usually) followed by message-specific data.  The order of definitions in
 * this file is as follows:
 *
 *  (3) Message types.
 *  (4) Encoding types.
 *  (5) For each message type, the form of the data following the type byte.
 *      Sometimes this is defined by a single structure but the more complex
 *      messages have to be explained by comments.
 */




/*****************************************************************************
 *
 * Initial handshaking messages
 *
 *****************************************************************************/


#define aaPreInitMsg_v2   "+"
#define aaPreInitMsg_v3   "="
#define aaPreInitReply "-"


/*-----------------------------------------------------------------------------
 * Protocol Version
 *
 * The format string below can be used in sprintf or sscanf to generate or decode the version string respectively.
 */

#define aaProtocolVersionFormat_2_1 "AA%s\n%u\n%u\n%u\n"		//	version, query_type, local_id, remote_id

#define sz_aaProtocolVersionMsg_2_1 36

#define aaTypeTarget 1
#define aaTypeViewer 2

enum RouterReply
{
	Router_Connected            = 0,
	Router_ComputerNotFound     = 1,
	Router_ComputerIsBusy       = 2,
	Router_ComputerOtherVersion = 3,
	Router_Denied				= 4,
	Router_PasswordIncorrect	= 5,
};



/*****************************************************************************
 *
 * Message types
 *
 *****************************************************************************/


enum aaCommand
{
	// viewer <-> target    both ways
	aaNop		  = 10,
	aaPingRequest = 11,		// uses by Router also
	aaPingReply   = 12,		// uses by Router also
	aaSound		  = 13,
	aaCutText	  = 14,

	// target -> viewer
	aaScreenUpdate        = 21,
	aaSetColourMapEntries = 22,	
	aaPointerMove         = 23,
	aaError               = 24,
	aaDesktopUnavailable  = 25,

	// viewer -> target
	aaSetEncoder		  = 40,
	aaDesktopOFF		  = 41,
	aaSetPointer		  = 42,
	aaScreenUpdateRequest = 43,	// full screen update request
	aaScreenUpdateCommit  = 44,
	aaKeyEvent            = 45,
	aaPointerEvent        = 46,
	aaRDP                 = 47,
	aaDirectConnect		  = 48,
	aaSpeedTest			  = 49,

	// FileManager: viewer -> target
	aaFileListRequest     = 60,
	aaFolderCreateRequest = 61,
	aaRenameRequest		  =	62,
	aaDeleteRequest		  =	63,
	aaDnloadRequest		  =	64,
	aaUploadRequest		  =	65,
	aaUploadData		  =	66,
	aaUploadDataLast	  =	67,
	aaDnloadDataAck		  =	68,

	// FileManager: target -> viewer
	aaFmReply			  =	70,
	aaUploadDataAck		  =	71,
	aaDnloadData		  =	72,
	aaDnloadDataLast	  =	73,
};


// viewer -> target ONLY!
#define aaSoundInit  0xFFFE
#define aaSoundClose 0xFFFF




//-----------------------------------------------------------------------------


#define aaKey01 "~@sa78"

static UINT8 aaKey02[21] = { 0x45,0xB1,0x87,0xA6,0x98,0x40,0x7E,0x14,0x91,0xC6,0xD4,0x97,0xBD,0x70,0x95,0x5A,0xA2,0x75,0xAA,0xF0,0};

struct aaInitMsg {
	UINT16 random;
	UINT16 zero;
	UINT32 version;
	UINT8  type;
	UINT32 id;
	UINT32 remote_id; // or pint_time for aaTypeTarget
};

struct aaRouterInfoMsg {
	UINT16 random;	// just random 2 bytes, to make good encryption
	UINT64 time;
	UINT16 tcp_port;
	UINT32 ip;
};



struct aaTerminateMsg {
    UINT8  type;  /* always aaError */
    UINT16 code;
};

enum aaTerminateMsgType
{
	aaErrorNone = 0,

	// Router_ComputerIsBusy
	
	aaErrorAccessRejected  = 10,
	aaErrorInternalError   = 11,	// internal error was occur
	aaErrorSessionInactive = 12,	// session is out of console	
	aaErrorSessionEnd      = 13,	// Windows session is ended !
	aaCloseSession         = 14,	// no error user just want to close session
	aaErrorRDPServer       = 15,
	aaPasswordRequired     = 16,	// not error
	aaDesktopInitError     = 17,
};

enum aaDRF // flag for aaDnloadRequest
{
	aaDRF_TryToContinue = 1,
	aaDRF_Cancel        = 2,
	aaDRF_GetTime       = 4,
};

enum aaURF // flag for aaUploadRequest
{
	aaURF_Overwrite = 1,
	aaURF_TryToContinue = 2,
	aaURF_SetTime       = 4,

};

enum aaDirectConnection
{
	aaDcWaitTCP = 1,
	aaDcConnect = 2,
	aaDcExit = 3,
	aaDcPing = 4,
	aaDcPingReply = 5,
	aaDcOK = 100,
	aaDcFailed = 101,
};

enum aaSpeedTest
{
	aaStExit = 1,
	aaStPing = 2,
	aaStPingReply = 3,
	aaStDownload = 4,
	aaStDownloadData = 5,
	aaStDownloadFinished = 6,
	aaStUpload = 7,
	aaStUploadData = 8,
	aaStUploadFinished = 9,
};

enum aaAuthorization
{
	aaAuthorizationOK = 0,
	aaAuthorizationNeedPassword = 1,
};

#pragma pack(pop)

#endif // _AA_PROTOCOL_H__INCLUDED_
