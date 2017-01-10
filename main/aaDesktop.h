#ifndef _AA_DESKTOP_H__INCLUDED_
#define _AA_DESKTOP_H__INCLUDED_


#pragma pack(push, 1)

/*-----------------------------------------------------------------------------
 * Structure used to specify a rectangle.  This structure is a multiple of 4
 * bytes so that it can be interspersed with 32-bit pixel data without affecting alignment.
 */

struct aaRectangle {
    UINT16 x;
    UINT16 y;
    UINT16 w;
    UINT16 h;
};


/*-----------------------------------------------------------------------------
 * Structure used to specify pixel format.
 */

enum aaPixelFormatType
{
	aaTrueColor = 0,
	aaGrayScale = 2,
};

struct aaPixelFormat 
{
public:
    UINT8 bitsPerPixel;		/* 8,16,24, 32 only */
	UINT8 type;				// 0 - true color, 1 - pallete, 2 - 8-bit grayscale
    // If 'true color' then xxxMax and xxxShift specify bits used for red, green and blue

    // maximum red, blue, green value (= 2^n - 1 where n is the number of bits used for red, blue, green).
	// Note this value is always in big endian order.

	UINT16 rMax;
    UINT16 gMax;
    UINT16 bMax;

	//  number of shifts needed to get the [red, green, blue] value in a pixel to the least significant bit
	//	To find the red value from a given pixel, do the following:
	//   1) Swap pixel value according to bigEndian (e.g. if bigEndian is false and host byte order is big endian, then swap).
	//   2) Shift right by rShift.
	//	 3) AND with rMax (in host byte order).
	//   4) You now have the red value between 0 and rMax.

	UINT8 rShift;
    UINT8 gShift;
    UINT8 bShift;

	aaPixelFormat() {}

	//constructor for truecolor
	aaPixelFormat(UINT8 bpp, UINT8 _type, UINT16 _rMax, UINT16 _gMax, UINT16 _bMax, UINT8 _rShift, UINT8 _gShift, UINT8 _bShift)
	{
		bitsPerPixel = bpp;
		type = _type;
		rMax = _rMax;
		gMax = _gMax;
		bMax = _bMax;
		rShift = _rShift;
		gShift = _gShift;
		bShift = _bShift;
	}

	bool inline IsTrueColor() { return type==aaTrueColor; }
	bool inline IsGrayScale() { return type==aaGrayScale; }

	bool operator==(const aaPixelFormat& f2) const
	{
		if (this->bitsPerPixel != f2.bitsPerPixel || this->type != f2.type) return false;

		return (this->rMax   == f2.rMax   && this->gMax   == f2.gMax   && this->bMax   == f2.bMax &&
				this->rShift == f2.rShift && this->gShift == f2.gShift && this->bShift == f2.bShift);
	}
};

/*
bool operator==(const aaPixelFormat& f1, const aaPixelFormat& f2)
{
	if(	 f1.bitsPerPixel != f2.bitsPerPixel || f1.trueColour != f2.trueColour) return false;

	return (f1.rMax   == f2.rMax   && f1.gMax   == f2.gMax   && f1.bMax   == f2.bMax &&
			f1.rShift == f2.rShift && f1.gShift == f2.gShift && f1.bShift == f2.bShift);
}
*/

/*****************************************************************************
 *
 * Encoder types
 *
 *****************************************************************************/

enum aaEncoderDesktopEnum
{
	aaEncoderChangedEncoder = 1,
	aaEncoderChangedSize = 2,
};

enum aaEncoder
{
	aaEncoderRaw       = 1,
	aaEncoderCopyRect  = 2,
	aaEncoderAAFC	   = 3, // like Hextile, but a little more efficinty
	aaEncoderAAC	   = 4,
	//aaEncoderRRE       = 5,
	//aaEncoderCoRRE     = 6,
	//aaEncoderZlib      = 7,
	//aaEncoderTight     = 8,
	//aaEncoderZlibHex   = 9,
	//aaEncoderJpeg	   = 10,

	aaEncoderCursorMono    = 110,
	aaEncoderCursorRich    = 111,
	aaEncoderChanged       = 112, // desktop format, size or encoder was changed!

