//#pragma comment(lib,"dxguid.lib")

#include "Game.h"

#include <WindowsX.h>
#include <dinput.h>

#include "demoii13_1res.h"
#include "BSPFile.h"
#include "context.h"

#include "Modules.h"
#include "Input.h"
#include "BSP.h"
#include "Camera.h"
#include "Graphics.h"

#define WINDOW_CLASS_NAME "WIN3DCLASS"  // class name
#define WINDOW_TITLE      "T3D Graphics Console Ver 2.0"

HWND main_window_handle           = NULL; // save the window handle
HINSTANCE main_instance           = NULL; // save the instance

char ascfilename[256]; // holds file name when loader loads

// windows gui functions
BOOL CALLBACK DialogProc(HWND hwnddlg,    // handle to dialog box
						 UINT umsg,       // message
						 WPARAM waram,    // first message parameter
						 LPARAM lparam);  // second message parameter

LRESULT CALLBACK WindowProc(HWND hwnd, 
							UINT msg, 
							WPARAM wparam, 
							LPARAM lparam)
{
	// this is the main message handler of the system
	PAINTSTRUCT	ps;		   // used in WM_PAINT
	HDC			hdc;	   // handle to a device context
	HMENU       hmenu;     // menu handle sending commands

	DIMOUSESTATE& mouse_state = t3d::Modules::GetInput().MouseState();

	bspeditor::Context* context = bspeditor::Context::Instance();

	// what is the message 
	switch(msg)
	{	
	case WM_CREATE: 
		{
			// do initialization stuff here
			return(0);
		} break;

	case WM_PAINT:
		{
			// start painting
			hdc = BeginPaint(hwnd,&ps);

			// end painting
			EndPaint(hwnd,&ps);
			return(0);
		} break;

		// pass mouse events out to globals
	case WM_MOUSEMOVE:
		{
			int fwkeys     = wparam;          // key flags 
			mouse_state.lX = LOWORD(lparam);  // horizontal position of cursor 
			mouse_state.lY = HIWORD(lparam);  // vertical position of cursor 
		} break;

	case WM_LBUTTONDOWN:
		{
			int fwkeys     = wparam;          // key flags 
			mouse_state.lX = LOWORD(lparam);  // horizontal position of cursor 
			mouse_state.lY = HIWORD(lparam);  // vertical position of cursor 
			mouse_state.rgbButtons[0] = 1;
		} break;

	case WM_MBUTTONDOWN:
		{
			int fwkeys     = wparam;          // key flags 
			mouse_state.lX = LOWORD(lparam);  // horizontal position of cursor 
			mouse_state.lY = HIWORD(lparam);  // vertical position of cursor 
			mouse_state.rgbButtons[1] = 1;
		} break;

	case WM_RBUTTONDOWN:
		{
			int fwkeys     = wparam;          // key flags 
			mouse_state.lX = LOWORD(lparam);  // horizontal position of cursor 
			mouse_state.lY = HIWORD(lparam);  // vertical position of cursor 
			mouse_state.rgbButtons[2] = 1;
		} break;

	case WM_COMMAND:
		{
			// get the menu that has generated this command
			hmenu = GetMenu(hwnd);

			// what is the id of the command
			switch(LOWORD(wparam))
			{
				// handle the FILE menu
			case ID_FILE_EXIT:
				{
					// terminate window
					PostQuitMessage(0);
				} break;

			case ID_FILE_LOAD_LEV:
				{
					// call load dialog and load .lev file
					if (DialogBox (main_instance, MAKEINTRESOURCE(IDD_DIALOG1), hwnd, DialogProc ) == IDOK )
					{
						context->bspfile->Load(ascfilename);
					} // end if

				} break;

			case ID_FILE_SAVE_LEV:
				{
					// call save dialog and load .lev file
					if (DialogBox (main_instance, MAKEINTRESOURCE(IDD_DIALOG2), hwnd, DialogProc ) == IDOK )
					{
						context->bspfile->Save(ascfilename,0);
					} // end if

				} break;

			case IDC_VIEWGRID:
				{
					// toggle viewing state
					context->view_grid = -context->view_grid;

					// toggle checkmark
					if (context->view_grid==1)
						CheckMenuItem(hmenu, IDC_VIEWGRID, MF_CHECKED);
					else
						CheckMenuItem(hmenu, IDC_VIEWGRID, MF_UNCHECKED);

				} break;

			case IDC_VIEWWALLS:
				{
					// toggle viewing state
					context->view_walls = -context->view_walls;

					// toggle checkmark
					if (context->view_walls==1)
						CheckMenuItem(hmenu, IDC_VIEWWALLS, MF_CHECKED);
					else
						CheckMenuItem(hmenu, IDC_VIEWWALLS, MF_UNCHECKED);

				} break;

			case IDC_VIEWFLOORS:
				{
					// toggle viewing state
					context->view_floors = -context->view_floors;

					// toggle checkmark
					if (context->view_floors==1)
						CheckMenuItem(hmenu, IDC_VIEWFLOORS, MF_CHECKED);
					else
						CheckMenuItem(hmenu, IDC_VIEWFLOORS, MF_UNCHECKED);

				} break;

			case ID_BUILD_TO_FILE:
				{
					// not fully implemented yet....

					// first delete any previous tree
					context->bspfile->bsp_root->Delete();

					// convert 2-d lines to 3-d walls
					context->bspfile->ConvertLinesToWalls();

					// build the 3-D bsp tree
					context->bspfile->wall_list->Build();

					// alias the bsp tree to the wall list first node
					// which has now become the root of the tree
					context->bspfile->bsp_root = context->bspfile->wall_list;

					// print out to error file
					context->bspfile->bsp_root->Print();

					// save .bsp to file

				} break;

			case ID_COMPILE_VIEW:
				{
					// compile bsp

					// first delete any previous tree
					if (context->bspfile->bsp_root)
						context->bspfile->bsp_root->Delete();

					// reset ceiling height
					// conversion will find heighest
					context->ceiling_height  = 0;

					// convert 2-d lines to 3-d walls
					context->bspfile->ConvertLinesToWalls();

					// build the 3-D bsp tree
					context->bspfile->wall_list->Build();

					// alias the bsp tree to the wall list first node
					// which has now become the root of the tree
					context->bspfile->bsp_root = context->bspfile->wall_list;

					// print out to error file
					context->bspfile->bsp_root->Print();

					// position camera at (0,0,0) and wall height
					context->cam->PosRef().x = 0;
					context->cam->PosRef().y = context->wall_height; 
					context->cam->PosRef().z = 0;

					// set heading and speed
					context->cam->DirRef().y = 0;
					context->cam_speed = 0;

					// set position of floor to (0,0,0)
					vec4 vpos(0,0,0,0);

					// generate the floor mesh object
					context->bspfile->GenerateFloorsObj(context->obj_scene, _RGB32BIT(255,255,255,255), &vpos, NULL,
						POLY_ATTR_DISABLE_MATERIAL | 
						POLY_ATTR_2SIDED |
						POLY_ATTR_RGB16 |
						POLY_ATTR_SHADE_MODE_TEXTURE |
						POLY_ATTR_SHADE_MODE_FLAT);

					// switch states to view state
					context->editor_state = EDITOR_STATE_VIEW;
				} break;

				// handle the HELP menu
			case ID_HELP_ABOUT:                 
				{
					//  pop up a message box
					MessageBox(hwnd, "Version 1.0", 
						"BSP Level Generator",
						MB_OK);
				} break;

			default: break;

			} // end switch wparam

		} break; // end WM_COMMAND

	case WM_DESTROY: 
		{
			// kill the application			
			PostQuitMessage(0);
			return(0);
		} break;

	default:break;

	} // end switch

	// process any messages that we didn't take care of 
	return (DefWindowProc(hwnd, msg, wparam, lparam));

} // end WinProc

