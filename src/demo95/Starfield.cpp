#include "Starfield.h"

#include "context.h"
#include "Modules.h"

namespace raider3d {

void Starfield::Init()
{
	// create the starfield
	for (int index=0; index < NUM_STARS; index++)
	{
		// randomly position stars in an elongated cylinder stretching from
		// the viewpoint 0,0,-d to the yon clipping plane 0,0,far_z
		stars[index].x = -WINDOW_WIDTH/2  + rand()%WINDOW_WIDTH;
		stars[index].y = -WINDOW_HEIGHT/2 + rand()%WINDOW_HEIGHT;
		stars[index].z = NEAR_Z + rand()%(FAR_Z - NEAR_Z);

		// set color of stars
		stars[index].color = rgb_white;
	} // end for index

}

void Starfield::Move()
{
	// move the stars

	int index; // looping var

	// the stars are technically stationary,but we are going
	// to move them to simulate motion of the viewpoint
	for (index=0; index<NUM_STARS; index++)
	{
		// move the next star
		stars[index].z-=(player_z_vel+3*diff_level);

		// test for past near clipping plane
		if (stars[index].z <= NEAR_Z)
			stars[index].z = FAR_Z;

	} // end for index

}

void Starfield::Draw() const
{
	// draw the stars in 3D using perspective transform

	t3d::Graphics& graphics = t3d::Modules::GetGraphics();

	unsigned char* video_buffer = graphics.GetBackBuffer();
	int lpitch = graphics.GetBackLinePitch();

	int index; // looping var

	// convert pitch to 2 words
	lpitch >>= 2;

	// draw each star
	for (index=0; index < NUM_STARS; index++)
	{
		// draw the next star
		// step 1: perspective transform
		float x_per = VIEW_DISTANCE*stars[index].x/stars[index].z;
		float y_per = VIEW_DISTANCE*stars[index].y/stars[index].z;

		// step 2: compute screen coords
		int x_screen = WINDOW_WIDTH/2  + x_per;
		int y_screen = WINDOW_HEIGHT/2 - y_per;

		// clip to screen coords
		if (x_screen>=WINDOW_WIDTH || x_screen < 0 || 
			y_screen >= WINDOW_HEIGHT || y_screen < 0)
		{
			// continue to next star
			continue; 
		} // end if
		else
		{
			// else render to buffer

			int i = ((float)(10000*NEAR_Z) / stars[index].z);
			if (i > 255) i = 255;

			((unsigned int*)video_buffer)[x_screen + y_screen*lpitch] 
				= graphics.GetColor(i,i,i);
		} // end else

	} // end for index
}

}