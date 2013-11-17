#pragma once

#include <ddraw.h>

namespace bspeditor {

// interface action states
#define ACTION_STARTING_LINE           0  // used to determine if user is starting a new line
#define ACTION_ENDING_LINE             1  // or ending one
#define ACTION_STARTING_POLYLINE       2  // starting a polyline
#define ACTION_ADDING_POLYLINE         3  // adding polyline segment
#define ACTION_ENDING_POLYLINE         4  // done with polyline
#define ACTION_DELETE_LINE             5  // deleting a line
#define ACTION_FLOOR_MODE              6  // adding floor tiles

class EditTask
{
public:
	EditTask();

	void DrawLines(unsigned char* video_buffer, int mempitch);

	int DeleteLine(int x, int y, int delete_line);

	void DrawFloors(LPDIRECTDRAWSURFACE7 lpdds);

	void DrawGrid(unsigned char* video_buffer, 
				  int mempitch,
				  int rgbcolor, 
				  int x0, int y0, 
				  int xpitch, int ypitch,
				  int xcells, int ycells);

public:
	// set initial action state
	int action;

}; // EditTask

}