BOOL CALLBACK DialogProc(HWND hwnddlg,  // handle to dialog box
						 UINT umsg,     // message
						 WPARAM wparam, // first message parameter
						 LPARAM lparam)  // second message parameter

{
	// dialog handler for the line input

	// get the handles to controls...
	HWND htextedit = GetDlgItem(hwnddlg, IDC_EDIT1);

	// options (not implemented yet...)
	HWND hoption1 = GetDlgItem(hwnddlg, IDC_OPTION1);
	HWND hoption2 = GetDlgItem(hwnddlg, IDC_OPTION2);
	HWND hoption3 = GetDlgItem(hwnddlg, IDC_OPTION3);
	HWND hoption4 = GetDlgItem(hwnddlg, IDC_OPTION4);

	switch (umsg)
	{
	case WM_INITDIALOG:
		{
			SetFocus ( htextedit );
		} break;

	case WM_COMMAND:
		{
			switch (LOWORD(wparam))
			{
			case IDOK:
				{
					// text has been entered, update gloabal string with filename
					int linelength = SendMessage ( htextedit, EM_LINELENGTH, 0, 0 );
					ascfilename[0] = (unsigned char )255;
					SendMessage (htextedit, EM_GETLINE, 0, (LPARAM) (LPCSTR) ascfilename );
					ascfilename[linelength] = 0;

					// get checks 
					SendMessage(hoption1, BM_GETCHECK, 0,0);
					SendMessage(hoption2, BM_GETCHECK, 0,0);
					SendMessage(hoption3, BM_GETCHECK, 0,0);
					SendMessage(hoption4, BM_GETCHECK, 0,0);

					// terminate dialog
					EndDialog (hwnddlg, IDOK );
					return TRUE;
				} break;

			case IDCANCEL:
				{
					// terminate dialog
					EndDialog ( hwnddlg, IDCANCEL );
					return TRUE;
				} break;

			default: break;
			} // end switch 

		} break;

	} // end switch

	// return normal
	return 0;

} // end DialogProc