	aaEncoderLastRect      = 0xFF,
};


/*****************************************************************************
 *
 * Server -> client message definitions
 *
 *****************************************************************************/


/*-----------------------------------------------------------------------------
 * FramebufferUpdate - a block of rectangles to be copied to the framebuffer.
 *
 * This message consists of a header giving the number of rectangles of pixel
 * data followed by the rectangles themselves.
 */



/*
 * Each rectangle of pixel data consists of a header describing the position
 * and size of the rectangle and a type word describing the encoding of the
 * pixel data, followed finally by the pixel data.  Note that if the client has
 * not sent a SetEncodings message then it will only receive raw pixel data.
 */

struct aaFramebufferUpdateRectHeader {
	UINT8 encoder;		/* one of the encoder types rfbEncoding... */
    aaRectangle r;
};

#define sz_aaFramebufferUpdateRectHeader sizeof (aaFramebufferUpdateRectHeader)


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Raw Encoding.  Pixels are sent in top-to-bottom scanline order,
 * left-to-right within a scanline with no padding in between.
 */


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * CopyRect Encoding.  The pixels are specified simply by the x and y position of the source rectangle.
 */

struct aaCopyRect {
	UINT8 encoder;
	aaRectangle r;
    UINT16 srcX;
    UINT16 srcY;
};


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * RRE - Rise-and-Run-length Encoding.  We have an aaRREHeader structure
 * giving the number of subrectangles following.  Finally the data follows in
 * the form [<bgpixel><subrect><subrect>...] where each <subrect> is
 * [<pixel><aaRectangle>].
 */

struct aaRREHeader {
    UINT32 nSubrects;
};

#define sz_aaRREHeader sizeof(aaRREHeader)


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * CoRRE - Compact RRE Encoding.  We have an aaRREHeader structure giving
 * the number of subrectangles following.  Finally the data follows in the form
 * [<bgpixel><subrect><subrect>...] where each <subrect> is
 * [<pixel><aaCoRRERectangle>].  This means that
 * the whole rectangle must be at most 255x255 pixels.
 */

struct aaCoRRERectangle {
    UINT8 x;
    UINT8 y;
    UINT8 w;
    UINT8 h;
};

#define sz_aaCoRRERectangle sizeof(aaCoRRERectangle)


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Hextile Encoding.  The rectangle is divided up into "tiles" of 16x16 pixels,
 * starting at the top left going in left-to-right, top-to-bottom order.  If
 * the width of the rectangle is not an exact multiple of 16 then the width of
 * the last tile in each row will be correspondingly smaller.  Similarly if the
 * height is not an exact multiple of 16 then the height of each tile in the
 * final row will also be smaller.  Each tile begins with a "subencoding" type
 * byte, which is a mask made up of a number of bits.  If the Raw bit is set
 * then the other bits are irrelevant; w*h pixel values follow (where w and h
 * are the width and height of the tile).  Otherwise the tile is encoded in a
 * similar way to RRE, except that the position and size of each subrectangle
 * can be specified in just two bytes.  The other bits in the mask are as
 * follows:
 *
 * BackgroundSpecified - if set, a pixel value follows which specifies
 *    the background colour for this tile.  The first non-raw tile in a
 *    rectangle must have this bit set.  If this bit isn't set then the
 *    background is the same as the last tile.
 *
 * ForegroundSpecified - if set, a pixel value follows which specifies
 *    the foreground colour to be used for all subrectangles in this tile.
 *    If this bit is set then the SubrectsColoured bit must be zero.
 *
 * AnySubrects - if set, a single byte follows giving the number of
 *    subrectangles following.  If not set, there are no subrectangles (i.e.
 *    the whole tile is just solid background colour).
 *
 * SubrectsColoured - if set then each subrectangle is preceded by a pixel
 *    value giving the colour of that subrectangle.  If not set, all
 *    subrectangles are the same colour, the foreground colour;  if the
 *    ForegroundSpecified bit wasn't set then the foreground is the same as
 *    the last tile.
 *
 * The position and size of each subrectangle is specified in two bytes.  The
 * Pack macros below can be used to generate the two bytes from x, y, w, h,
 * and the Extract macros can be used to extract the x, y, w, h values from
 * the two bytes.
 */

