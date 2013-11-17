#include "BOB.h"

#include "Modules.h"
#include "Graphics.h"
#include "BmpFile.h"
#include "BmpImg.h"


namespace t3d {

int BOB::Create(int x, int y,
				int width, int height, 
				int num_frames, 
				int attr,  
				int mem_flags, 
				unsigned int color_key_value, 
				int bpp)
{
	// Create the BOB object, note that all BOBs 
	// are created as offscreen surfaces in VRAM as the
	// default, if you want to use system memory then
	// set flags equal to:
	// DDSCAPS_SYSTEMMEMORY 
	// for video memory you can create either local VRAM surfaces or AGP 
	// surfaces via the second set of constants shown below in the regular expression
	// DDSCAPS_VIDEOMEMORY | (DDSCAPS_NONLOCALVIDMEM | DDSCAPS_LOCALVIDMEM ) 


	DDSURFACEDESC2 ddsd; // used to create surface
	int index;           // looping var

	// set state and attributes of BOB
	this->state          = BOB_STATE_ALIVE;
	this->attr           = attr;
	this->anim_state     = 0;
	this->counter_1      = 0;     
	this->counter_2      = 0;
	this->max_count_1    = 0;
	this->max_count_2    = 0;

	this->curr_frame     = 0;
	this->num_frames     = num_frames;
	this->bpp            = bpp;
	this->curr_animation = 0;
	this->anim_counter   = 0;
	this->anim_index     = 0;
	this->anim_count_max = 0; 
	this->x              = x;
	this->y              = y;
	this->xv             = 0;
	this->yv             = 0;

	// set dimensions of the new bitmap surface
	this->width  = width;
	this->height = height;

	// set all images to null
	for (index=0; index<MAX_BOB_FRAMES; index++)
		this->images[index] = NULL;

	// set all animations to null
	for (index=0; index<MAX_BOB_ANIMATIONS; index++)
		this->animations[index] = NULL;

	this->width_fill = 0;
#if 0
	// make sure surface width is a multiple of 8, some old version of dd like that
	// now, it's unneeded...
	this->width_fill = ((width%8!=0) ? (8-width%8) : 0);
	t3d::Modules::GetLog().WirteError("\nCreate BOB: width_fill=%d",width_fill);
#endif

	// now create each surface
	for (index=0; index<num_frames; index++)
	{
		// set to access caps, width, and height
		memset(&ddsd,0,sizeof(ddsd));
		ddsd.dwSize  = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;

		ddsd.dwWidth  = width + width_fill;
		ddsd.dwHeight = height;

		// set surface to offscreen plain
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | mem_flags;

		// create the surfaces, return failure if problem
		if (FAILED(Modules::GetGraphics().GetDraw()->CreateSurface(&ddsd,&(this->images[index]),NULL)))
			return(0);

		// set color key to default color 000
		// note that if this is a 8bit bob then palette index 0 will be 
		// transparent by default
		// note that if this is a 16bit bob then RGB value 000 will be 
		// transparent
		DDCOLORKEY color_key; // used to set color key
		color_key.dwColorSpaceLowValue  = color_key_value;
		color_key.dwColorSpaceHighValue = color_key_value;

		// now set the color key for source blitting
		(this->images[index])->SetColorKey(DDCKEY_SRCBLT, &color_key);

	} // end for index

	// return success
	return(1);
}

int BOB::LoadFrame(BmpFile* bitmap, int frame, int cx,int cy, int mode)
{
	// this function extracts a 16-bit bitmap out of a 16-bit bitmap file

	DDSURFACEDESC2 ddsd;  //  direct draw surface description 

	unsigned int *source_ptr,   // working pointers
		*dest_ptr;

	// test the mode of extraction, cell based or absolute
	if (mode==BITMAP_EXTRACT_MODE_CELL)
	{
		// re-compute x,y
		cx = cx*(width+1) + 1;
		cy = cy*(height+1) + 1;
	} // end if

	// extract bitmap data
	source_ptr = (unsigned int*)bitmap->Buffer() + cy*bitmap->Width()+cx;

	// get the addr to destination surface memory

	// set size of the structure
	ddsd.dwSize = sizeof(ddsd);

	// lock the display surface
	(images[frame])->Lock(NULL,
		&ddsd,
		DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR,
		NULL);

	// assign a pointer to the memory surface for manipulation
	dest_ptr = (unsigned int*)ddsd.lpSurface;

	// iterate thru each scanline and copy bitmap
	for (int index_y=0; index_y<height; index_y++)
	{
		// copy next line of data to destination
		memcpy(dest_ptr, source_ptr,(width*4));

		// advance pointers
		dest_ptr   += (ddsd.lPitch >> 2); 
		source_ptr += bitmap->Width();
	} // end for index_y

	// unlock the surface 
	(images[frame])->Unlock(NULL);

	// set state to loaded
	attr |= BOB_ATTR_LOADED;

	// return success
	return(1);
}

int BOB::Draw(LPDIRECTDRAWSURFACE7 dest)
{
	// draw a bob at the x,y defined in the BOB
	// on the destination surface defined in dest

	RECT dest_rect,   // the destination rectangle
		source_rect; // the source rectangle                             

	// is bob visible
	if (!(attr & BOB_ATTR_VISIBLE))
		return(1);

	// fill in the destination rect
	dest_rect.left   = x;
	dest_rect.top    = y;
	dest_rect.right  = x+width;
	dest_rect.bottom = y+height;

	// fill in the source rect
	source_rect.left    = 0;
	source_rect.top     = 0;
	source_rect.right   = width;
	source_rect.bottom  = height;

	// blt to destination surface
	if (FAILED(dest->Blt(&dest_rect, images[curr_frame],
		&source_rect,(DDBLT_WAIT | DDBLT_KEYSRC),
		NULL)))
		return(0);

	// return success
	return(1);
}

int BOB::DrawScaled(int swidth, int sheight, LPDIRECTDRAWSURFACE7 dest)
{
	// this function draws a scaled bob to the size swidth, sheight

	RECT dest_rect,   // the destination rectangle
		source_rect; // the source rectangle                             

	// is bob visible
	if (!(attr & BOB_ATTR_VISIBLE))
		return(1);

	// fill in the destination rect
	dest_rect.left   = x;
	dest_rect.top    = y;
	dest_rect.right  = x+swidth;
	dest_rect.bottom = y+sheight;

	// fill in the source rect
	source_rect.left    = 0;
	source_rect.top     = 0;
	source_rect.right   = width;
	source_rect.bottom  = height;

	// blt to destination surface
	if (FAILED(dest->Blt(&dest_rect, images[curr_frame],
		&source_rect,(DDBLT_WAIT | DDBLT_KEYSRC),
		NULL)))
		return(0);

	// return success
	return(1);
}

}