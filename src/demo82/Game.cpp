#include "Game.h"

#include "Modules.h"
#include "Log.h"
#include "Graphics.h"
#include "BmpFile.h"
#include "BmpImg.h"
#include "Input.h"
#include "Sound.h"
#include "Timer.h"

#define DEMO82B

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
		_textures[itext] = new BmpImg((WINDOW_WIDTH/2)-4*(TEXTSIZE/2),(WINDOW_HEIGHT/2)-2*(TEXTSIZE/2),TEXTSIZE,TEXTSIZE,32);
		_textures[itext]->LoadImage32(*bmpfile,itext % 4,itext / 4,BITMAP_EXTRACT_MODE_CELL);
	}

	// create temporary working texture (load with first texture to set loaded flags)
	_temp_text = new BmpImg((WINDOW_WIDTH/2)-(TEXTSIZE/2),(WINDOW_HEIGHT/2)+(TEXTSIZE/2),TEXTSIZE,TEXTSIZE,32);
	_temp_text->LoadImage32(*bmpfile,0,0,BITMAP_EXTRACT_MODE_CELL);

	// done, so unload the bitmap
	delete bmpfile, bmpfile = NULL;

	// now load the lightmaps

	// load in the textures
	bmpfile = new BmpFile("../../assets/chap08/LIGHTMAPS128_24.BMP");

	// now extract each 128x128x16 texture from the image
	for (int itext = 0; itext < NUM_LMAP; itext++)
	{     
		// create the bitmap
		_lightmaps[itext] = new BmpImg((WINDOW_WIDTH/2)+2*(TEXTSIZE/2),(WINDOW_HEIGHT/2)-2*(TEXTSIZE/2),TEXTSIZE,TEXTSIZE,32);
		_lightmaps[itext]->LoadImage32(*bmpfile,itext % 4,itext / 4,BITMAP_EXTRACT_MODE_CELL);
	}

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
	static int   curr_texture  = 7;   // currently active texture
	static int   curr_lightmap = 1;   // current light map
#ifdef DEMO82B
	static float scalef        = .25; // texture ambient scale factor
#else
	static float scalef        = .5; // texture ambient scale factor
#endif // DEMO82B

	Graphics& graphics = Modules::GetGraphics();

	// start the timing clock
	Modules::GetTimer().Start_Clock();

	// clear the drawing surface
	graphics.FillSurface(graphics.GetBackSurface(), 0);

	// lock back buffer and copy background into it
	graphics.LockBackSurface();

	///////////////////////////////////////////
	// our little image processing algorithm :)

	// Pixel_dest[x,y]rgb = pixel_source[x,y]rgb * ambient + 
	//                      pixel_source[x,y]rgb * light_map[x,y]rgb

	unsigned int *sbuffer = (unsigned int *)_textures[curr_texture]->Buffer();
	unsigned int *lbuffer = (unsigned int *)_lightmaps[curr_lightmap]->Buffer();
	unsigned int *tbuffer = (unsigned int *)_temp_text->Buffer();

	// perform RGB transformation on bitmap
	for (int iy = 0; iy < _temp_text->Height(); iy++) {
		for (int ix = 0; ix < _temp_text->Width(); ix++) {
			int as,rs,gs,bs;   // used to extract the source rgb values
			int al,rl, gl, bl; // light map rgb values
			int rf,gf,bf;   // the final rgb terms

			// extract pixel from source bitmap
			unsigned int spixel = sbuffer[iy*_temp_text->Width() + ix];

			// extract ARGB values
			_RGB8888FROM32BIT(spixel, &as,&rs,&gs,&bs);

			// extract pixel from lightmap bitmap
			unsigned int lpixel = lbuffer[iy*_temp_text->Width() + ix];

			// extract ARGB values
			_RGB8888FROM32BIT(lpixel, &al,&rl,&gl,&bl);

#ifdef DEMO82B
			// simple formula base + scale * lightmap
			rf = ( scalef * (float)rl ) + ( (float)rs );
			gf = ( scalef * (float)gl ) + ( (float)gs );
			bf = ( scalef * (float)bl ) + ( (float)bs );
#else
			// ambient base texture term + modulation term
			rf = ( scalef * (float)rs ) + ( (float)rs*(float)rl/(float)64 );
			gf = ( scalef * (float)gs ) + ( (float)gs*(float)rl/(float)64 );
			bf = ( scalef * (float)bs ) + ( (float)bs*(float)rl/(float)64 );
#endif // DEMO82B

			// test for overflow
			if (rf > 255) rf=255;
			if (gf > 255) gf=255;
			if (bf > 255) bf=255;

			// rebuild RGB and test for overflow
			// and write back to buffer
			tbuffer[iy*_temp_text->Width() + ix] = _RGB32BIT(255,rf,gf,bf);
		}
	}

	////////////////////////////////////////

	// draw textures
	_temp_text->Draw32(graphics.GetBackBuffer(), graphics.GetBackLinePitch(), 0);
	_textures[curr_texture]->Draw32(graphics.GetBackBuffer(), graphics.GetBackLinePitch(), 0);
	_lightmaps[curr_lightmap]->Draw32(graphics.GetBackBuffer(), graphics.GetBackLinePitch(), 0);

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

	// test if user wants to change ligthmap texture
	if (Modules::GetInput().KeyboardState()[DIK_UP])
	{
		if (++curr_lightmap > (NUM_LMAP-1))
			curr_lightmap = (NUM_LMAP-1);
		Modules::GetTimer().Wait_Clock(100);
	}

	if (Modules::GetInput().KeyboardState()[DIK_DOWN])
	{
		if (--curr_lightmap < 0)
			curr_lightmap = 0;
		Modules::GetTimer().Wait_Clock(100);
	}

	// is user changing scaling factor
	if (Modules::GetInput().KeyboardState()[DIK_PGUP])
	{
		scalef+=.01;
		if (scalef > 10)
			scalef = 10;
		Modules::GetTimer().Wait_Clock(100);
	}

	if (Modules::GetInput().KeyboardState()[DIK_PGDN])
	{
		scalef-=.01;
		if (scalef < 0)
			scalef = 0;
		Modules::GetTimer().Wait_Clock(100);
	}

	// draw title
	graphics.DrawTextGDI("Use <RIGHT>/<LEFT> arrows to change texture.",10,4,RGB(255,255,255), graphics.GetBackSurface());
	graphics.DrawTextGDI("Use <UP>/<DOWN> arrows to change the light map texture.",10,20,RGB(255,255,255), graphics.GetBackSurface());
	graphics.DrawTextGDI("Use <PAGE UP>/<PAGE DOWN> arrows to change ambient scale factor.",10, 36,RGB(255,255,255), graphics.GetBackSurface());
	graphics.DrawTextGDI("Press <ESC> to Exit. ",10, 56,RGB(255,255,255), graphics.GetBackSurface());

	// print stats
	sprintf(buffer,"Texture #%d, Ligtmap #%d, Ambient Scaling Factor: %f", curr_texture, curr_lightmap, scalef);
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