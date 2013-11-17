#include "ZBuffer.h"

#include <stdlib.h>
#include <memory.h>

#include "tools.h"

namespace t3d {

int ZBuffer::Create(int width, int height, int attr)
{
	// this function creates a zbuffer with the sent width, height, 
	// and bytes per word, in fact the the zbuffer is totally linear
	// but this function is nice so we can create it with some
	// intelligence

	// is there any memory already allocated
	if (_zbuffer)
		free(_zbuffer);

	// set fields
	_width  = width;
	_height = height;
	_attr   = attr;

	// what size zbuffer 16/32 bit?
	if (attr & ZBUFFER_ATTR_16BIT)
	{
		// compute size in quads
		_sizeq = width*height/2;

		// allocate memory
		if ((_zbuffer = (unsigned char*)malloc(width * height * sizeof(unsigned short))))
			return(1);
		else
			return(0);

	} // end if
	else if (attr & ZBUFFER_ATTR_32BIT)
	{
		// compute size in quads
		_sizeq = width*height;

		// allocate memory
		if ((_zbuffer = (unsigned char*)malloc(width * height * sizeof(unsigned int))))
			return(1);
		else
			return(0);
	} // end if
	else
		return(0);
}

int ZBuffer::Delete()
{
	// this function deletes and frees the memory of the zbuffer

	// delete memory and zero object
	if (_zbuffer)
		free(_zbuffer);

	// clear memory
	memset(this,0, sizeof(ZBuffer));

	return(1);
}

void ZBuffer::Clear(unsigned int data)
{
	// this function clears/sets the zbuffer to a value, the filling
	// is ALWAYS performed in QUADS, thus if your zbuffer is a 16-bit
	// zbuffer then you must build a quad that has two values each
	// the 16-bit value you want to fill with, otherwise just send 
	// the fill value casted to a UINT

	Mem_Set_QUAD((void *)_zbuffer, data, _sizeq); 
}

}