// WINMAIN ////////////////////////////////////////////////

int WINAPI WinMain(	HINSTANCE hinstance,
				   HINSTANCE hprevinstance,
				   LPSTR lpcmdline,
				   int ncmdshow)
{
	// this is the winmain function

	WNDCLASS winclass;	// this will hold the class we create
	HWND	 hwnd;		// generic window handle
	MSG		 msg;		// generic message
	HDC      hdc;       // generic dc
	PAINTSTRUCT ps;     // generic paintstruct

	// first fill in the window class stucture
	winclass.style			= CS_DBLCLKS | CS_OWNDC | 
							  CS_HREDRAW | CS_VREDRAW;
	winclass.lpfnWndProc	= WindowProc;
	winclass.cbClsExtra		= 0;
	winclass.cbWndExtra		= 0;
	winclass.hInstance		= hinstance;
	winclass.hIcon			= LoadIcon(NULL, IDI_APPLICATION);
	winclass.hCursor		= LoadCursor(NULL, IDC_ARROW);
	winclass.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
	winclass.lpszMenuName	= MAKEINTRESOURCE(IDR_ASCMENU);
	winclass.lpszClassName	= WINDOW_CLASS_NAME;

	// register the window class
	if (!RegisterClass(&winclass))
		return(0);

	// create the window, note the test to see if WINDOWED_APP is
	// true to select the appropriate window flags
	if (!(hwnd = CreateWindow(WINDOW_CLASS_NAME, // class
								WINDOW_TITLE,	 // title
								(t3d::Game::WINDOWED_APP ? (WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION) : (WS_POPUP | WS_VISIBLE)),
								0,0,	   // x,y
								t3d::Game::WINDOW_WIDTH,  // width
								t3d::Game::WINDOW_HEIGHT, // height
								NULL,	   // handle to parent 
								NULL,	   // handle to menu
								hinstance,// instance
								NULL)))	// creation parms
								return(0);

	// save the window handle and instance in a global
	main_window_handle = hwnd;
	main_instance      = hinstance;

	// resize the window so that client is really width x height
	if (t3d::Game::WINDOWED_APP)
	{
		// now resize the window, so the client area is the actual size requested
		// since there may be borders and controls if this is going to be a windowed app
		// if the app is not windowed then it won't matter
		RECT window_rect = {0,0,t3d::Game::WINDOW_WIDTH-1,t3d::Game::WINDOW_HEIGHT-1};

		// make the call to adjust window_rect
		AdjustWindowRectEx(&window_rect,
			GetWindowStyle(hwnd),
			GetMenu(hwnd) != NULL,
			GetWindowExStyle(hwnd));

		// 		// save the global client offsets, they are needed in DDraw_Flip()
		// 		window_client_x0 = -window_rect.left;
		// 		window_client_y0 = -window_rect.top;

		// now resize the window with a call to MoveWindow()
		MoveWindow(hwnd,
			0, // x position
			0, // y position
			window_rect.right - window_rect.left, // width
			window_rect.bottom - window_rect.top, // height
			FALSE);

		// show the window, so there's no garbage on first render
		ShowWindow(hwnd, SW_SHOW);
	} // end if windowed

	// perform all game console specific initialization
	t3d::Game* game = new t3d::Game(hwnd, hinstance);
	game->Init();

	// disable CTRL-ALT_DEL, ALT_TAB, comment this line out 
	// if it causes your system to crash
	SystemParametersInfo(SPI_SCREENSAVERRUNNING, TRUE, NULL, 0);

	// enter main event loop
	while(1)
	{
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
		{ 
			// test if this is a quit
			if (msg.message == WM_QUIT)
				break;

			// translate any accelerator keys
			TranslateMessage(&msg);

			// send the message to the window proc
			DispatchMessage(&msg);
		} // end if

		// main game processing goes here
		game->Step();

	} // end while

	// shutdown game and release all resources
	game->Shutdown();

	// enable CTRL-ALT_DEL, ALT_TAB, comment this line out 
	// if it causes your system to crash
	SystemParametersInfo(SPI_SCREENSAVERRUNNING, FALSE, NULL, 0);

	// return to Windows like this
	return(msg.wParam);

} // end WinMain