#pragma once

#include "defines.h"

namespace t3d {

class BmpFile;

#define BITMAP_STATE_DEAD		0
#define BITMAP_STATE_ALIVE		1
#define BITMAP_STATE_DYING		2 

#define BITMAP_ATTR_LOADED		128

#define BITMAP_EXTRACT_MODE_CELL  0
#define BITMAP_EXTRACT_MODE_ABS   1

class BmpImg
{
public:
	BmpImg(int x, int y, int width, int height, int bpp=8);
	~BmpImg();

	int Draw(unsigned char* buffer, int lpitch, int transparent);
	int Draw16(unsigned char* buffer, int lpitch, int transparent);
	int Draw32(unsigned char* buffer, int lpitch, int transparent);

	// bitmap: bitmap to scan image data from
	// cx, cy: cell or absolute pos. to scan image from
	// mode: if 0 then cx,cy is cell position, else cx,cy are absolute coords
	int LoadImage8(const BmpFile& bitmap,int cx,int cy,int mode);  
	int LoadImage16(const BmpFile& bitmap,int cx,int cy,int mode);    
	int LoadImage32(const BmpFile& bitmap,int cx,int cy,int mode);

	int Scroll(int dx, int dy=0);

	int Width() const { return _width; }
	int Height() const { return _height; }
	int BitsPerPixel() const { return _bpp; }

	const unsigned char* Buffer() const { return _buffer; }

	void SetAttrBit(int bit) {
		SET_BIT(_attr, bit);
	}

	void SetPos(int x, int y) {
		_x = x;
		_y = y;
	}

	static int Copy(BmpImg* dest_bitmap, int dest_x, int dest_y, 
		const BmpImg* source_bitmap, int source_x, int source_y, 
		int width, int height);
	static int Flip(unsigned char *image, int bytes_per_line, int height);

private:
	int Create(int x, int y, int width, int height, int bpp=8);
	int Destroy();

private:
	int _state;					// state of bitmap
	int _attr;					// attributes of bitmap
	int _x, _y;					// position of bitmap
	int _width, _height;		// size of bitmap
	int _num_bytes;				// total bytes of bitmap
	int _bpp;					// bits per pixel
	unsigned char* _buffer;     // pixels of bitmap

}; // BmpImg

}