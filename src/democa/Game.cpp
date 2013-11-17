#include "Game.h"

#include "Modules.h"
#include "Log.h"
#include "Graphics.h"
#include "Input.h"
#include "Sound.h"
#include "Music.h"
#include "Timer.h"
#include "defines.h"
#include "Camera.h"
#include "BmpFile.h"
#include "BmpImg.h"
#include "Mipmaps.h"

namespace t3d {

Game::Game(HWND handle, HINSTANCE instance)
	: main_window_handle(handle)
	, main_instance(instance)
{
}

Game::~Game()
{
}

void Game::Init()
{
	Modules::Init();

	Modules::GetLog().Init();

	Modules::GetGraphics().Init(main_window_handle,
		WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_BPP, WINDOWED_APP, true);

	Modules::GetInput().Init(main_instance);
	Modules::GetInput().InitKeyboard(main_window_handle);

	Modules::GetSound().Init(main_window_handle);
//	Modules::GetMusic().Init(main_window_handle);

	// hide the mouse
	if (!WINDOWED_APP)
		ShowCursor(FALSE);

	// seed random number generator
	srand(Modules::GetTimer().Start_Clock());

// 	// initialize math engine
// 	Build_Sin_Cos_Tables();

	BmpFile* bitmap = new BmpFile("../../assets/chap12/OMPTXT128_24.BMP");

	// now extract each 64x64x16 texture from the image
	for (int itext = 0; itext < NUM_TEXT; itext++)
	{     
		// create the bitmap
		textures[itext] = new BmpImg((WINDOW_WIDTH/2)-(TEXTSIZE/2),(WINDOW_HEIGHT/2)-(TEXTSIZE/2) - 64,TEXTSIZE,TEXTSIZE,32);
		textures[itext]->LoadImage32(*bitmap, itext % 4,itext / 4,BITMAP_EXTRACT_MODE_CELL);
	} // end for

	// create temporary working texture (load with first texture to set loaded flags)
	temp_text = new BmpImg((WINDOW_WIDTH/2)-(TEXTSIZE/2),(WINDOW_HEIGHT/2)-(TEXTSIZE/2) - 40,TEXTSIZE,TEXTSIZE,32);
	temp_text->LoadImage32(*bitmap,0,0,BITMAP_EXTRACT_MODE_CELL);

	delete bitmap;
}

void Game::Shutdown()
{
	Modules::GetSound().StopAllSounds();
	Modules::GetSound().DeleteAllSounds();
	Modules::GetSound().Shutdown();

// 	Modules::GetMusic().DeleteAllMIDI();
// 	Modules::GetMusic().Shutdown();

	Modules::GetInput().ReleaseKeyboard();
	Modules::GetInput().Shutdown();

	Modules::GetGraphics().Shutdown();

	Modules::GetLog().Shutdown();
}

void Game::Step()
{
	Modules::GetTimer().Step();

	// this is the workhorse of your game it will be called
	// continuously in real-time this is like main() in C
	// all the calls for you game go here!

	int          index;              // looping var
	static int   curr_texture = 0;   // currently active texture
	static float gamma        = 1.0; // mipmap gamma factor

	Graphics& graphics = Modules::GetGraphics();

	// start the timing clock
	Modules::GetTimer().Start_Clock();

	// clear the drawing surface
	graphics.FillSurface(graphics.GetBackSurface(), 0);

	// lock back buffer and copy background into it
	graphics.LockBackSurface();

	// copy texture into temp display texture for rendering and scaling
	BmpImg::Copy(temp_text,0,0,  textures[curr_texture],0,0,TEXTSIZE, TEXTSIZE);

#if 0

	///////////////////////////////////////////
	// our little image processing algorithm :)
	//Cmodulated = s*C1 = (s*r1, s*g1, s*b1)

	USHORT *pbuffer = (USHORT *)temp_text.buffer;
	// perform RGB transformation on bitmap
	for (int iy = 0; iy < temp_text.height; iy++)
		for (int ix = 0; ix < temp_text.width; ix++)
		{
			// extract pixel
			USHORT pixel = pbuffer[iy*temp_text.width + ix];

			int ri,gi,bi; // used to extract the rgb values

			// extract RGB values
			_RGB565FROM16BIT(pixel, &ri,&gi,&bi);

			// perform scaling operation and test for overflow
			if ((ri = (float)ri * scalef) > 255) ri=255;
			if ((gi = (float)gi * scalef) > 255) gi=255;
			if ((bi = (float)bi * scalef) > 255) bi=255;

			// rebuild RGB and test for overflow
			// and write back to buffer
			pbuffer[iy*temp_text.width + ix] = _RGB16BIT565(ri,gi,bi);

		} // end for ix     
#endif

		////////////////////////////////////////

		// generate the mipmaps on the fly
		GenerateMipmaps(*temp_text, (BmpImg**)&gmipmaps, gamma);

		// draw texture
		temp_text->Draw32(graphics.GetBackBuffer(), graphics.GetBackLinePitch(),0);

		// alias/cast an array of pointers to the storage which is single ptr
		BmpImg** tmipmaps = (BmpImg**)gmipmaps;

		// compute number of mip levels total
		int num_mip_levels = logbase2ofx[tmipmaps[0]->Width()] + 1; 

		// x position to display
		int xpos = (WINDOW_WIDTH/2) - (TEXTSIZE/2); 

		// iterate thru and draw each bitmap
		for (int mip_level = 1; mip_level < num_mip_levels; mip_level++)
		{ 
			// position bitmap
			tmipmaps[mip_level]->SetPos(xpos, (WINDOW_HEIGHT/2) + (TEXTSIZE));
			// draw bitmap
			tmipmaps[mip_level]->Draw32(graphics.GetBackBuffer(), graphics.GetBackLinePitch(),0);
			xpos+=tmipmaps[mip_level]->Width()+8;

		} // end for mip_level

		// delete the mipmap chain now that we are done this frame
		DeleteMipmaps((BmpImg**)&gmipmaps,1);

		// unlock back surface
		graphics.UnlockBackSurface();

		// read keyboard
		Modules::GetInput().ReadKeyboard();

		// test if user wants to change texture
		if (Modules::GetInput().KeyboardState()[DIK_RIGHT])
		{
			if (++curr_texture > (NUM_TEXT-1))
				curr_texture = (NUM_TEXT-1);

			Modules::GetTimer().Wait_Clock(100);
		} // end if

		if (Modules::GetInput().KeyboardState()[DIK_LEFT])
		{
			if (--curr_texture < 0)
				curr_texture = 0;

			Modules::GetTimer().Wait_Clock(100);
		} // end if

		// is user changing gamma factor
		if (Modules::GetInput().KeyboardState()[DIK_UP])
		{
			gamma+=.01;
			if (gamma > 10)
				gamma = 10;
			Modules::GetTimer().Wait_Clock(10);
		} // end if

		if (Modules::GetInput().KeyboardState()[DIK_DOWN])
		{
			gamma-=.01;
			if (gamma < 0)
				gamma = 0;

			Modules::GetTimer().Wait_Clock(10);
		} // end if

		int ty = 10;

		// draw title
		graphics.DrawTextGDI("Press <RIGHT>/<LEFT> arrows to change texture.",10, ty,RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("Press <UP>/<DOWN> arrows to change mipmap gamma factor.",10,ty+=16,RGB(255,255,255), graphics.GetBackSurface());
		graphics.DrawTextGDI("<ESC> to Exit. ",10, ty+=16,RGB(255,255,255), graphics.GetBackSurface());

		// print stats
		sprintf(buffer,"Texture #%d, Gamma Factor: %f", curr_texture, gamma);
		graphics.DrawTextGDI(buffer, 10, WINDOW_HEIGHT - 20,RGB(255,255,255), graphics.GetBackSurface());

		// flip the surfaces
		graphics.Flip(main_window_handle);

		// sync to 30ish fps 
		Modules::GetTimer().Wait_Clock(30);

		// check of user is trying to exit
		if (KEY_DOWN(VK_ESCAPE) || Modules::GetInput().KeyboardState()[DIK_ESCAPE])
		{
			PostMessage(main_window_handle, WM_DESTROY,0,0);

			// stop all sounds
			Modules::GetSound().StopAllSounds();

		} // end if
}

}
