#pragma once

namespace t3d {

// defines for zbuffer
#define ZBUFFER_ATTR_16BIT   16
#define ZBUFFER_ATTR_32BIT   32 

class ZBuffer
{
public:
	ZBuffer() : _zbuffer(0) {}

	int Create(int width, int height, int attr);

	int Delete();

	void Clear(unsigned int data);

	unsigned char* Buffer() { return _zbuffer; }

private:
	int _attr;       // attributes of zbuffer
	unsigned char* _zbuffer; // ptr to storage
	int _width;      // width in zpixels
	int _height;     // height in zpixels
	int _sizeq;      // total size in QUADs of zbuffer

}; // ZBuffer

}