#pragma once

#include <ddraw.h>

namespace t3d {

// initializes a direct draw struct, basically zeros it and sets the dwSize field
#define DDRAW_INIT_STRUCT(ddstruct) { memset(&ddstruct,0,sizeof(ddstruct)); ddstruct.dwSize=sizeof(ddstruct); }

// this builds extract the RGB components of a 16 bit color value in 5.5.5 format (1-bit alpha mode)
#define _RGB555FROM16BIT(RGB, r,g,b) { *r = ( ((RGB) >> 10) & 0x1f); *g = (((RGB) >> 5) & 0x1f); *b = ( (RGB) & 0x1f); }

// this extracts the RGB components of a 16 bit color value in 5.6.5 format (green dominate mode)
#define _RGB565FROM16BIT(RGB, r,g,b) { *r = ( ((RGB) >> 11) & 0x1f); *g = (((RGB) >> 5) & 0x3f); *b = ((RGB) & 0x1f); }

// this extracts the ARGB components of a 32 bit color value in 8.8.8.8 format (8-bit alpha mode)
#define _RGB8888FROM32BIT(ARGB, a,r,g,b) { *a = ( ((ARGB) >> 24) & 0xff); *r = ( ((ARGB) >> 16) & 0xff); *g = (((ARGB) >> 8) & 0xff); *b = ((ARGB) & 0xff); }

// this builds a 16 bit color value in 5.5.5 format (1-bit alpha mode)
#define _RGB16BIT555(r,g,b) ((b & 31) + ((g & 31) << 5) + ((r & 31) << 10))

// this builds a 16 bit color value in 5.6.5 format (green dominate mode)
#define _RGB16BIT565(r,g,b) ((b & 31) + ((g & 63) << 5) + ((r & 31) << 11))

// this builds a 24 bit color value in 8.8.8 format 
#define _RGB24BIT(a,r,g,b) ((b) + ((g) << 8) + ((r) << 16) )

// this builds a 32 bit color value in A.8.8.8 format (8-bit alpha mode)
#define _RGB32BIT(a,r,g,b) ((b) + ((g) << 8) + ((r) << 16) + ((a) << 24))

class LightsMgr;
class MaterialsMgr;

class Graphics
{
public:
	static const int MAX_COLORS_PALETTE = 256;

public:
	Graphics();

	int Init(HWND handle, int width, int height, int bpp, int windowed, int backbuffer_enable = true);
	int Shutdown();

	LPDIRECTDRAWCLIPPER AttachClipper(LPDIRECTDRAWSURFACE7 lpdds, int num_rects, LPRECT clip_list);
	LPDIRECTDRAWSURFACE7 CreateSurface(int width, int height, int mem_flags=0, unsigned short color_key_value=0);

	int Flip(HWND handle);
	int WaitForVsync();
	int FillSurface(LPDIRECTDRAWSURFACE7 lpdds, unsigned short color, RECT *client=NULL);
	unsigned char *LockSurface(LPDIRECTDRAWSURFACE7 lpdds,int *lpitch);
	int UnlockSurface(LPDIRECTDRAWSURFACE7 lpdds);
	unsigned char *LockPrimarySurface();
	int UnlockPrimarySurface();
	unsigned char *LockBackSurface();
	int UnlockBackSurface();

	int DrawTextGDI(char *text, int x,int y,COLORREF color, LPDIRECTDRAWSURFACE7 lpdds);

	void GetClipValue(int& xmin, int& xmax, int& ymin, int& ymax) const {
		xmin = min_clip_x;
		xmax = max_clip_x;
		ymin = min_clip_y;
		ymax = max_clip_y;
	}

	LPDIRECTDRAW7 GetDraw() const { return lpdd; }
	LPDIRECTDRAWSURFACE7 GetBackSurface() const { return lpddsback; }
	LPDIRECTDRAWSURFACE7 GetFrontSurface() const { return lpddsprimary; }

	unsigned char* GetBackBuffer() { return back_buffer; }
	int GetBackLinePitch() const { return back_lpitch; }

//	LPDIRECTDRAWSURFACE7 GetPrimarySurface() const { return lpddsprimary; }

	int GetColor(int r, int g, int b) const {
		if (RGB16Bit) return RGB16Bit(r, g, b);
		else if (RGB32Bit) return RGB32Bit(r, g, b);
		else return 0;
	}

	int GetDDPixelFormat() const { return dd_pixel_format; }

	LightsMgr& GetLights() const { return *_lights_mgr; }
	MaterialsMgr& GetMaterials() const { return *_materials_mgr; }

private:
	// palette functions
	int LoadPaletteFromFile(char *filename, LPPALETTEENTRY palette);

private:
	// directdraw pixel format defines, used to help
	// bitmap loader put data in proper format
	#define DD_PIXEL_FORMAT8        8
	#define DD_PIXEL_FORMAT555      15
	#define DD_PIXEL_FORMAT565      16
	#define DD_PIXEL_FORMAT888      24
	#define DD_PIXEL_FORMATALPHA888 32 

private:
	// notice that interface 7.0 is used on a number of interfaces
	LPDIRECTDRAW7        lpdd;				// dd object
	LPDIRECTDRAWSURFACE7 lpddsprimary;		// dd primary surface
	LPDIRECTDRAWSURFACE7 lpddsback;			// dd back surface
	LPDIRECTDRAWPALETTE  lpddpal;			// a pointer to the created dd palette
	LPDIRECTDRAWCLIPPER  lpddclipper;		// dd clipper for back surface
	LPDIRECTDRAWCLIPPER  lpddclipperwin;	// dd clipper for window

	PALETTEENTRY         palette[MAX_COLORS_PALETTE];         // color palette
	PALETTEENTRY         save_palette[MAX_COLORS_PALETTE];    // used to save palettes
	DDSURFACEDESC2       ddsd;                 // a direct draw surface description struct
	DDBLTFX              ddbltfx;              // used to fill
	DDSCAPS2             ddscaps;              // a direct draw surface capabilities struct
	HRESULT              ddrval;               // result back from dd calls
	unsigned char        *primary_buffer;      // primary video buffer
	unsigned char        *back_buffer;         // secondary back buffer
	int                  primary_lpitch;       // memory line pitch for primary buffer
	int                  back_lpitch;          // memory line pitch for back buffer

	// clipping rectangle 
	int min_clip_x;
	int max_clip_x;
	int min_clip_y;
	int max_clip_y;

	int screen_width;
	int screen_height;
	int screen_bpp;	// bits per pixel
	int screen_windowed;
	bool screen_backbuffer_enable;

	int dd_pixel_format;

	int window_client_x0;   // used to track the starting (x,y) client area for
	int window_client_y0;   // for windowed mode directdraw operations

	LightsMgr* _lights_mgr;

	MaterialsMgr* _materials_mgr;

	// function ptr to RGB16 builder
	unsigned short (*RGB16Bit)(int r, int g, int b);
	// function ptr to RGB32 builder
	unsigned int (*RGB32Bit)(int r, int g, int b);

}; // Graphics

}