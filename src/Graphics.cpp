#include "Graphics.h"

#include <locale.h>

#include "Modules.h"
#include "Log.h"
#include "Light.h"
#include "Material.h"

namespace t3d {

unsigned short RGB16Bit565(int r, int g, int b)
{
	// this function simply builds a 5.6.5 format 16 bit pixel
	// assumes input is RGB 0-255 each channel
	r>>=3; g>>=2; b>>=3;
	return(_RGB16BIT565((r),(g),(b)));
}

unsigned short RGB16Bit555(int r, int g, int b)
{
	// this function simply builds a 5.5.5 format 16 bit pixel
	// assumes input is RGB 0-255 each channel
	r>>=3; g>>=3; b>>=3;
	return(_RGB16BIT555((r),(g),(b)));
}

unsigned int RGB32Bit888(int r, int g, int b)
{
	return _RGB32BIT(255, r, g, b);
}

#define DEFAULT_PALETTE_FILE "PALDATA2.PAL"

Graphics::Graphics()
	: lpdd(NULL)
	, lpddsprimary(NULL)
	, lpddsback(NULL)
	, lpddpal(NULL)
	, lpddclipper(NULL)
	, lpddclipperwin(NULL)
	, primary_buffer(NULL)
	, back_buffer(NULL)
	, primary_lpitch(0)
	, back_lpitch(0)
	, _lights_mgr(NULL)
	, _materials_mgr(NULL)
	, RGB16Bit(NULL)
	, RGB32Bit(NULL)
{
	min_clip_x = 0;
	max_clip_x = 0;
	min_clip_y = 0;
	max_clip_y = 0;

	screen_width    = 0;
	screen_height   = 0;
	screen_bpp      = 0;
	screen_windowed = 0;  

	dd_pixel_format = DD_PIXEL_FORMAT565;  // default pixel format

	window_client_x0 = 0;
	window_client_y0 = 0;
}

int Graphics::Init(HWND handle, int width, int height, 
				   int bpp, int windowed, int backbuffer_enable)
{
	// this function initializes directdraw, but allows for a non flippable pure
	// memory backbuffer even in full screen mode, this is to override an anomaly
	// with new directdraw that does not like us reading from the backbuffer in a chained
	// complex surface, thus we have to create a secondary offscreen surface just like in
	// windowed mode for this case, so the alpha blending code will work fast, otherwise
	// its' like 30 times slower! Alas, if you don't want to use the complex chained backbuffer
	// then send a 0 for the last parameter, or in other words for alpha bleding support with
	// speed send a zero if you're going to do full screen mode, otherwise send a 1 as the last 
	// parameter which is the default, hence you can call this function as you have the first
	// version in that case
	int index; // looping variable

	// create IDirectDraw interface 7.0 object and test for error
	if (FAILED(DirectDrawCreateEx(NULL, (void **)&lpdd, IID_IDirectDraw7, NULL)))
		return(0);

	// based on windowed or fullscreen set coorperation level
	if (windowed)
	{
		// set cooperation level to windowed mode 
		if (FAILED(lpdd->SetCooperativeLevel(handle,DDSCL_NORMAL)))
			return(0);
	}
	else
	{
		// set cooperation level to fullscreen mode 
		if (FAILED(lpdd->SetCooperativeLevel(handle,
			DDSCL_ALLOWMODEX | DDSCL_FULLSCREEN | 
			DDSCL_EXCLUSIVE | DDSCL_ALLOWREBOOT | DDSCL_MULTITHREADED )))
			return(0);

		// set the display mode
		if (FAILED(lpdd->SetDisplayMode(width,height,bpp,0,0)))
			return(0);
	}

	// set globals
	screen_height   = height;
	screen_width    = width;
	screen_bpp      = bpp;
	screen_windowed = windowed;
	screen_backbuffer_enable = backbuffer_enable;

	// Create the primary surface
	memset(&ddsd,0,sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	// we need to let dd know that we want a complex 
	// flippable surface structure, set flags for that
	if (!screen_windowed)
	{
		// test for backbuffer enabled
		if (screen_backbuffer_enable)
		{
			// fullscreen mode
			ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
			ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;

			// set the backbuffer count to 0 for windowed mode
			// 1 for fullscreen mode, 2 for triple buffering
			ddsd.dwBackBufferCount = 1;
		}
		else
		{
			// user is requested no back buffer and wants to use a plain offscreen memory
			// surface to speed up reading for alpha blending
			// fullscreen mode
			ddsd.dwFlags = DDSD_CAPS;
			ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

			// set the backbuffer count to 0 for windowed mode
			// 1 for fullscreen mode, 2 for triple buffering
			ddsd.dwBackBufferCount = 0;
		}
	}
	else
	{
		// windowed mode
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

		// set the backbuffer count to 0 for windowed mode
		// 1 for fullscreen mode, 2 for triple buffering
		ddsd.dwBackBufferCount = 0;
	}

	// create the primary surface
	lpdd->CreateSurface(&ddsd,&lpddsprimary,NULL);

	// get the pixel format of the primary surface
	DDPIXELFORMAT ddpf; // used to get pixel format

	// initialize structure
	DDRAW_INIT_STRUCT(ddpf);

	// query the format from primary surface
	lpddsprimary->GetPixelFormat(&ddpf);

	// based on masks determine if system is 5.6.5 or 5.5.5
	//RGB Masks for 5.6.5 mode
	//DDPF_RGB  16 R: 0x0000F800  
	//             G: 0x000007E0  
	//             B: 0x0000001F  

	//RGB Masks for 5.5.5 mode
	//DDPF_RGB  16 R: 0x00007C00  
	//             G: 0x000003E0  
	//             B: 0x0000001F  
	// test for 6 bit green mask)
	//if (ddpf.dwGBitMask == 0x000007E0)
	//   dd_pixel_format = DD_PIXEL_FORMAT565;

	// use number of bits, better method
	dd_pixel_format = ddpf.dwRGBBitCount;

	_lights_mgr = new LightsMgr;

	_materials_mgr = new MaterialsMgr;

	Modules::GetLog().WriteError("\npixel format = %d",dd_pixel_format);

	// set up conversion macros, so you don't have to test which one to use
	if (dd_pixel_format == DD_PIXEL_FORMAT555)
	{
		RGB16Bit = RGB16Bit555;
		Modules::GetLog().WriteError("\npixel format = 5.5.5");
	}
	else if (dd_pixel_format == DD_PIXEL_FORMAT565)
	{
		RGB16Bit = RGB16Bit565;
		Modules::GetLog().WriteError("\npixel format = 5.6.5");
	}
	else
	{
		RGB32Bit = RGB32Bit888;
		Modules::GetLog().WriteError("\npixel format = 8.8.8");
	}

	// only need a backbuffer for fullscreen modes
	if (!screen_windowed)
	{
		// test for backbuffer enabled
		if (screen_backbuffer_enable)
		{
			// query for the backbuffer i.e the secondary surface
			ddscaps.dwCaps = DDSCAPS_BACKBUFFER;

			if (FAILED(lpddsprimary->GetAttachedSurface(&ddscaps,&lpddsback)))
				return(0);
		}
		else
		{
			// must be requesting this for high performance alpha blending
			lpddsback = CreateSurface(width, height, DDSCAPS_SYSTEMMEMORY); // int mem_flags, USHORT color_key_flag);
		}
	}
	else
	{
		// must be windowed, so create a double buffer that will be blitted
		// rather than flipped as in full screen mode
		lpddsback = CreateSurface(width, height, DDSCAPS_SYSTEMMEMORY); // int mem_flags, unsigned short color_key_flag);
	}

	// create a palette only if 8bit mode
	if (screen_bpp==DD_PIXEL_FORMAT8)
	{
		// create and attach palette
		// clear all entries, defensive programming
		memset(palette,0,MAX_COLORS_PALETTE*sizeof(PALETTEENTRY));

		// load a pre-made "good" palette off disk
		LoadPaletteFromFile(DEFAULT_PALETTE_FILE, palette);

		// load and attach the palette, test for windowed mode
		if (screen_windowed)
		{
			// in windowed mode, so the first 10 and last 10 entries have
			// to be slightly modified as does the call to createpalette
			// reset the peFlags bit to PC_EXPLICIT for the "windows" colors
			for (index=0; index < 10; index++)
				palette[index].peFlags = palette[index+246].peFlags = PC_EXPLICIT;         

			// now create the palette object, but disable access to all 256 entries
			if (FAILED(lpdd->CreatePalette(DDPCAPS_8BIT | DDPCAPS_INITIALIZE,
				palette,&lpddpal,NULL)))
				return(0);
		}
		else
		{
			// in fullscreen mode, so simple create the palette with the default palette
			// and fill in all 256 entries
			if (FAILED(lpdd->CreatePalette(DDPCAPS_8BIT | DDPCAPS_INITIALIZE | DDPCAPS_ALLOW256,
				palette,&lpddpal,NULL)))
				return(0);
		}

		// now attach the palette to the primary surface
		if (FAILED(lpddsprimary->SetPalette(lpddpal)))
			return(0);

	} // end if attach palette for 8bit mode

	// clear out both primary and secondary surfaces
	if (screen_windowed)
	{
		// only clear backbuffer
		FillSurface(lpddsback,0);
	}
	else
	{
		// fullscreen, simply clear everything
		FillSurface(lpddsprimary,0);
		FillSurface(lpddsback,0);
	}

	// set software algorithmic clipping region
	min_clip_x = 0;
	max_clip_x = screen_width - 1;
	min_clip_y = 0;
	max_clip_y = screen_height - 1;

	// setup backbuffer clipper always
	RECT screen_rect = {0,0,screen_width,screen_height};
	lpddclipper = AttachClipper(lpddsback,1,&screen_rect);

	// set up windowed mode clipper
	if (screen_windowed)
	{
		// set windowed clipper
		if (FAILED(lpdd->CreateClipper(0,&lpddclipperwin,NULL)))
			return(0);

		if (FAILED(lpddclipperwin->SetHWnd(0, handle)))
			return(0);

		if (FAILED(lpddsprimary->SetClipper(lpddclipperwin)))
			return(0);
	}

	return(1);
}

int Graphics::Shutdown()
{
	// this function release all the resources directdraw
	// allocated, mainly to com objects

	delete _lights_mgr, _lights_mgr = NULL;
	delete _materials_mgr, _materials_mgr = NULL;

	// release the clippers first
	if (lpddclipper)
		lpddclipper->Release();

	if (lpddclipperwin)
		lpddclipperwin->Release();

	// release the palette if there is one
	if (lpddpal)
		lpddpal->Release();

	// release the secondary surface
	if (lpddsback)
		lpddsback->Release();

	// release the primary surface
	if (lpddsprimary)
		lpddsprimary->Release();

	// finally, the main dd object
	if (lpdd)
		lpdd->Release();

	return(1);
}

LPDIRECTDRAWCLIPPER 
Graphics::AttachClipper(LPDIRECTDRAWSURFACE7 lpdds, 
							   int num_rects,
							   LPRECT clip_list)
{
	// this function creates a clipper from the sent clip list and attaches
	// it to the sent surface

	int index;                         // looping var
	LPDIRECTDRAWCLIPPER lpddclipper;   // pointer to the newly created dd clipper
	LPRGNDATA region_data;             // pointer to the region data that contains
	// the header and clip list

	// first create the direct draw clipper
	if (FAILED(lpdd->CreateClipper(0,&lpddclipper,NULL)))
		return(NULL);

	// now create the clip list from the sent data

	// first allocate memory for region data
	region_data = (LPRGNDATA)malloc(sizeof(RGNDATAHEADER)+num_rects*sizeof(RECT));

	// now copy the rects into region data
	memcpy(region_data->Buffer, clip_list, sizeof(RECT)*num_rects);

	// set up fields of header
	region_data->rdh.dwSize          = sizeof(RGNDATAHEADER);
	region_data->rdh.iType           = RDH_RECTANGLES;
	region_data->rdh.nCount          = num_rects;
	region_data->rdh.nRgnSize        = num_rects*sizeof(RECT);

	region_data->rdh.rcBound.left    =  64000;
	region_data->rdh.rcBound.top     =  64000;
	region_data->rdh.rcBound.right   = -64000;
	region_data->rdh.rcBound.bottom  = -64000;

	// find bounds of all clipping regions
	for (index=0; index<num_rects; index++)
	{
		// test if the next rectangle unioned with the current bound is larger
		if (clip_list[index].left < region_data->rdh.rcBound.left)
			region_data->rdh.rcBound.left = clip_list[index].left;

		if (clip_list[index].right > region_data->rdh.rcBound.right)
			region_data->rdh.rcBound.right = clip_list[index].right;

		if (clip_list[index].top < region_data->rdh.rcBound.top)
			region_data->rdh.rcBound.top = clip_list[index].top;

		if (clip_list[index].bottom > region_data->rdh.rcBound.bottom)
			region_data->rdh.rcBound.bottom = clip_list[index].bottom;
	}

	// now we have computed the bounding rectangle region and set up the data
	// now let's set the clipping list

	if (FAILED(lpddclipper->SetClipList(region_data, 0)))
	{
		// release memory and return error
		free(region_data);
		return(NULL);
	}

	// now attach the clipper to the surface
	if (FAILED(lpdds->SetClipper(lpddclipper)))
	{
		// release memory and return error
		free(region_data);
		return(NULL);
	}

	// all is well, so release memory and send back the pointer to the new clipper
	free(region_data);
	return(lpddclipper);
}

LPDIRECTDRAWSURFACE7 
Graphics::CreateSurface(int width, int height, 
							   int mem_flags, unsigned short color_key_value)
{
	// this function creates an offscreen plain surface

	DDSURFACEDESC2 ddsd;         // working description
	LPDIRECTDRAWSURFACE7 lpdds;  // temporary surface

	// set to access caps, width, and height
	memset(&ddsd,0,sizeof(ddsd));
	ddsd.dwSize  = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;

	// set dimensions of the new bitmap surface
	ddsd.dwWidth  =  width;
	ddsd.dwHeight =  height;

	// set surface to offscreen plain
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | mem_flags;

	// create the surface
	if (FAILED(lpdd->CreateSurface(&ddsd,&lpdds,NULL)))
		return(NULL);

	// set color key to default color 000
	// note that if this is a 8bit bob then palette index 0 will be 
	// transparent by default
	// note that if this is a 16bit bob then RGB value 000 will be 
	// transparent
	DDCOLORKEY color_key; // used to set color key
	color_key.dwColorSpaceLowValue  = color_key_value;
	color_key.dwColorSpaceHighValue = color_key_value;

	// now set the color key for source blitting
	lpdds->SetColorKey(DDCKEY_SRCBLT, &color_key);

	return(lpdds);
}

int Graphics::Flip(HWND handle)
{
	// this function flip the primary surface with the secondary surface

	// test if either of the buffers are locked
	if (primary_buffer || back_buffer)
		return(0);

	// flip pages
	if (!screen_windowed)
	{
		// test for backbuffer enabled
		if (screen_backbuffer_enable)
		{
			while(FAILED(lpddsprimary->Flip(NULL, DDFLIP_WAIT)));
		}
		else
		{
			// must have an overrided off screen memory backbuffer for alpha blending speed
			RECT    dest_rect;    // used to compute destination rectangle

			// get the window's rectangle in screen coordinates
			GetWindowRect(handle, &dest_rect);   

			// compute the destination rectangle
			dest_rect.left   +=window_client_x0;
			dest_rect.top    +=window_client_y0;

			dest_rect.right  =dest_rect.left+screen_width-1;
			dest_rect.bottom =dest_rect.top +screen_height-1;

			// clip the screen coords 

			// blit the entire back surface to the primary
			if (FAILED(lpddsprimary->Blt(&dest_rect, lpddsback,NULL,DDBLT_WAIT,NULL)))
				return(0); 

		} // end else
	} // end if
	else
	{
		RECT    dest_rect;    // used to compute destination rectangle

		// get the window's rectangle in screen coordinates
		GetWindowRect(handle, &dest_rect);   

		// compute the destination rectangle
		dest_rect.left   +=window_client_x0;
		dest_rect.top    +=window_client_y0;

		dest_rect.right  =dest_rect.left+screen_width-1;
		dest_rect.bottom =dest_rect.top +screen_height-1;

		// clip the screen coords 

		// blit the entire back surface to the primary
		if (FAILED(lpddsprimary->Blt(&dest_rect, lpddsback,NULL,DDBLT_WAIT,NULL)))
			return(0);    
	}

	return(1);
}

int Graphics::WaitForVsync()
{
	// this function waits for a vertical blank to begin

	lpdd->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN,0);

	return(1);
}

int Graphics::FillSurface(LPDIRECTDRAWSURFACE7 lpdds, unsigned short color, RECT *client)
{
	DDBLTFX ddbltfx; // this contains the DDBLTFX structure

	// clear out the structure and set the size field 
	DDRAW_INIT_STRUCT(ddbltfx);

	// set the dwfillcolor field to the desired color
	ddbltfx.dwFillColor = color; 

	// ready to blt to surface
	lpdds->Blt(client,     // ptr to dest rectangle
		NULL,       // ptr to source surface, NA            
		NULL,       // ptr to source rectangle, NA
		DDBLT_COLORFILL | DDBLT_WAIT,   // fill and wait                   
		&ddbltfx);  // ptr to DDBLTFX structure

	return(1);
}

unsigned char* Graphics::LockSurface(LPDIRECTDRAWSURFACE7 lpdds, int *lpitch)
{
	// this function locks the sent surface and returns a pointer to it

	// is this surface valid
	if (!lpdds)
		return(NULL);

	// lock the surface
	DDRAW_INIT_STRUCT(ddsd);
	lpdds->Lock(NULL,&ddsd,DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR,NULL); 

	// set the memory pitch
	if (lpitch)
		*lpitch = ddsd.lPitch;

	// return pointer to surface
	return((unsigned char *)ddsd.lpSurface);
}

int Graphics::UnlockSurface(LPDIRECTDRAWSURFACE7 lpdds)
{
	// this unlocks a general surface

	// is this surface valid
	if (!lpdds)
		return(0);

	// unlock the surface memory
	lpdds->Unlock(NULL);

	return(1);
}

unsigned char* Graphics::LockPrimarySurface()
{
	// this function locks the priamary surface and returns a pointer to it
	// and updates the global variables primary_buffer, and primary_lpitch

	// is this surface already locked
	if (primary_buffer)
	{
		// return to current lock
		return(primary_buffer);
	}

	// lock the primary surface
	DDRAW_INIT_STRUCT(ddsd);
	lpddsprimary->Lock(NULL,&ddsd,DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR,NULL); 

	// set globals
	primary_buffer = (unsigned char *)ddsd.lpSurface;
	primary_lpitch = ddsd.lPitch;

	return(primary_buffer);
}

int Graphics::UnlockPrimarySurface()
{
	// is this surface valid
	if (!primary_buffer)
		return(0);

	// unlock the primary surface
	lpddsprimary->Unlock(NULL);

	// reset the primary surface
	primary_buffer = NULL;
	primary_lpitch = 0;

	return(1);
}

unsigned char* Graphics::LockBackSurface()
{
	// this function locks the secondary back surface and returns a pointer to it
	// and updates the global variables secondary buffer, and back_lpitch

	// is this surface already locked
	if (back_buffer)
	{
		// return to current lock
		return(back_buffer);
	}

	// lock the primary surface
	DDRAW_INIT_STRUCT(ddsd);
	lpddsback->Lock(NULL,&ddsd,DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR,NULL); 

	// set globals
	back_buffer = (unsigned char *)ddsd.lpSurface;
	back_lpitch = ddsd.lPitch;

	return(back_buffer);
}

int Graphics::UnlockBackSurface()
{
	// this unlocks the secondary

	// is this surface valid
	if (!back_buffer)
		return(0);

	// unlock the secondary surface
	lpddsback->Unlock(NULL);

	// reset the secondary surface
	back_buffer = NULL;
	back_lpitch = 0;

	return(1);
}

int Graphics::LoadPaletteFromFile(char *filename, LPPALETTEENTRY palette)
{
	// this function loads a palette from disk into a palette
	// structure, but does not set the pallette

	FILE *fp_file; // working file

	// try and open file
	if ((fp_file = fopen(filename,"r"))==NULL)
		return(0);

	// read in all 256 colors RGBF
	for (int index=0; index<MAX_COLORS_PALETTE; index++)
	{
		// read the next entry in
		fscanf(fp_file,"%d %d %d %d",&palette[index].peRed,
			&palette[index].peGreen,
			&palette[index].peBlue,                                
			&palette[index].peFlags);
	}

	fclose(fp_file);

	return(1);
}

int Graphics::DrawTextGDI(char *text, int x,int y,COLORREF color, LPDIRECTDRAWSURFACE7 lpdds)
{
	// this function draws the sent text on the sent surface 
	// using color index as the color in the palette

	HDC xdc; // the working dc

	// get the dc from surface
	if (FAILED(lpdds->GetDC(&xdc)))
		return(0);

	// set the colors for the text up
	SetTextColor(xdc,color);

	// set background mode to transparent so black isn't copied
	SetBkMode(xdc, TRANSPARENT);

	// draw the text a
	TextOut(xdc,x,y,text,strlen(text));

	// release the dc
	lpdds->ReleaseDC(xdc);

	return(1);
}

}