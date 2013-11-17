#include "BmpFile.h"

#include "Modules.h"
#include "Graphics.h"

namespace t3d {

BmpFile::BmpFile(char *filename)
	: _buffer(NULL)
{
	Load(filename);
}

BmpFile::~BmpFile()
{
	Unload();
}

int BmpFile::Load(char *filename)
{
	// this function opens a bitmap file and loads the data into bitmap

	int file_handle,  // the file handle
		index;        // looping index

	unsigned char* temp_buffer = NULL; // used to convert 24 bit images to 16 bit
	OFSTRUCT file_data;          // the file data information

	// open the file if it exists
	if ((file_handle = OpenFile(filename,&file_data,OF_READ))==-1)
		return(0);

	// now load the bitmap file header
	_lread(file_handle, &_bitmapfileheader,sizeof(BITMAPFILEHEADER));

	// test if this is a bitmap file
	if (_bitmapfileheader.bfType!=BITMAP_ID)
	{
		// close the file
		_lclose(file_handle);

		// return error
		return(0);
	} // end if

	// now we know this is a bitmap, so read in all the sections

	// first the bitmap infoheader

	// now load the bitmap file header
	_lread(file_handle, &_bitmapinfoheader,sizeof(BITMAPINFOHEADER));

	// now load the color palette if there is one
	if (_bitmapinfoheader.biBitCount == 8)
	{
		_lread(file_handle, &_palette,Graphics::MAX_COLORS_PALETTE*sizeof(PALETTEENTRY));

		// now set all the flags in the palette correctly and fix the reversed 
		// BGR RGBQUAD data format
		for (index=0; index < Graphics::MAX_COLORS_PALETTE; index++)
		{
			// reverse the red and green fields
			int temp_color                = _palette[index].peRed;
			_palette[index].peRed  = _palette[index].peBlue;
			_palette[index].peBlue = temp_color;

			// always set the flags word to this
			_palette[index].peFlags = PC_NOCOLLAPSE;
		} // end for index

	} // end if

	// finally the image data itself
	//_lseek(file_handle,-(int)(_bitmapinfoheader.biSizeImage),SEEK_END);

	// now read in the image
	if (_bitmapinfoheader.biBitCount==8 || _bitmapinfoheader.biBitCount==16) 
	{
		// delete the last image if there was one
		if (_buffer)
			free(_buffer);

		// allocate the memory for the image
		if (!(_buffer = (unsigned char *)malloc(_bitmapinfoheader.biSizeImage)))
		{
			// close the file
			_lclose(file_handle);

			// return error
			return(0);
		} // end if

		// now read it in
		_lread(file_handle,_buffer,_bitmapinfoheader.biSizeImage);

	} // end if
	else
		if (_bitmapinfoheader.biBitCount==24)
		{
			// allocate temporary buffer to load 24 bit image
			if (!(temp_buffer = (unsigned char *)malloc(_bitmapinfoheader.biSizeImage)))
			{
				// close the file
				_lclose(file_handle);

				// return error
				return(0);
			} // end if

			//    // allocate final 16 bit storage buffer
			//    if (!(_buffer=(unsigned char *)malloc(2*_bitmapinfoheader.biWidth*_bitmapinfoheader.biHeight)))
			//       {
			//       // close the file
			//       _lclose(file_handle);
			// 
			//       // release working buffer
			//       free(temp_buffer);
			// 
			//       // return error
			//       return(0);
			//       } // end if

			bool Is32Bit = Modules::GetGraphics().GetDDPixelFormat() == DD_PIXEL_FORMATALPHA888;
			int size = Is32Bit ? 4 : 2;

			// allocate final 16 bit storage buffer
			if (!(_buffer=(unsigned char *)malloc(size*_bitmapinfoheader.biWidth*_bitmapinfoheader.biHeight)))
			{
				// close the file
				_lclose(file_handle);

				// release working buffer
				free(temp_buffer);

				// return error
				return(0);
			} // end if

			// now read the file in
			_lread(file_handle,temp_buffer,_bitmapinfoheader.biSizeImage);

			if (Is32Bit)
			{
				// now convert each 24 bit RGB value into a 32 bit value
				for (index=0; index < _bitmapinfoheader.biWidth*_bitmapinfoheader.biHeight; index++)
				{
					// extract RGB components (in BGR order), note the scaling
					unsigned int blue  = temp_buffer[index*3 + 0],
						green = temp_buffer[index*3 + 1],
						red   = temp_buffer[index*3 + 2]; 
					// build up 32 bit color word
					unsigned int color = Modules::GetGraphics().GetColor(red,green,blue);

					// write color to buffer
					((unsigned int*)_buffer)[index] = color;
				}
			}
			else
			{
				// now convert each 24 bit RGB value into a 16 bit value
				for (index=0; index < _bitmapinfoheader.biWidth*_bitmapinfoheader.biHeight; index++)
				{
					// extract RGB components (in BGR order), note the scaling
					unsigned char blue  = temp_buffer[index*3 + 0],
						green = temp_buffer[index*3 + 1],
						red   = temp_buffer[index*3 + 2]; 
					// build up 16 bit color word
					unsigned short color = Modules::GetGraphics().GetColor(red,green,blue);

					// write color to buffer
					((unsigned short *)_buffer)[index] = color;
				}
			}

			// finally write out the correct number of bits
			_bitmapinfoheader.biBitCount= Is32Bit? 32 : 16;

			// release working buffer
			free(temp_buffer);
		}
		else
		{
			// serious problem
			return(0);
		}

#if 0
		// write the file info out 
		printf("\nfilename:%s \nsize=%d \nwidth=%d \nheight=%d \nbitsperpixel=%d \ncolors=%d \nimpcolors=%d",
			filename,
			_bitmapinfoheader.biSizeImage,
			_bitmapinfoheader.biWidth,
			_bitmapinfoheader.biHeight,
			_bitmapinfoheader.biBitCount,
			_bitmapinfoheader.biClrUsed,
			_bitmapinfoheader.biClrImportant);
#endif

	// close the file
	_lclose(file_handle);

	// flip the bitmap
	FlipBitmap(_buffer, 
		_bitmapinfoheader.biWidth*(_bitmapinfoheader.biBitCount/8), 
		_bitmapinfoheader.biHeight);

	return(1);
}

int BmpFile::Unload()
{
	// this function releases all memory associated with "bitmap"
	if (_buffer)
	{
		// release memory
		free(_buffer);

		// reset pointer
		_buffer = NULL;

	} // end if

	// return success
	return(1);

} // end UnloadBitmapFile

int BmpFile::FlipBitmap(unsigned char *image, int bytes_per_line, int height)
{
	// this function is used to flip bottom-up .BMP images

	unsigned char *buffer; // used to perform the image processing
	int index;     // looping index

	// allocate the temporary buffer
	if (!(buffer = (unsigned char *)malloc(bytes_per_line*height)))
		return(0);

	// copy image to work area
	memcpy(buffer,image,bytes_per_line*height);

	// flip vertically
	for (index=0; index < height; index++)
		memcpy(&image[((height-1) - index)*bytes_per_line],
		&buffer[index*bytes_per_line], bytes_per_line);

	// release the memory
	free(buffer);

	// return success
	return(1);

} // end FlipBitmap

}