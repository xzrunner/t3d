#pragma once

#include <Windows.h>
#include <WinGDI.h>

namespace t3d {

#define BITMAP_ID				0x4D42 // universal id for a bitmap

class BmpFile
{
public:
	BmpFile(char *filename);
	~BmpFile();

	const unsigned char* Buffer() const { return _buffer; }

	int Width() const { return _bitmapinfoheader.biWidth; }
	int Height() const { return _bitmapinfoheader.biHeight; }
	int BitCount() const { return _bitmapinfoheader.biBitCount; }

private:
	int Load(char *filename);
	int Unload();

private:
	static int FlipBitmap(unsigned char *image, int bytes_per_line, int height);

private:
	BITMAPFILEHEADER _bitmapfileheader;  // this contains the bitmapfile header
	BITMAPINFOHEADER _bitmapinfoheader;  // this is all the info including the palette
	PALETTEENTRY     _palette[256];      // we will store the palette here
	unsigned char*   _buffer;           // this is a pointer to the data
};

}