#define aaHextileRaw			(1 << 0)
#define aaHextileBackgroundSpecified	(1 << 1)
#define aaHextileForegroundSpecified	(1 << 2)
#define aaHextileAnySubrects		(1 << 3)
#define aaHextileSubrectsColoured	(1 << 4)

#define aaHextilePackXY(x,y) (((x) << 4) | (y))
#define aaHextilePackWH(w,h) ((((w)-1) << 4) | ((h)-1))
//#define aaHextileExtractX(byte) ((byte) >> 4)
//#define aaHextileExtractY(byte) ((byte) & 0xf)
//#define aaHextileExtractW(byte) (((byte) >> 4) + 1)
//#define aaHextileExtractH(byte) (((byte) & 0xf) + 1)

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * ZLIB - zlib compression Encoding.  We have an aaZlibHeader structure
 * giving the number of bytes to follow.  Finally the data follows in zlib compressed format.
 */

struct aaZlibHeader {
    UINT32 nBytes;
};


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * JPEG - jpeg compression Encoding.
 */
struct aaJpegHeader {
	UINT32 nBytes;
};



/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Tight Encoding.
 *
 *-- The first byte of each Tight-encoded rectangle is a "compression control
 *   byte". Its format is as follows (bit 0 is the least significant one):
 *
 *   bit 0:    if 1, then compression stream 0 should be reset;
 *   bit 1:    if 1, then compression stream 1 should be reset;
 *   bit 2:    if 1, then compression stream 2 should be reset;
 *   bit 3:    if 1, then compression stream 3 should be reset;
 *   bits 7-4: if 1000 (0x08), then the compression type is "fill",
 *             if 1001 (0x09), then the compression type is "jpeg",
 *             if 0xxx, then the compression type is "basic",
 *             values greater than 1001 are not valid.
 *
 * If the compression type is "basic", then bits 6..4 of the
 * compression control byte (those xxx in 0xxx) specify the following:
 *
 *   bits 5-4:  decimal representation is the index of a particular zlib
 *              stream which should be used for decompressing the data;
 *   bit 6:     if 1, then a "filter id" byte is following this byte.
 *
 *-- The data that follows after the compression control byte described
 * above depends on the compression type ("fill", "jpeg" or "basic").
 *
 *-- If the compression type is "fill", then the only pixel value follows, in
 * client pixel format (see NOTE 1). This value applies to all pixels of the
 * rectangle.
 *
 *-- If the compression type is "jpeg", the following data stream looks like
 * this:
 *
 *   1..3 bytes:  data size (N) in compact representation;
 *   N bytes:     JPEG image.
 *
 * Data size is compactly represented in one, two or three bytes, according
 * to the following scheme:
 *
 *  0xxxxxxx                    (for values 0..127)
 *  1xxxxxxx 0yyyyyyy           (for values 128..16383)
 *  1xxxxxxx 1yyyyyyy zzzzzzzz  (for values 16384..4194303)
 *
 * Here each character denotes one bit, xxxxxxx are the least significant 7
 * bits of the value (bits 0-6), yyyyyyy are bits 7-13, and zzzzzzzz are the
 * most significant 8 bits (bits 14-21). For example, decimal value 10000
 * should be represented as two bytes: binary 10010000 01001110, or
 * hexadecimal 90 4E.
 *
 *-- If the compression type is "basic" and bit 6 of the compression control
 * byte was set to 1, then the next (second) byte specifies "filter id" which
 * tells the decoder what filter type was used by the encoder to pre-process
 * pixel data before the compression. The "filter id" byte can be one of the
 * following:
 *
 *   0:  no filter ("copy" filter);
 *   1:  "palette" filter;
 *   2:  "gradient" filter.
 *
 *-- If bit 6 of the compression control byte is set to 0 (no "filter id"
 * byte), or if the filter id is 0, then raw pixel values in the client
 * format (see NOTE 1) will be compressed. See below details on the
 * compression.
 *
 *-- The "gradient" filter pre-processes pixel data with a simple algorithm
 * which converts each color component to a difference between a "predicted"
 * intensity and the actual intensity. Such a technique does not affect
 * uncompressed data size, but helps to compress photo-like images better. 
 * Pseudo-code for converting intensities to differences is the following:
 *
 *   P[i,j] := V[i-1,j] + V[i,j-1] - V[i-1,j-1];
 *   if (P[i,j] < 0) then P[i,j] := 0;
 *   if (P[i,j] > MAX) then P[i,j] := MAX;
 *   D[i,j] := V[i,j] - P[i,j];
 *
 * Here V[i,j] is the intensity of a color component for a pixel at
 * coordinates (i,j). MAX is the maximum value of intensity for a color
 * component.
 *
 *-- The "palette" filter converts true-color pixel data to indexed colors
 * and a palette which can consist of 2..256 colors. If the number of colors
 * is 2, then each pixel is encoded in 1 bit, otherwise 8 bits is used to
 * encode one pixel. 1-bit encoding is performed such way that the most
 * significant bits correspond to the leftmost pixels, and each raw of pixels
 * is aligned to the byte boundary. When "palette" filter is used, the
 * palette is sent before the pixel data. The palette begins with an unsigned
 * byte which value is the number of colors in the palette minus 1 (i.e. 1
 * means 2 colors, 255 means 256 colors in the palette). Then follows the
 * palette itself which consist of pixel values in client pixel format (see
 * NOTE 1).
 *
 *-- The pixel data is compressed using the zlib library. But if the data
 * size after applying the filter but before the compression is less then 12,
 * then the data is sent as is, uncompressed. Four separate zlib streams
 * (0..3) can be used and the decoder should read the actual stream id from
 * the compression control byte (see NOTE 2).
 *
 * If the compression is not used, then the pixel data is sent as is,
 * otherwise the data stream looks like this:
 *
 *   1..3 bytes:  data size (N) in compact representation;
 *   N bytes:     zlib-compressed data.
 *
 * Data size is compactly represented in one, two or three bytes, just like
 * in the "jpeg" compression method (see above).
 *
 *-- NOTE 1. If the color depth is 24, and all three color components are
 * 8-bit wide, then one pixel in Tight encoding is always represented by
 * three bytes, where the first byte is red component, the second byte is
 * green component, and the third byte is blue component of the pixel color
 * value. This applies to colors in palettes as well.
 *
 *-- NOTE 2. The decoder must reset compression streams' states before
 * decoding the rectangle, if some of bits 0,1,2,3 in the compression control
 * byte are set to 1. Note that the decoder must reset zlib streams even if
 * the compression type is "fill" or "jpeg".
 *
 *-- NOTE 3. The "gradient" filter and "jpeg" compression may be used only
 * when bits-per-pixel value is either 16 or 32, not 8.
 *
 *-- NOTE 4. The width of any Tight-encoded rectangle cannot exceed 2048
 * pixels. If a rectangle is wider, it must be split into several rectangles
 * and each one should be encoded separately.
 *
 */

