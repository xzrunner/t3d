#include "Game.h"

#include "Modules.h"
#include "Log.h"
#include "Graphics.h"
#include "BmpFile.h"
#include "BmpImg.h"
#include "Input.h"
#include "Sound.h"
#include "Timer.h"

namespace t3d {

Game::Game(HWND handle, HINSTANCE instance)
	: main_window_handle(handle)
	, main_instance(instance)
{
}

void Game::Init()
{
	Modules::Init();

	Modules::GetLog().Init();

	// initialize directdraw, very important that in the call
	// to setcooperativelevel that the flag DDSCL_MULTITHREADED is used
	// which increases the response of directX graphics to
	// take the global critical section more frequently
	Modules::GetGraphics().Init(main_window_handle,
		WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_BPP, WINDOWED_APP);

	// load in the textures
	BmpFile* bmpfile = new BmpFile("../../assets/chap08/OMPTXT128_24.BMP");

	// now extract each 128x128x32 texture from the image
	for (int itext = 0; itext < NUM_TEXT; itext++)
	{     
		// create the bitmap
		_textures[itext] = new BmpImg((WINDOW_WIDTH/2)-(TEXTSIZE/2),(WINDOW_HEIGHT/2)-(TEXTSIZE/2),TEXTSIZE,TEXTSIZE,32);
		_textures[itext]->LoadImage32(*bmpfile,itext % 4,itext / 4,BITMAP_EXTRACT_MODE_CELL);
	}

	// create temporary working texture (load with first texture to set loaded flags)
	_temp_text = new BmpImg((WINDOW_WIDTH/2)-(TEXTSIZE/2),(WINDOW_HEIGHT/2)-(TEXTSIZE/2),TEXTSIZE,TEXTSIZE,32);
	_temp_text->LoadImage32(*bmpfile,0,0,BITMAP_EXTRACT_MODE_CELL);

	// done, so unload the bitmap
	delete bmpfile, bmpfile = NULL;

	// initialize directinput
	Modules::GetInput().Init(main_instance);

	// acquire the keyboard only
	Modules::GetInput().InitKeyboard(main_window_handle);

	// hide the mouse
	if (!WINDOWED_APP)
		ShowCursor(FALSE);

	// seed random number generate
	srand(Modules::GetTimer().Start_Clock());
}

void Game::Shutdown()
{
	// this function is where you shutdown your game and
	// release all resources that you allocated

	// shut everything down

	// shutdown directdraw last
	Modules::GetGraphics().Shutdown();

	// now directsound
	Modules::GetSound().StopAllSounds();
	Modules::GetSound().Shutdown();

	// shut down directinput
	Modules::GetInput().Shutdown();

	Modules::GetLog().Shutdown();
}

void Game::Step()
{
	// this is the workhorse of your game it will be called
	// continuously in real-time this is like main() in C
	// all the calls for you game go here!

	int          index;              // looping var
	static int   curr_texture = 0;   // currently active texture
	static float scalef       = 1.0; // texture scaling factor

	Graphics& graphics = Modules::GetGraphics();

	// start the timing clock
	Modules::GetTimer().Start_Clock();

	// clear the drawing surface
	graphics.FillSurface(graphics.GetBackSurface(), 0);

	// lock back buffer and copy background into it
	graphics.LockBackSurface();

	// copy texture into temp display texture for rendering and scaling
	BmpImg::Copy(_temp_text, 0, 0, _textures[curr_texture], 0, 0, TEXTSIZE, TEXTSIZE);

	///////////////////////////////////////////
	// our little image processing algorithm :)
	//Cmodulated = s*C1 = (s*r1, s*g1, s*b1)

	unsigned int *pbuffer = (unsigned int*)_temp_text->Buffer();

	// perform RGB transformation on bitmap
	for (int iy = 0; iy < _temp_text->Height(); iy++) {
		for (int ix = 0; ix < _temp_text->Width(); ix++) {
			// extract pixel
			unsigned int pixel = pbuffer[iy*_temp_text->Width() + ix];

			int ai,ri,gi,bi; // used to extract the rgb values

			// extract ARGB values
			_RGB8888FROM32BIT(pixel, &ai,&ri,&gi,&bi);

			// perform scaling operation and test for overflow
			if ((ri = (float)ri * scalef) > 255) ri=255;
			if ((gi = (float)gi * scalef) > 255) gi=255;
			if ((bi = (float)bi * scalef) > 255) bi=255;

			// rebuild RGB and test for overflow
			// and write back to buffer
			pbuffer[iy*_temp_text->Width() + ix] = _RGB32BIT(ai,ri,gi,bi);
		}
	}

	////////////////////////////////////////

	// draw texture
	_temp_text->Draw32(graphics.GetBackBuffer(), graphics.GetBackLinePitch(), 0);

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
	}

	if (Modules::GetInput().KeyboardState()[DIK_LEFT])
	{
		if (--curr_texture < 0)
			curr_texture = 0;
		Modules::GetTimer().Wait_Clock(100);
	}

	// is user changing scaling factor
	if (Modules::GetInput().KeyboardState()[DIK_UP])
	{
		scalef+=.01;
		if (scalef > 10)
			scalef = 10;
		Modules::GetTimer().Wait_Clock(10);
	}

	if (Modules::GetInput().KeyboardState()[DIK_DOWN])
	{
		scalef-=.01;
		if (scalef < 0)
			scalef = 0;
		Modules::GetTimer().Wait_Clock(10);
	}

	// draw title
	graphics.DrawTextGDI("Use <RIGHT>/<LEFT> arrows to change texture, <UP>/<DOWN> arrows to change scale factor.",10, 10,RGB(255,255,255), graphics.GetBackSurface());
	graphics.DrawTextGDI("Press <ESC> to Exit. ",10, 32,RGB(255,255,255), graphics.GetBackSurface());

	// print stats
	sprintf(buffer,"Texture #%d, Scaling Factor: %f", curr_texture, scalef);
	graphics.DrawTextGDI(buffer, 10, WINDOW_HEIGHT - 20,RGB(255,255,255), graphics.GetBackSurface());

	// flip the surfaces
	graphics.Flip(main_window_handle);

	// sync to 30ish fps
	Modules::GetTimer().Wait_Clock(30);

	if (KEY_DOWN(VK_ESCAPE) || 
		Modules::GetInput().KeyboardState()[DIK_ESCAPE])
	{
		PostMessage(main_window_handle, WM_DESTROY,0,0);
		Modules::GetSound().StopAllSounds();
	}
}

}