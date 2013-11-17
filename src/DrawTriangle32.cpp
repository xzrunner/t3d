#include "tools.h"
#include "tmath.h"
#include "defines.h"
#include "Modules.h"
#include "Graphics.h"
#include "Polygon.h"

namespace t3d {

void DrawTopTri32(float x1,float y1,float x2,float y2, float x3,float y3,
				  int color,unsigned char* _dest_buffer, int mempitch)
{
	// this function draws a triangle that has a flat top

	float dx_right,    // the dx/dy ratio of the right edge of line
		  dx_left,     // the dx/dy ratio of the left edge of line
		  xs,xe,       // the starting and ending points of the edges
		  height,      // the height of the triangle
		  right,       // used by clipping
		  left;

	int iy1, iy3, loop_y; // integers for y looping

	// cast dest buffer to ushort
	unsigned int* dest_buffer = (unsigned int*)_dest_buffer;

	// destination address of next scanline
	unsigned int* dest_addr = NULL;

	// recompute mempitch in 32-bit words
	mempitch = (mempitch >> 2);

	// test order of x1 and x2
	if (x2 < x1)
		std::swap(x1, x2);

	// compute delta's
	height = y3 - y1;

	dx_left  = (x3 - x1) / height;
	dx_right = (x3 - x2) / height;

	// set starting points
	xs = x1;
	xe = x2;

	int min_clip_x;
	int max_clip_x;
	int min_clip_y;
	int max_clip_y;
	Modules::GetGraphics().GetClipValue(min_clip_x,
		max_clip_x, min_clip_y, max_clip_y);

#if (RASTERIZER_MODE == RASTERIZER_ACCURATE)
	// perform y clipping
	if (y1 < min_clip_y)
	{
		// compute new xs and ys
		xs = xs+dx_left*(-y1+min_clip_y);
		xe = xe+dx_right*(-y1+min_clip_y);

		// reset y1
		y1 = min_clip_y;

		// make sure top left fill convention is observed
		iy1 = y1;
	} // end if top is off screen
	else
	{
		// make sure top left fill convention is observed
		iy1 = ceil(y1);

		// bump xs and xe appropriately
		xs = xs+dx_left*(iy1-y1);
		xe = xe+dx_right*(iy1-y1);
	} // end else

	if (y3 > max_clip_y)
	{
		// clip y
		y3 = max_clip_y;

		// make sure top left fill convention is observed
		iy3 = y3-1;
	} // end if
	else
	{
		// make sure top left fill convention is observed
		iy3 = ceil(y3)-1;
	} // end else
#endif

#if ( (RASTERIZER_MODE==RASTERIZER_FAST) || (RASTERIZER_MODE==RASTERIZER_FASTEST) )
	// perform y clipping
	if (y1 < min_clip_y)
	{
		// compute new xs and ys
		xs = xs+dx_left*(-y1+min_clip_y);
		xe = xe+dx_right*(-y1+min_clip_y);

		// reset y1
		y1 = min_clip_y;
	} // end if top is off screen

	if (y3 > max_clip_y)
		y3 = max_clip_y;

	// make sure top left fill convention is observed
	iy1 = ceil(y1);
	iy3 = ceil(y3)-1;
#endif 

	//Write_Error("\nTri-Top: xs=%f, xe=%f, y1=%f, y3=%f, iy1=%d, iy3=%d", xs,xe,y1,y3,iy1,iy3);

	// compute starting address in video memory
	dest_addr = dest_buffer+iy1*mempitch;

	// test if x clipping is needed
	if (x1>=min_clip_x && x1<=max_clip_x &&
		x2>=min_clip_x && x2<=max_clip_x &&
		x3>=min_clip_x && x3<=max_clip_x)
	{
		// draw the triangle
		for (loop_y=iy1; loop_y<=iy3; loop_y++,dest_addr+=mempitch)
		{
			//Write_Error("\nxs=%f, xe=%f", xs,xe);
			// draw the line
			Mem_Set_QUAD(dest_addr+(unsigned int)(xs),color,(unsigned int)((int)xe-(int)xs+1));

			// adjust starting point and ending point
			xs+=dx_left;
			xe+=dx_right;

		} // end for

	} // end if no x clipping needed
	else
	{
		// clip x axis with slower version

		// draw the triangle
		for (loop_y=iy1; loop_y<=iy3; loop_y++,dest_addr+=mempitch)
		{
			// do x clip
			left  = xs;
			right = xe;

			// adjust starting point and ending point
			xs+=dx_left;
			xe+=dx_right;

			// clip line
			if (left < min_clip_x)
			{
				left = min_clip_x;

				if (right < min_clip_x)
					continue;
			}

			if (right > max_clip_x)
			{
				right = max_clip_x;

				if (left > max_clip_x)
					continue;
			}

			//Write_Error("\nleft=%f, right=%f", left,right);
			// draw the line
			Mem_Set_QUAD(dest_addr+(unsigned int)(left),color,(unsigned int)((int)right-(int)left+1));

		} // end for

	} // end else x clipping needed
}

void DrawBottomTri32(float x1,float y1, float x2,float y2, float x3,float y3,
					 int color,unsigned char* _dest_buffer, int mempitch)
{
	// this function draws a triangle that has a flat bottom

	float dx_right,    // the dx/dy ratio of the right edge of line
		  dx_left,     // the dx/dy ratio of the left edge of line
		  xs,xe,       // the starting and ending points of the edges
		  height,      // the height of the triangle
		  right,       // used by clipping
		  left;

	int iy1,iy3,loop_y;

	// cast dest buffer to ushort
	unsigned int* dest_buffer = (unsigned int*)_dest_buffer;

	// destination address of next scanline
	unsigned int* dest_addr = NULL;

	// recompute mempitch in 32-bit words
	mempitch = (mempitch >> 2);

	// test order of x1 and x2
	if (x3 < x2)
		std::swap(x2, x3);

	// compute delta's
	height = y3-y1;

	dx_left  = (x2-x1)/height;
	dx_right = (x3-x1)/height;

	// set starting points
	xs = x1;
	xe = x1;

	int min_clip_x;
	int max_clip_x;
	int min_clip_y;
	int max_clip_y;
	Modules::GetGraphics().GetClipValue(min_clip_x,
		max_clip_x, min_clip_y, max_clip_y);

#if (RASTERIZER_MODE==RASTERIZER_ACCURATE)
	// perform y clipping
	if (y1 < min_clip_y)
	{
		// compute new xs and ys
		xs = xs+dx_left*(-y1+min_clip_y);
		xe = xe+dx_right*(-y1+min_clip_y);

		// reset y1
		y1 = min_clip_y;

		// make sure top left fill convention is observed
		iy1 = y1;
	} // end if top is off screen
	else
	{
		// make sure top left fill convention is observed
		iy1 = ceil(y1);

		// bump xs and xe appropriately
		xs = xs+dx_left*(iy1-y1);
		xe = xe+dx_right*(iy1-y1);
	} // end else

	if (y3 > max_clip_y)
	{
		// clip y
		y3 = max_clip_y;

		// make sure top left fill convention is observed
		iy3 = y3-1;
	} // end if
	else
	{
		// make sure top left fill convention is observed
		iy3 = ceil(y3)-1;
	} // end else
#endif

#if ( (RASTERIZER_MODE==RASTERIZER_FAST) || (RASTERIZER_MODE==RASTERIZER_FASTEST) )
	// perform y clipping
	if (y1 < min_clip_y)
	{
		// compute new xs and ys
		xs = xs+dx_left*(-y1+min_clip_y);
		xe = xe+dx_right*(-y1+min_clip_y);

		// reset y1
		y1 = min_clip_y;
	} // end if top is off screen

	if (y3 > max_clip_y)
		y3 = max_clip_y;

	// make sure top left fill convention is observed
	iy1 = ceil(y1);
	iy3 = ceil(y3)-1;
#endif 

	//Write_Error("\nTri-Bottom: xs=%f, xe=%f, y1=%f, y3=%f, iy1=%d, iy3=%d", xs,xe,y1,y3,iy1,iy3);

	// compute starting address in video memory
	dest_addr = dest_buffer+iy1*mempitch;

	// test if x clipping is needed
	if (x1>=min_clip_x && x1<=max_clip_x &&
		x2>=min_clip_x && x2<=max_clip_x &&
		x3>=min_clip_x && x3<=max_clip_x)
	{
		// draw the triangle
		for (loop_y=iy1; loop_y<=iy3; loop_y++,dest_addr+=mempitch)
		{
			//Write_Error("\nxs=%f, xe=%f", xs,xe);
			// draw the line
			Mem_Set_QUAD(dest_addr+(unsigned int)(xs),color,(unsigned int)((int)xe-(int)xs+1));

			// adjust starting point and ending point
			xs+=dx_left;
			xe+=dx_right;

		} // end for

	} // end if no x clipping needed
	else
	{
		// clip x axis with slower version

		// draw the triangle
		for (loop_y=y1; loop_y<=y3; loop_y++,dest_addr+=mempitch)
		{
			// do x clip
			left  = xs;
			right = xe;

			// adjust starting point and ending point
			xs+=dx_left;
			xe+=dx_right;

			// clip line
			if (left < min_clip_x)
			{
				left = min_clip_x;

				if (right < min_clip_x)
					continue;
			}

			if (right > max_clip_x)
			{
				right = max_clip_x;

				if (left > max_clip_x)
					continue;
			}

			//Write_Error("\nleft=%f, right=%f", left,right);
			// draw the line
			Mem_Set_QUAD(dest_addr+(unsigned int)(left),color,(unsigned int)((int)right-(int)left+1));

		}

	} // end else x clipping needed
}

void DrawTriangle32(float x1,float y1,float x2,float y2,float x3,float y3, 
					int color,unsigned char *dest_buffer, int mempitch)
{
	// this function draws a triangle on the destination buffer
	// it decomposes all triangles into a pair of flat top, flat bottom

#ifdef DEBUG_ON
	// track rendering stats
	debug_polys_rendered_per_frame++;
#endif


	// test for h lines and v lines
	if ((FCMP(x1,x2) && FCMP(x2,x3))  ||  (FCMP(y1,y2) && FCMP(y2,y3)))
		return;

	// sort p1,p2,p3 in ascending y order
	if (y2 < y1)
	{
		std::swap(x1,x2);
		std::swap(y1,y2);
	} // end if

	// now we know that p1 and p2 are in order
	if (y3 < y1)
	{
		std::swap(x1,x3);
		std::swap(y1,y3);
	} // end if

	// finally test y3 against y2
	if (y3 < y2)
	{
		std::swap(x2,x3);
		std::swap(y2,y3);
	} // end if

	int min_clip_x;
	int max_clip_x;
	int min_clip_y;
	int max_clip_y;
	Modules::GetGraphics().GetClipValue(min_clip_x,
		max_clip_x, min_clip_y, max_clip_y);

	// do trivial rejection tests for clipping
	if ( y3<min_clip_y || y1>max_clip_y ||
		(x1<min_clip_x && x2<min_clip_x && x3<min_clip_x) ||
		(x1>max_clip_x && x2>max_clip_x && x3>max_clip_x) )
		return;

	// test if top of triangle is flat
	if (FCMP(y1,y2))
	{
		DrawTopTri32(x1,y1,x2,y2,x3,y3,color, dest_buffer, mempitch);
	}
	else if (FCMP(y2,y3)) 
	{
		DrawBottomTri32(x1,y1,x2,y2,x3,y3,color, dest_buffer, mempitch);
	} 
	// end if bottom is flat
	else 
	{
		// general triangle that's needs to be broken up along long edge
		float new_x = x1 + (y2-y1)*(x3-x1)/(y3-y1);

		// draw each sub-triangle
		DrawBottomTri32(x1,y1,new_x,y2,x2,y2,color, dest_buffer, mempitch);
		DrawTopTri32(x2,y2,new_x,y2,x3,y3,color, dest_buffer, mempitch);
	}
}

void DrawTriangle32(PolygonF* face,unsigned char* _dest_buffer, int mem_pitch)
{
	// this function draws a flat shaded polygon with zbuffering

	int v0=0,
		v1=1,
		v2=2,
		tri_type = TRI_TYPE_NONE,
		irestart = INTERP_LHS;

	int dx,dy,dyl,dyr,      // general deltas
		xi,yi,              // the current interpolated x,y
		index_x,index_y,    // looping vars
		x,y,                // hold general x,y
		xstart,
		xend,
		ystart,
		yrestart,
		yend,
		xl,                 
		dxdyl,              
		xr,
		dxdyr;

	int x0,y0,    // cached vertices
		x1,y1,
		x2,y2;

	unsigned int *screen_ptr  = NULL,
				 *screen_line = NULL,
				 *textmap     = NULL,
				 *dest_buffer = (unsigned int*)_dest_buffer;

	unsigned int color;    // polygon color

#ifdef DEBUG_ON
	// track rendering stats
	debug_polys_rendered_per_frame++;
#endif

	// adjust memory pitch to words, divide by 4
	mem_pitch >>= 2;

	// apply fill convention to coordinates
	face->tvlist[0].x = (int)(face->tvlist[0].x+0.5);
	face->tvlist[0].y = (int)(face->tvlist[0].y+0.5);

	face->tvlist[1].x = (int)(face->tvlist[1].x+0.5);
	face->tvlist[1].y = (int)(face->tvlist[1].y+0.5);

	face->tvlist[2].x = (int)(face->tvlist[2].x+0.5);
	face->tvlist[2].y = (int)(face->tvlist[2].y+0.5);

	int min_clip_x;
	int max_clip_x;
	int min_clip_y;
	int max_clip_y;
	Modules::GetGraphics().GetClipValue(min_clip_x,
		max_clip_x, min_clip_y, max_clip_y);

	// first trivial clipping rejection tests 
	if (((face->tvlist[0].y < min_clip_y)  && 
		(face->tvlist[1].y < min_clip_y)  &&
		(face->tvlist[2].y < min_clip_y)) ||

		((face->tvlist[0].y > max_clip_y)  && 
		(face->tvlist[1].y > max_clip_y)  &&
		(face->tvlist[2].y > max_clip_y)) ||

		((face->tvlist[0].x < min_clip_x)  && 
		(face->tvlist[1].x < min_clip_x)  &&
		(face->tvlist[2].x < min_clip_x)) ||

		((face->tvlist[0].x > max_clip_x)  && 
		(face->tvlist[1].x > max_clip_x)  &&
		(face->tvlist[2].x > max_clip_x)))
		return;


	// sort vertices
	if (face->tvlist[v1].y < face->tvlist[v0].y) 
	{std::swap(v0,v1);} 

	if (face->tvlist[v2].y < face->tvlist[v0].y) 
	{std::swap(v0,v2);}

	if (face->tvlist[v2].y < face->tvlist[v1].y) 
	{std::swap(v1,v2);}

	// now test for trivial flat sided cases
	if (FCMP(face->tvlist[v0].y, face->tvlist[v1].y) )
	{ 
		// set triangle type
		tri_type = TRI_TYPE_FLAT_TOP;

		// sort vertices left to right
		if (face->tvlist[v1].x < face->tvlist[v0].x) 
		{std::swap(v0,v1);}

	} // end if
	else
		// now test for trivial flat sided cases
		if (FCMP(face->tvlist[v1].y, face->tvlist[v2].y) )
		{ 
			// set triangle type
			tri_type = TRI_TYPE_FLAT_BOTTOM;

			// sort vertices left to right
			if (face->tvlist[v2].x < face->tvlist[v1].x) 
			{std::swap(v1,v2);}

		} // end if
		else
		{
			// must be a general triangle
			tri_type = TRI_TYPE_GENERAL;

		} // end else

		// extract vertices for processing, now that we have order
		x0  = (int)(face->tvlist[v0].x+0.0);
		y0  = (int)(face->tvlist[v0].y+0.0);

		x1  = (int)(face->tvlist[v1].x+0.0);
		y1  = (int)(face->tvlist[v1].y+0.0);

		x2  = (int)(face->tvlist[v2].x+0.0);
		y2  = (int)(face->tvlist[v2].y+0.0);

		// degenerate triangle
		if ( ((x0 == x1) && (x1 == x2)) || ((y0 ==  y1) && (y1 == y2)))
			return;

		// extract constant color
		color = face->lit_color[0];

		// set interpolation restart value
		yrestart = y1;

		// what kind of triangle
		if (tri_type & TRI_TYPE_FLAT_MASK)
		{

			if (tri_type == TRI_TYPE_FLAT_TOP)
			{
				// compute all deltas
				dy = (y2 - y0);

				dxdyl = ((x2 - x0)   << FIXP16_SHIFT)/dy;
				dxdyr = ((x2 - x1)   << FIXP16_SHIFT)/dy;

				// test for y clipping
				if (y0 < min_clip_y)
				{
					// compute overclip
					dy = (min_clip_y - y0);

					// computer new LHS starting values
					xl = dxdyl*dy + (x0  << FIXP16_SHIFT);

					// compute new RHS starting values
					xr = dxdyr*dy + (x1  << FIXP16_SHIFT);

					// compute new starting y
					ystart = min_clip_y;

				} // end if
				else
				{
					// no clipping

					// set starting values
					xl = (x0 << FIXP16_SHIFT);
					xr = (x1 << FIXP16_SHIFT);

					// set starting y
					ystart = y0;

				} // end else

			} // end if flat top
			else
			{
				// must be flat bottom

				// compute all deltas
				dy = (y1 - y0);

				dxdyl = ((x1 - x0)   << FIXP16_SHIFT)/dy;
				dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dy;

				// test for y clipping
				if (y0 < min_clip_y)
				{
					// compute overclip
					dy = (min_clip_y - y0);

					// computer new LHS starting values
					xl = dxdyl*dy + (x0  << FIXP16_SHIFT);

					// compute new RHS starting values
					xr = dxdyr*dy + (x0  << FIXP16_SHIFT);

					// compute new starting y
					ystart = min_clip_y;

				} // end if
				else
				{
					// no clipping

					// set starting values
					xl = (x0 << FIXP16_SHIFT);
					xr = (x0 << FIXP16_SHIFT);

					// set starting y
					ystart = y0;

				} // end else	

			} // end else flat bottom

			// test for bottom clip, always
			if ((yend = y2) > max_clip_y)
				yend = max_clip_y;

			// test for horizontal clipping
			if ((x0 < min_clip_x) || (x0 > max_clip_x) ||
				(x1 < min_clip_x) || (x1 > max_clip_x) ||
				(x2 < min_clip_x) || (x2 > max_clip_x))
			{
				// clip version

				// point screen ptr to starting line
				screen_ptr = dest_buffer + (ystart * mem_pitch);

				for (yi = ystart; yi < yend; yi++)
				{
					// compute span endpoints
					xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
					xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

					dx = (xend - xstart);

					///////////////////////////////////////////////////////////////////////

					// test for x clipping, LHS
					if (xstart < min_clip_x)
					{
						// compute x overlap
						dx = min_clip_x - xstart;

						// reset vars
						xstart = min_clip_x;

					} // end if

					// test for x clipping RHS
					if (xend > max_clip_x)
						xend = max_clip_x;

					///////////////////////////////////////////////////////////////////////

					// draw span
					for (xi=xstart; xi < xend; xi++)
					{
						// write textel assume 8.8.8
						screen_ptr[xi] = color;
					} // end for xi

					// interpolate x along right and left edge
					xl+=dxdyl;
					xr+=dxdyr;

					// advance screen ptr
					screen_ptr+=mem_pitch;

				} // end for y

			} // end if clip
			else
			{
				// non-clip version

				// point screen ptr to starting line
				screen_ptr = dest_buffer + (ystart * mem_pitch);

				for (yi = ystart; yi < yend; yi++)
				{
					// compute span endpoints
					xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
					xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

					dx = (xend - xstart);

					// draw span
					for (xi=xstart; xi < xend; xi++)
					{
						// write textel 8.8.8
						screen_ptr[xi] = color;
					} // end for xi

					// interpolate x,z along right and left edge
					xl+=dxdyl;
					xr+=dxdyr;

					// advance screen ptr
					screen_ptr+=mem_pitch;

				} // end for y

			} // end if non-clipped

		} // end if
		else
			if (tri_type==TRI_TYPE_GENERAL)
			{

				// first test for bottom clip, always
				if ((yend = y2) > max_clip_y)
					yend = max_clip_y;

				// pre-test y clipping status
				if (y1 < min_clip_y)
				{
					// compute all deltas
					// LHS
					dyl = (y2 - y1);

					dxdyl = ((x2  - x1)  << FIXP16_SHIFT)/dyl;

					// RHS
					dyr = (y2 - y0);	

					dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;

					// compute overclip
					dyr = (min_clip_y - y0);
					dyl = (min_clip_y - y1);

					// computer new LHS starting values
					xl = dxdyl*dyl + (x1  << FIXP16_SHIFT);

					// compute new RHS starting values
					xr = dxdyr*dyr + (x0  << FIXP16_SHIFT);

					// compute new starting y
					ystart = min_clip_y;

					// test if we need swap to keep rendering left to right
					if (dxdyr > dxdyl)
					{
						std::swap(dxdyl,dxdyr);
						std::swap(xl,xr);
						std::swap(x1,x2);
						std::swap(y1,y2);

						// set interpolation restart
						irestart = INTERP_RHS;

					} // end if

				} // end if
				else
					if (y0 < min_clip_y)
					{
						// compute all deltas
						// LHS
						dyl = (y1 - y0);

						dxdyl = ((x1  - x0)  << FIXP16_SHIFT)/dyl;

						// RHS
						dyr = (y2 - y0);	

						dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;

						// compute overclip
						dy = (min_clip_y - y0);

						// computer new LHS starting values
						xl = dxdyl*dy + (x0  << FIXP16_SHIFT);

						// compute new RHS starting values
						xr = dxdyr*dy + (x0  << FIXP16_SHIFT);

						// compute new starting y
						ystart = min_clip_y;

						// test if we need swap to keep rendering left to right
						if (dxdyr < dxdyl)
						{
							std::swap(dxdyl,dxdyr);
							std::swap(xl,xr);
							std::swap(x1,x2);
							std::swap(y1,y2);

							// set interpolation restart
							irestart = INTERP_RHS;

						} // end if

					} // end if
					else
					{
						// no initial y clipping

						// compute all deltas
						// LHS
						dyl = (y1 - y0);

						dxdyl = ((x1  - x0)  << FIXP16_SHIFT)/dyl;

						// RHS
						dyr = (y2 - y0);	

						dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dyr;

						// no clipping y

						// set starting values
						xl = (x0 << FIXP16_SHIFT);
						xr = (x0 << FIXP16_SHIFT);

						// set starting y
						ystart = y0;

						// test if we need swap to keep rendering left to right
						if (dxdyr < dxdyl)
						{
							std::swap(dxdyl,dxdyr);
							std::swap(xl,xr);
							std::swap(x1,x2);
							std::swap(y1,y2);

							// set interpolation restart
							irestart = INTERP_RHS;

						} // end if

					} // end else

					// test for horizontal clipping
					if ((x0 < min_clip_x) || (x0 > max_clip_x) ||
						(x1 < min_clip_x) || (x1 > max_clip_x) ||
						(x2 < min_clip_x) || (x2 > max_clip_x))
					{
						// clip version
						// x clipping	

						// point screen ptr to starting line
						screen_ptr = dest_buffer + (ystart * mem_pitch);

						for (yi = ystart; yi < yend; yi++)
						{
							// compute span endpoints
							xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
							xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

							dx = (xend - xstart);

							///////////////////////////////////////////////////////////////////////

							// test for x clipping, LHS
							if (xstart < min_clip_x)
							{
								// compute x overlap
								dx = min_clip_x - xstart;

								// set x to left clip edge
								xstart = min_clip_x;

							} // end if

							// test for x clipping RHS
							if (xend > max_clip_x)
								xend = max_clip_x;

							///////////////////////////////////////////////////////////////////////

							// draw span
							for (xi=xstart; xi < xend; xi++)
							{
								// write textel assume 8.8.8
								screen_ptr[xi] = color;

							} // end for xi

							// interpolate z,x along right and left edge
							xl+=dxdyl;
							xr+=dxdyr;

							// advance screen ptr
							screen_ptr+=mem_pitch;

							// test for yi hitting second region, if so change interpolant
							if (yi==yrestart)
							{
								// test interpolation side change flag

								if (irestart == INTERP_LHS)
								{
									// LHS
									dyl = (y2 - y1);	

									dxdyl = ((x2 - x1)   << FIXP16_SHIFT)/dyl;

									// set starting values
									xl = (x1  << FIXP16_SHIFT);

									// interpolate down on LHS to even up
									xl+=dxdyl;
								} // end if
								else
								{
									// RHS
									dyr = (y1 - y2);	

									dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;

									// set starting values
									xr = (x2  << FIXP16_SHIFT);

									// interpolate down on RHS to even up
									xr+=dxdyr;

								} // end else

							} // end if

						} // end for y

					} // end if
					else
					{
						// no x clipping
						// point screen ptr to starting line
						screen_ptr = dest_buffer + (ystart * mem_pitch);

						for (yi = ystart; yi < yend; yi++)
						{
							// compute span endpoints
							xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
							xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

							dx = (xend - xstart);

							// draw span
							for (xi=xstart; xi < xend; xi++)
							{
								// write textel assume 8.8.8
								screen_ptr[xi] = color;
							} // end for xi

							// interpolate x,z along right and left edge
							xl+=dxdyl;
							xr+=dxdyr;

							// advance screen ptr
							screen_ptr+=mem_pitch;

							// test for yi hitting second region, if so change interpolant
							if (yi==yrestart)
							{
								// test interpolation side change flag

								if (irestart == INTERP_LHS)
								{
									// LHS
									dyl = (y2 - y1);	

									dxdyl = ((x2 - x1)   << FIXP16_SHIFT)/dyl;

									// set starting values
									xl = (x1  << FIXP16_SHIFT);

									// interpolate down on LHS to even up
									xl+=dxdyl;
								} // end if
								else
								{
									// RHS
									dyr = (y1 - y2);	

									dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;

									// set starting values
									xr = (x2  << FIXP16_SHIFT);

									// interpolate down on RHS to even up
									xr+=dxdyr;
								} // end else

							} // end if

						} // end for y

					} // end else	

			} // end if
}


}