#define rfbTightExplicitFilter         0x04
#define rfbTightFill                   0x08
#define rfbTightJpeg                   0x09
#define rfbTightMaxSubencoding         0x09

/* Filters to improve compression efficiency */
#define rfbTightFilterCopy             0x00
#define rfbTightFilterPalette          0x01
#define rfbTightFilterGradient         0x02


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * ZLIBHEX - zlib compressed Hextile Encoding.  Essentially, this is the
 * hextile encoding with zlib compression on the tiles that can not be
 * efficiently encoded with one of the other hextile subencodings.  The
 * new zlib subencoding uses two bytes to specify the length of the
 * compressed tile and then the compressed data follows.  As with the
 * raw sub-encoding, the zlib subencoding invalidates the other
 * values, if they are also set.
 */

#define rfbHextileZlibRaw		(1 << 5)
#define rfbHextileZlibHex		(1 << 6)



/*-----------------------------------------------------------------------------
 * SetColourMapEntries - these messages are only sent if the pixel
 * format uses a "colour map" (i.e. trueColour false) and the client has not
 * fixed the entire colour map using FixColourMapEntries.  In addition they
 * will only start being sent after the client has sent its first
 * FramebufferUpdateRequest.  So if the client always tells the server to use
 * trueColour then it never needs to process this type of message.
 */

