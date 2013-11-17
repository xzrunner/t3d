#include "BmpImg.h"

#include <stdlib.h>
#include <string.h>

#include "BmpFile.h"

namespace t3d {

BmpImg::BmpImg(int x, int y, int width, int height, int bpp)
{
	Create(x, y, width, height, bpp);
}

BmpImg::~BmpImg()
{
	Destroy();
}

int BmpImg::Draw(unsigned char* buffer, int lpitch, int transparent)
{
	// this function draws the bitmap onto the destination memory surface
	// if transparent is 1 then color 0 (8bit) or 0.0.0 (16bit) will be transparent
	// note this function does NOT clip, so be carefull!!!

	// test if this bitmap is loaded
	if (!(_attr & BITMAP_ATTR_LOADED))
		return(0);

	unsigned char *dest_addr,   // starting address of bitmap in destination
		*source_addr; // starting adddress of bitmap data in source

	unsigned char pixel;        // used to hold pixel value

	int index,          // looping vars
		pixel_x;

	// compute starting destination address
	dest_addr = buffer + _y*lpitch + _x;

	// compute the starting source address
	source_addr = _buffer;

	// is this bitmap transparent
	if (transparent)
	{
		// copy each line of bitmap into destination with transparency
		for (index=0; index<_height; index++)
		{
			// copy the memory
			for (pixel_x=0; pixel_x<_width; pixel_x++)
			{
				if ((pixel = source_addr[pixel_x])!=0)
					dest_addr[pixel_x] = pixel;
			}

			// advance all the pointers
			dest_addr   += lpitch;
			source_addr += _width;
		}
	}
	else
	{
		// non-transparent version
		// copy each line of bitmap into destination

		for (index=0; index < _height; index++)
		{
			// copy the memory
			memcpy(dest_addr, source_addr, _width);

			// advance all the pointers
			dest_addr   += lpitch;
			source_addr += _width;
		}
	}

	return(1);
}

int BmpImg::Draw16(unsigned char* buffer, int lpitch, int transparent)
{
	// this function draws the bitmap onto the destination memory surface
	// if transparent is 1 then color 0 (8bit) or 0.0.0 (16bit) will be transparent
	// note this function does NOT clip, so be carefull!!!

	// test if this bitmap is loaded
	if (!(_attr & BITMAP_ATTR_LOADED))
		return(0);

	unsigned short *dest_addr,   // starting address of bitmap in destination
		*source_addr; // starting adddress of bitmap data in source

	unsigned short pixel;        // used to hold pixel value

	int index,           // looping vars
		pixel_x,
		lpitch_2 = lpitch >> 1; // lpitch in unsigned short terms

	// compute starting destination address
	dest_addr = ((unsigned short *)buffer) + _y*lpitch_2 + _x;

	// compute the starting source address
	source_addr = (unsigned short *)_buffer;

	// is this bitmap transparent
	if (transparent)
	{
		// copy each line of bitmap into destination with transparency
		for (index=0; index<_height; index++)
		{
			// copy the memory
			for (pixel_x=0; pixel_x<_width; pixel_x++)
			{
				if ((pixel = source_addr[pixel_x])!=0)
					dest_addr[pixel_x] = pixel;
			}

			// advance all the pointers
			dest_addr   += lpitch_2;
			source_addr += _width;
		}
	}
	else
	{
		// non-transparent version
		// copy each line of bitmap into destination

		int source_bytes_per_line = _width*2;

		for (index=0; index < _height; index++)
		{
			// copy the memory
			memcpy(dest_addr, source_addr, source_bytes_per_line);

			// advance all the pointers
			dest_addr   += lpitch_2;
			source_addr += _width;

		}
	}

	return(1);
}

int BmpImg::Draw32(unsigned char* buffer, int lpitch, int transparent)
{
	// test if this bitmap is loaded
	if (!(_attr & BITMAP_ATTR_LOADED))
		return(0);

	unsigned int *dest_addr,   // starting address of bitmap in destination
		*source_addr; // starting adddress of bitmap data in source

	unsigned int pixel;        // used to hold pixel value

	int index,           // looping vars
		pixel_x,
		lpitch_4 = lpitch >> 2; // lpitch in unsigned short terms

	// compute starting destination address
	dest_addr = ((unsigned int*)buffer) + _y*lpitch_4 + _x;

	// compute the starting source address
	source_addr = (unsigned int*)_buffer;

	// is this bitmap transparent
	if (transparent)
	{
		// copy each line of bitmap into destination with transparency
		for (index=0; index<_height; index++)
		{
			// copy the memory
			for (pixel_x=0; pixel_x<_width; pixel_x++)
			{
				if ((pixel = source_addr[pixel_x])!=0)
					dest_addr[pixel_x] = pixel;
			}

			// advance all the pointers
			dest_addr   += lpitch_4;
			source_addr += _width;
		}
	}
	else
	{
		// non-transparent version
		// copy each line of bitmap into destination

		int source_bytes_per_line = _width*4;

		for (index=0; index < _height; index++)
		{
			// copy the memory
			memcpy(dest_addr, source_addr, source_bytes_per_line);

			// advance all the pointers
			dest_addr   += lpitch_4;
			source_addr += _width;

		}
	}

	return(1);
}

int BmpImg::LoadImage8(const BmpFile& bitmap, int cx, int cy, int mode)
{
	// this function extracts a bitmap out of a bitmap file

	// working pointers
	const unsigned char* source_ptr;
	unsigned char* dest_ptr;

	// test the mode of extraction, cell based or absolute
	if (mode == BITMAP_EXTRACT_MODE_CELL)
	{
		// re-compute x,y
		cx = cx*(_width+1) + 1;
		cy = cy*(_height+1) + 1;
	} // end if

	// extract bitmap data
	source_ptr = bitmap.Buffer() + cy * bitmap.Width() + cx;

	// assign a pointer to the bimap image
	dest_ptr = (unsigned char *)_buffer;

	// iterate thru each scanline and copy bitmap
	for (int index_y=0; index_y<_height; index_y++)
	{
		// copy next line of data to destination
		memcpy(dest_ptr, source_ptr,_width);

		// advance pointers
		dest_ptr   += _width;
		source_ptr += bitmap.Width();
	}

	// set state to loaded
	_attr |= BITMAP_ATTR_LOADED;

	return(1);
}

int BmpImg::LoadImage16(const BmpFile& bitmap, int cx, int cy, int mode)
{
	// this function extracts a 16-bit bitmap out of a 16-bit bitmap file

	// working pointers
	const unsigned short* source_ptr;
	unsigned short* dest_ptr;

	// test the mode of extraction, cell based or absolute
	if (mode == BITMAP_EXTRACT_MODE_CELL)
	{
		// re-compute x,y
		cx = cx*(_width+1) + 1;
		cy = cy*(_height+1) + 1;
	}

	// extract bitmap data
	source_ptr = (const unsigned short *)bitmap.Buffer() + cy * bitmap.Width() + cx;

	// assign a pointer to the bimap image
	dest_ptr = (unsigned short *)_buffer;

	int bytes_per_line = _width*2;

	// iterate thru each scanline and copy bitmap
	for (int index_y=0; index_y < _height; index_y++)
	{
		// copy next line of data to destination
		memcpy(dest_ptr, source_ptr,bytes_per_line);

		// advance pointers
		dest_ptr   += _width;
		source_ptr += bitmap.Width();
	}

	// set state to loaded
	_attr |= BITMAP_ATTR_LOADED;

	return(1);
}

int BmpImg::LoadImage32(const BmpFile& bitmap,int cx,int cy,int mode)
{
	// this function extracts a 32-bit bitmap out of a 16-bit bitmap file

	// working pointers
	const unsigned int* source_ptr;
	unsigned int* dest_ptr;

	// test the mode of extraction, cell based or absolute
	if (mode == BITMAP_EXTRACT_MODE_CELL)
	{
		// re-compute x,y
		cx = cx*(_width+1) + 1;
		cy = cy*(_height+1) + 1;
	}

	// extract bitmap data
	source_ptr = (const unsigned int *)bitmap.Buffer() + cy * bitmap.Width() + cx;

	// assign a pointer to the bimap image
	dest_ptr = (unsigned int *)_buffer;

	int bytes_per_line = _width*4;

	// iterate thru each scanline and copy bitmap
	for (int index_y=0; index_y < _height; index_y++)
	{
		// copy next line of data to destination
		memcpy(dest_ptr, source_ptr,bytes_per_line);

		// advance pointers
		dest_ptr   += _width;
		source_ptr += bitmap.Width();
	}

	// set state to loaded
	_attr |= BITMAP_ATTR_LOADED;

	return(1);
}

int BmpImg::Scroll(int dx, int dy)
{
	// this function scrolls a bitmap

	// are the parms valid 
	if (dx==0 && dy==0)
		return(0);

	// scroll on x-axis first
	if (dx!=0)
	{
		// step 1: normalize scrolling amount
		dx %= _width;

		// step 2: which way?
		if (dx > 0)
		{
			// scroll right
			// create bitmap to hold region that is scrolled around
			BmpImg temp_image(0, 0, dx, _height, _bpp); // temp image buffer

			// copy region we are going to scroll and wrap around
			Copy(&temp_image, 0, 0, this, _width-dx, 0, dx, _height);

			// set some pointers up
			unsigned char *source_ptr = _buffer;  // start of each line
			int shift         = (_bpp >> 3)*dx;

			// now scroll image to right "scroll" pixels
			for (int y=0; y < _height; y++)
			{
				// scroll the line over
				memmove(source_ptr+shift, source_ptr, (_width-dx)*(_bpp >> 3));

				// advance to the next line
				source_ptr+=((_bpp >> 3)*_width);
			} // end for

			// and now copy it back
			Copy(this, 0,0, &temp_image, 0, 0, dx, _height);           
		}
		else
		{
			// scroll left
			dx = -dx; // invert sign

			// create bitmap to hold region that is scrolled around
			BmpImg temp_image(0, 0, dx, _height, _bpp); // temp image buffer

			// copy region we are going to scroll and wrap around
			Copy(&temp_image, 0, 0, this, 0, 0, dx, _height);

			// set some pointers up
			unsigned char *source_ptr = _buffer;  // start of each line
			int shift         = (_bpp >> 3)*dx;

			// now scroll image to left "scroll" pixels
			for (int y=0; y < _height; y++)
			{
				// scroll the line over
				memmove(source_ptr, source_ptr+shift, (_width-dx)*(_bpp >> 3));

				// advance to the next line
				source_ptr+=((_bpp >> 3)*_width);
			}

			// and now copy it back
			Copy(this, _width-dx, 0, &temp_image, 0, 0, dx, _height);           
		}
	}

	return(1);
}

int BmpImg::Copy(BmpImg* dest_bitmap, int dest_x, int dest_y, 
	const BmpImg* source_bitmap, int source_x, int source_y, 
	int width, int height)
{
	// this function copies a bitmap from one source to another

	// make sure the pointers are at least valid
	if (!dest_bitmap || !source_bitmap)
		return(0);

	// do some computations
	int bytes_per_pixel = (source_bitmap->_bpp >> 3);

	// create some pointers
	unsigned char *source_ptr = source_bitmap->_buffer + (source_x + source_y * source_bitmap->_width) * bytes_per_pixel;
	unsigned char *dest_ptr   = dest_bitmap->_buffer + (dest_x + dest_y * dest_bitmap->_width) * bytes_per_pixel;

	// now copying is easy :)
	for (int y = 0; y < height; y++)
	{
		// copy this line
		memcpy(dest_ptr, source_ptr, bytes_per_pixel*width);

		// advance the pointers
		source_ptr += (source_bitmap->_width * bytes_per_pixel);
		dest_ptr   += (dest_bitmap->_width * bytes_per_pixel);
	}

	return(1);
}

int BmpImg::Flip(unsigned char *image, int bytes_per_line, int height)
{
	// this function is used to flip bottom-up .BMP images

	unsigned char *buffer; // used to perform the image processing

	// allocate the temporary buffer
	if (!(buffer = (unsigned char *)malloc(bytes_per_line*height)))
		return(0);

	// copy image to work area
	memcpy(buffer,image,bytes_per_line*height);

	// flip vertically
	for (int index=0; index < height; index++)
		memcpy(&image[((height-1) - index)*bytes_per_line], 
			&buffer[index*bytes_per_line], bytes_per_line);

	// release the memory
	free(buffer);

	return(1);
}

int BmpImg::Create(int x, int y, int width, int height, int bpp)
{
	// this function is used to intialize a bitmap, 8 or 16 bit

	// allocate the memory
	if (!(_buffer = (unsigned char *)malloc(width*height*(bpp>>3))))
		return(0);

	// initialize variables
	_state     = BITMAP_STATE_ALIVE;
	_attr      = 0;
	_width     = width;
	_height    = height;
	_bpp       = bpp;
	_x         = x;
	_y         = y;
	_num_bytes = width*height*(bpp>>3);

	// clear memory out
	memset(_buffer,0,width*height*(bpp>>3));

	return(1);
}

int BmpImg::Destroy()
{
	// this function releases the memory associated with a bitmap

	if (_buffer)
	{
		// free memory and reset vars
		free(_buffer);

		// set all vars in structure to 0
		memset(this, 0, sizeof(BmpImg));

		return(1);
	}
	else  // invalid entry pointer of the object wasn't initialized
		return(0);
}

}