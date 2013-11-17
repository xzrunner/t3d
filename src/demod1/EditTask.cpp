#include "EditTask.h"

#include <dinput.h>

#include "PrimitiveDraw.h"
#include "Modules.h"
#include "Graphics.h"
#include "Input.h"
#include "BOB.h"

#include "Context.h"

namespace bspeditor {

EditTask::EditTask()
	: action(ACTION_STARTING_LINE)
{
}

void EditTask::DrawLines(unsigned char* video_buffer, int mempitch)
{
	BSPLine* lines = Context::Instance()->lines;

	int total_lines = Context::Instance()->total_lines;

	const DIMOUSESTATE& mouse_state = t3d::Modules::GetInput().MouseState();

	// this function draws all the lines for the BSP
	for (int index = 0; index < total_lines; index++)
	{
		// draw each line, textured lines red, blank lines blue

		if (lines[index].texture_id >= 0)
		{
			t3d::DrawLine32(lines[index].p0.x, lines[index].p0.y,
							lines[index].p1.x, lines[index].p1.y,
							_RGB32BIT(255,255,0,0), video_buffer, mempitch);
		} // end if
		else
		{
			t3d::DrawLine32(lines[index].p0.x, lines[index].p0.y,
							lines[index].p1.x, lines[index].p1.y,
							_RGB32BIT(255,128,128,128), video_buffer, mempitch);
		} // end if

		// mark endpoints with X
		t3d::DrawLine32(lines[index].p0.x-4, lines[index].p0.y-4,
						lines[index].p0.x+4, lines[index].p0.y+4,
						_RGB32BIT(255,0,255,0), video_buffer, mempitch);

		t3d::DrawLine32(lines[index].p0.x+4, lines[index].p0.y-4,
						lines[index].p0.x-4, lines[index].p0.y+4,
						_RGB32BIT(255,0,255,0), video_buffer, mempitch);

		t3d::DrawLine32(lines[index].p1.x-4, lines[index].p1.y-4,
						lines[index].p1.x+4, lines[index].p1.y+4,
						_RGB32BIT(255,0,255,0), video_buffer, mempitch);

		t3d::DrawLine32(lines[index].p1.x+4, lines[index].p1.y-4,
						lines[index].p1.x-4, lines[index].p1.y+4,
						_RGB32BIT(255,0,255,0), video_buffer, mempitch);

	} // end for index

	// now draw special lines
	if (action == ACTION_ENDING_LINE &&
		mouse_state.lX > BSP_MIN_X && mouse_state.lX < BSP_MAX_X && 
		mouse_state.lY > BSP_MIN_Y && mouse_state.lY < BSP_MAX_Y)
	{
		t3d::DrawLine32(lines[total_lines].p0.x, lines[total_lines].p0.y,
						mouse_state.lX, mouse_state.lY,
						_RGB32BIT(255,255,255,255), video_buffer, mempitch);

	} // end if

	// hightlight path to end point
	if (action == ACTION_ADDING_POLYLINE &&
		mouse_state.lX > BSP_MIN_X && mouse_state.lX < BSP_MAX_X && 
		mouse_state.lY > BSP_MIN_Y && mouse_state.lY < BSP_MAX_Y)
	{
		t3d::DrawLine32(lines[total_lines].p0.x, lines[total_lines].p0.y,
						mouse_state.lX, mouse_state.lY,
						_RGB32BIT(255,255,255,255), video_buffer, mempitch);

	} // end if

	static int red_counter = 0; // used to flicker delete candidate 
	if ((red_counter+=32) > 255) red_counter = 0;

	// highlight line mouse is hovering over
	if (mouse_state.lX > BSP_MIN_X && mouse_state.lX < BSP_MAX_X && 
		mouse_state.lY > BSP_MIN_Y && mouse_state.lY < BSP_MAX_Y)
	{
		int nearest_line = Context::Instance()->nearest_line;
		if (nearest_line >= 0)
		{
			if (action == ACTION_DELETE_LINE)
			{
				t3d::DrawLine32(lines[nearest_line].p0.x, lines[nearest_line].p0.y,
								lines[nearest_line].p1.x, lines[nearest_line].p1.y,
								_RGB32BIT(255,red_counter,red_counter,red_counter), video_buffer, mempitch);
			} // end if
			else
			{
				t3d::DrawLine32(lines[nearest_line].p0.x, lines[nearest_line].p0.y,
								lines[nearest_line].p1.x, lines[nearest_line].p1.y,
								_RGB32BIT(255,255,255,255), video_buffer, mempitch);
			} // end if

		} // end if

	} // end if         
}

int EditTask::DeleteLine(int x, int y, int delete_line)
{
	// this function hunts thru the lines and deletes the one closest to
	// the sent position (which is the center of the cross hairs

	int total_lines = Context::Instance()->total_lines;

	int curr_line,          // current line being processed
		best_line=-1;       // the best match so far in process 

	float sx,sy,            // starting coordinates of test line
		ex,ey,            // ending coordinates of test line
		length_line,      // total length of line being tested
		length_1,         // length of lines from endpoints of test line to target area
		length_2,
		min_x,max_x,      // bounding box of test line
		min_y,max_y;

	float best_error=10000, // start error off really large
		test_error;       // current error being processed

	BSPLine* lines = Context::Instance()->lines;

	// process each line and find best fit
	for (curr_line=0; curr_line < total_lines; curr_line++)
	{
		// extract line parameters
		sx = lines[curr_line].p0.x;
		sy = lines[curr_line].p0.y;

		ex = lines[curr_line].p1.x;
		ey = lines[curr_line].p1.y;

		// first compute length of line
		length_line = sqrt( (ex-sx) * (ex-sx) + (ey-sy) * (ey-sy) );

		// compute length of first endpoint to selected position
		length_1    = sqrt( (x-sx) * (x-sx) + (y-sy) * (y-sy) );

		// compute length of second endpoint to selected position
		length_2    = sqrt( (ex-x) * (ex-x) + (ey-y) * (ey-y) );

		// compute the bounding box of line
		min_x = min(sx,ex)-2;
		min_y = min(sy,ey)-2;
		max_x = max(sx,ex)+2;
		max_y = max(sy,ey)+2;

		// if the selection position is within bounding box then compute distance
		// errors and save this line as a possibility

		if (x >= min_x && x <= max_x && y >= min_y && y <= max_y)
		{
			// compute percent error of total length to length of lines
			// from endpoint to selected position
			test_error = (float)(100 * fabs( (length_1 + length_2) - length_line) ) / (float)length_line;

			// test if this line is a better selection than the last
			if (test_error < best_error)
			{
				// make this line the "best selection" so far
				best_error = test_error;
				best_line  = curr_line;
			} // end if

		} // end if in bounding box

	} // end for curr_line

	// did we get a line to delete?
	if (best_line != -1)
	{
		// delete the line from the line array by copying another line into
		// the position

		// test for deletion enable or just viewing
		if (delete_line)
		{
			// test for special cases
			if (total_lines == 1) // a single line
				total_lines = 0;
			else
				if (best_line == (total_lines-1))  // the line to delete is the last in array
					total_lines--;
				else
				{
					// the line to delete must be in the 0th to total_lines-1 position
					// so copy the last line into the deleted one and decrement the
					// number of lines in system
					lines[best_line] = lines[--total_lines];
				} // end else
		} // end if

		// return the line number that was deleted
		return(best_line);
	} // end if
	else
		return(-1);
}

void EditTask::DrawFloors(LPDIRECTDRAWSURFACE7 lpdds)
{
	// this function draws all the floors by rendering the textures that are 
	// mapped onto the floor scaled down (it very cute)

	// reset number of floor tiles
	Context::Instance()->num_floor_tiles = 0;

	int total_lines = Context::Instance()->total_lines;

	t3d::BOB* textures = Context::Instance()->textures;

	for (int cell_y = 0; cell_y < BSP_CELLS_Y-1; cell_y++)
	{
		for (int cell_x = 0; cell_x < BSP_CELLS_X-1; cell_x++)
		{
			// extract texture id
			int texture_id = Context::Instance()->floors[cell_y][cell_x];

			// is there a texture here?
			if (texture_id > -1)
			{
				// set position of texture on level editor space
				textures->SetPosition(BSP_MIN_X + (cell_x * BSP_GRID_SIZE),
					BSP_MIN_Y + (cell_y * BSP_GRID_SIZE));

				// set texture id
				textures->SetCurrFrame(texture_id);
				textures->DrawScaled(BSP_GRID_SIZE,BSP_GRID_SIZE,lpdds);

				// increment number of floor tiles
				Context::Instance()->num_floor_tiles++;

			} // end if 

		} // end for cell_y

	} // end for cell_y
}

void EditTask::DrawGrid(unsigned char* video_buffer, 
						int mempitch,
						int rgbcolor, 
						int x0, int y0, 
						int xpitch, int ypitch,
						int xcells, int ycells)
{
	// this function draws the grid
	for (int xc=0; xc < xcells; xc++)
	{
		for (int yc=0; yc < ycells; yc++)
		{
			t3d::DrawPixel32(x0 + xc*xpitch, y0 + yc*ypitch, rgbcolor, video_buffer, mempitch);
		} // end for y
	} // end for x
}

}