struct aaSetColourMapEntriesMsg {
    UINT8 type;			/* always aaSetColourMapEntries */
    UINT16 firstColour;
    UINT16 nColours;

    /* Followed by nColours * 3 * UINT16
       r1, g1, b1, r2, g2, b2, r3, g3, b3, ..., rn, bn, gn */

};


struct aaPointerMoveMsg {
    UINT8 type;		// always aaPointerMove
	UINT8 counter;	// counter of aaPointerEvent
    UINT16 x;
    UINT16 y;
};


/*****************************************************************************
 *
 * Message definitions (client -> server)
 *
 *****************************************************************************/



/*-----------------------------------------------------------------------------
 * SetEncodings - tell the Ammyy target which encoding types we accept.  Put them
 * in order of preference, if we have any.  We may always receive raw encoding, even if we don't specify it here.
 */

enum aaPixelFormatEnum
{
	aaPixelFormat8_gray = 0,
	aaPixelFormat8  = 1,
	aaPixelFormat16 = 2,
	aaPixelFormat24 = 3,
	aaPixelFormat32 = 4
};

struct aaSetEncoderMsg {
    UINT8 type;			/* always aaSetEncoder */	
	aaPixelFormat	pixelFrm;
	UINT8			colorQuality;
	UINT8 encoder;
	UINT8 copyRect;
	 INT8 compressLevel; // -1        - is off
	 INT8 qualityLevel;  // aaJpegOFF - is off
};

#define aaJpegOFF 101

struct aaSetPointerMsg {
    UINT8 type;			/* always aaSetPointer */	
	UINT8 pointerShape;
	UINT8 pointerPos;
};


enum aaSetPointerEnum
{
	aaSetPointerPos   = 1,
	aaSetPointerShape = 2,
};


/*-----------------------------------------------------------------------------*/



struct aaScreenUpdateCommitMsg {
	UINT8 type;			/* always aaScreenUpdateCommitMsg */
	UINT8 countCommits;
};


/*-----------------------------------------------------------------------------
 * KeyEvent - key press or release
 *
 * Keys are specified using the "keysym" values defined by the X Window System.
 * For most ordinary keys, the keysym is the same as the corresponding ASCII
 * value.  Other common keys are:
 *
 * BackSpace	0xff08
 * Tab			0xff09
 * Return or Enter	0xff0d
 * Escape		0xff1b
 * Insert		0xff63
 * Delete		0xffff
 * Home			0xff50
 * End			0xff57
 * Page Up		0xff55
 * Page Down	0xff56
 * Left			0xff51
 * Up			0xff52
 * Right		0xff53
 * Down			0xff54
 * F1			0xffbe
 * F2			0xffbf
 * ...			...
 * F12			0xffc9
 * Shift		0xffe1
 * Control		0xffe3
 * Meta			0xffe7
 * Alt			0xffe9
 */

struct aaKeyEventMsg {
    UINT8 type;			// always aaKeyEvent
    UINT8 down;			// true if down (press), false if up
    UINT32 key;			// key is specified as an X keysym 
};


/*-----------------------------------------------------------------------------
 * PointerEvent - mouse/pen move and/or button press.
 */

struct aaPointerEventMsg {
    UINT8 type;			/* always aaPointerEvent */
    UINT8 buttonMask;		/* bits 0-7 are buttons 1-8, 0=up, 1=down */
    UINT16 x;
    UINT16 y;
};

#define aaButton1Mask 1
#define aaButton2Mask 2
#define aaButton3Mask 4
#define aaButton4Mask 8
#define aaButton5Mask 16


struct aaCutTextMsg {
    UINT8 type;			/* always aaCutText */
    UINT32 length;
    /* followed by char text[length] */
};


#pragma pack(pop)

#endif // _AA_DESKTOP_H__INCLUDED_
