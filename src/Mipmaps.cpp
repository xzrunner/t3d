#include "Mipmaps.h"

#include "tmath.h"
#include "BmpImg.h"
#include "Graphics.h"

namespace t3d {

int GenerateMipmaps(const BmpImg& source, BmpImg** mipmaps, float gamma)
{
	// this functions creates a mip map chain of bitmap textures
	// on entry source should point to the bottom level d = 0 texture
	// and on exit mipmap will point to an array of pointers that holds
	// all the mip levels including source as entry 0, additionally the
	// function will return the total number of levels or -1 if there 
	// is a problem, the last param gamma is used to brighten each mip level up
	// since averaging has the effect of darkening, a value of 1.01 is usually 
	// good, values greater that 1.0 brighten each mip map, values less than
	// 1.0 darken each mip map, and 1.0 has no effect

	BmpImg** tmipmaps; // local temporary pointer to array of pointers

	// step 1: compute number of mip levels total
	int num_mip_levels = logbase2ofx[source.Width()] + 1; 

	// allocate array of pointers to mip maps
	tmipmaps = (BmpImg**)malloc(num_mip_levels * sizeof(BmpImg*));

	// point element 0 (level 0) to entry source
	tmipmaps[0] = const_cast<BmpImg*>(&source);

	// set width and height (same actually)
	int mip_width  = source.Width();
	int mip_height = source.Height();

	// iterate thru and generate pyramid mipmap levels using averaging filter
	for (int mip_level = 1; mip_level <  num_mip_levels; mip_level++)
	{
		// scale size of mip map down one level
		mip_width  = mip_width  / 2;
		mip_height = mip_height / 2;

		// create a bitmap to hold mip map
		tmipmaps[mip_level] = new BmpImg(0,0, mip_width, mip_height, 32);

		// enable the bitmap for rendering  
		tmipmaps[mip_level]->SetAttrBit(BITMAP_ATTR_LOADED);

		// now interate thru previous level's mipmap and average down to create this level
		for (int x = 0; x < tmipmaps[mip_level]->Width(); x++)
		{
			for (int y = 0; y < tmipmaps[mip_level]->Height(); y++)
			{
				// we need to average the 4 pixel located at:
				// (x*2, y*2), (x*2+1, y*2), (x*2,y*2+1), (x*2+1,y*2+1)
				// in previous mipmap level, and then write them
				// to x,y in this mipmap level :) easy!
				float a;
				float r0, g0, b0,        // r,g,b components of 4 sample pixels
					  r1, g1, b1,
					  r2, g2, b2,
					  r3, g3, b3;

				int r_avg, g_avg, b_avg; // used to compute averages

				unsigned int *src_buffer  = (unsigned int*)tmipmaps[mip_level-1]->Buffer(),
							 *dest_buffer = (unsigned int*)tmipmaps[mip_level]->Buffer();

				// extract rgb components of each pixel
				_RGB8888FROM32BIT( src_buffer[(x*2+0) + (y*2+0)*mip_width*2] , &a, &r0, &g0, &b0);
				_RGB8888FROM32BIT( src_buffer[(x*2+1) + (y*2+0)*mip_width*2] , &a, &r1, &g1, &b1);
				_RGB8888FROM32BIT( src_buffer[(x*2+0) + (y*2+1)*mip_width*2] , &a, &r2, &g2, &b2);
				_RGB8888FROM32BIT( src_buffer[(x*2+1) + (y*2+1)*mip_width*2] , &a, &r3, &g3, &b3);

				// compute average, take gamma into consideration
				r_avg = (int)(0.5f + gamma*(r0+r1+r2+r3)/4);
				g_avg = (int)(0.5f + gamma*(g0+g1+g2+g3)/4);
				b_avg = (int)(0.5f + gamma*(b0+b1+b2+b3)/4);

				// clamp values to max r, g, b values for 8.8.8 format
				if (r_avg > 255) r_avg = 255;
				if (g_avg > 255) g_avg = 255;
				if (b_avg > 255) b_avg = 255;

				// write data
				dest_buffer[x + y*mip_width] = _RGB32BIT(255,r_avg,g_avg,b_avg);

			} // end for y

		} // end for x

	} // end for mip_level

	// now assign array of pointers to exit 
	*mipmaps = (BmpImg*)tmipmaps;

	return num_mip_levels;
}

int DeleteMipmaps(BmpImg** mipmaps, bool leave_level_0)
{
	// this function deletes all the mipmaps in the chain
	// that each pointer in the array mipmaps points to and 
	// then releases the array memory itself, the function has the
	// ability to leave the top level 0 bitmap in place if 
	// leave_level_0 flag is true

	BmpImg** tmipmaps = (BmpImg**)*mipmaps;

	// are there any mipmaps
	if (!tmipmaps)
		return(0);

	// step 1: compute number of mip levels total
	int num_mip_levels = logbase2ofx[tmipmaps[0]->Width()] + 1; 

	// iterate thru delete each bitmap
	for (int mip_level = 1; mip_level < num_mip_levels; mip_level++)
	{
		// release the memory for the bitmap buffer
		// now release the bitmap object itself
		delete tmipmaps[mip_level];

	} // end for mip_level

	// now depending on the leave_level_0 flag delete everything or leave the 
	// original level 0 in place
	if (leave_level_0)
	{
		// we need a temp pointer to the level 0 bitmap
		BmpImg* temp = tmipmaps[0];

		// and the array storage too
		//free(*tmipmaps);

		// and assign mipmaps to the original level 0
		*tmipmaps = temp;
	} // end if     
	else 
	{
		// delete everything!
		// now release the bitmap object itself
		delete tmipmaps[0];

		// and the array storage too
		//free(*tmipmaps);
	} // end else

	// return success
	return(1);
}

}