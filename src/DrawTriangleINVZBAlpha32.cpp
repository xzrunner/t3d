#include "tools.h"
#include "tmath.h"
#include "defines.h"
#include "Modules.h"
#include "Graphics.h"
#include "Polygon.h"

namespace t3d {

 void DrawTriangleINVZBAlpha32(PolygonF* face,unsigned char* _dest_buffer, int mempitch, 
	 unsigned char* _zbuffer, int zpitch, int alpha)
 {
 	// this function draws a flat shaded polygon with zbuffering
 
 	int v0=0,
 		v1=1,
 		v2=2,
 		temp=0,
 		tri_type = TRI_TYPE_NONE,
 		irestart = INTERP_LHS;
 
 	int dx,dy,dyl,dyr,      // general deltas
 		z,
 		dz,
 		xi,yi,              // the current interpolated x,y
 		zi,                 // the current interpolated z
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
 		dxdyr,             
 		dzdyl,   
 		zl,
 		dzdyr,
 		zr;
 
 	int x0,y0,tz0,    // cached vertices
 		x1,y1,tz1,
 		x2,y2,tz2;
 
 	unsigned int *screen_ptr  = NULL,
 				 *screen_line = NULL,
 				 *textmap     = NULL,
 				 *dest_buffer = (unsigned int*)_dest_buffer;
 
 	unsigned int *z_ptr = NULL,
 				 *zbuffer = (unsigned int*)_zbuffer;
 
 	unsigned int color;    // polygon color
 
 #ifdef DEBUG_ON
 	// track rendering stats
 	debug_polys_rendered_per_frame++;
 #endif
 
 	// adjust memory pitch to words, divide by 4
 	mempitch >>= 2;
 
 	// adjust zbuffer pitch for 32 bit alignment
 	zpitch >>= 2;
 
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
 
 		tz0 = (1 << FIXP28_SHIFT) / (int)(face->tvlist[v0].z+0.5);
 
 		x1  = (int)(face->tvlist[v1].x+0.0);
 		y1  = (int)(face->tvlist[v1].y+0.0);
 
 		tz1 = (1 << FIXP28_SHIFT) / (int)(face->tvlist[v1].z+0.5);
 
 		x2  = (int)(face->tvlist[v2].x+0.0);
 		y2  = (int)(face->tvlist[v2].y+0.0);
 
 		tz2 = (1 << FIXP28_SHIFT) / (int)(face->tvlist[v2].z+0.5);
 
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
 				dzdyl = ((tz2 - tz0) << 0)/dy; 
 
 				dxdyr = ((x2 - x1)   << FIXP16_SHIFT)/dy;
 				dzdyr = ((tz2 - tz1) << 0)/dy;   
 
 				// test for y clipping
 				if (y0 < min_clip_y)
 				{
 					// compute overclip
 					dy = (min_clip_y - y0);
 
 					// computer new LHS starting values
 					xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
 					zl = dzdyl*dy + (tz0 << 0);
 
 					// compute new RHS starting values
 					xr = dxdyr*dy + (x1  << FIXP16_SHIFT);
 					zr = dzdyr*dy + (tz1 << 0);
 
 					// compute new starting y
 					ystart = min_clip_y;
 
 				} // end if
 				else
 				{
 					// no clipping
 
 					// set starting values
 					xl = (x0 << FIXP16_SHIFT);
 					xr = (x1 << FIXP16_SHIFT);
 
 					zl = (tz0 << 0);
 					zr = (tz1 << 0);
 
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
 				dzdyl = ((tz1 - tz0) << 0)/dy; 
 
 				dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dy;
 				dzdyr = ((tz2 - tz0) << 0)/dy;   
 
 				// test for y clipping
 				if (y0 < min_clip_y)
 				{
 					// compute overclip
 					dy = (min_clip_y - y0);
 
 					// computer new LHS starting values
 					xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
 					zl = dzdyl*dy + (tz0 << 0);
 
 					// compute new RHS starting values
 					xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
 					zr = dzdyr*dy + (tz0 << 0);
 
 					// compute new starting y
 					ystart = min_clip_y;
 
 				} // end if
 				else
 				{
 					// no clipping
 
 					// set starting values
 					xl = (x0 << FIXP16_SHIFT);
 					xr = (x0 << FIXP16_SHIFT);
 
 					zl = (tz0 << 0);
 					zr = (tz0 << 0);
 
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
 				screen_ptr = dest_buffer + (ystart * mempitch);
 
 				// point zbuffer to starting line
 				z_ptr = zbuffer + (ystart * zpitch);
 
 				for (yi = ystart; yi < yend; yi++)
 				{
 					// compute span endpoints
 					xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
 					xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
 
 					// compute starting points for u,v,w interpolants
 					zi = zl;
 
 					// compute u,v interpolants
 					if ((dx = (xend - xstart))>0)
 					{
 						dz = (zr - zl)/dx;
 					} // end if
 					else
 					{
 						dz = (zr - zl);
 					} // end else
 
 					///////////////////////////////////////////////////////////////////////
 
 					// test for x clipping, LHS
 					if (xstart < min_clip_x)
 					{
 						// compute x overlap
 						dx = min_clip_x - xstart;
 
 						// slide interpolants over
 						zi+=dx*dz;
 
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
 						// test if z of current pixel is nearer than current z buffer value
 						if (zi > z_ptr[xi])
 						{
 							// write textel assume 8.8.8
							int tmpa;
							int r0, g0, b0;
							_RGB8888FROM32BIT(color, &tmpa, &r0, &g0, &b0);
							int r1, g1, b1;
							_RGB8888FROM32BIT(screen_ptr[xi], &tmpa, &r1, &g1, &b1);
							r0 = r0 * alpha + r1 * (255 - alpha);
							g0 = g0 * alpha + g1 * (255 - alpha);
							b0 = b0 * alpha + b1 * (255 - alpha);
							screen_ptr[xi] = _RGB32BIT(255, r0 >> 8, g0 >> 8, b0 >> 8);
 
 							// update z-buffer
 							z_ptr[xi] = zi;           
 						} // end if
 
 						// interpolate u,v,w,z
 						zi+=dz;
 					} // end for xi
 
 					// interpolate z,x along right and left edge
 					xl+=dxdyl;
 					zl+=dzdyl;
 
 					xr+=dxdyr;
 					zr+=dzdyr;
 
 					// advance screen ptr
 					screen_ptr+=mempitch;
 
 					// advance z-buffer ptr
 					z_ptr+=zpitch;
 
 				} // end for y
 
 			} // end if clip
 			else
 			{
 				// non-clip version
 
 				// point screen ptr to starting line
 				screen_ptr = dest_buffer + (ystart * mempitch);
 
 				// point zbuffer to starting line
 				z_ptr = zbuffer + (ystart * zpitch);
 
 				for (yi = ystart; yi < yend; yi++)
 				{
 					// compute span endpoints
 					xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
 					xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
 
 					// compute starting points for u,v,w interpolants
 					zi = zl;
 
 					// compute u,v interpolants
 					if ((dx = (xend - xstart))>0)
 					{
 						dz = (zr - zl)/dx;
 					} // end if
 					else
 					{
 						dz = (zr - zl);
 					} // end else
 
 					// draw span
 					for (xi=xstart; xi < xend; xi++)
 					{
 						// test if z of current pixel is nearer than current z buffer value
 						if (zi > z_ptr[xi])
 						{
 							// write textel 8.8.8
							int tmpa;
							int r0, g0, b0;
							_RGB8888FROM32BIT(color, &tmpa, &r0, &g0, &b0);
							int r1, g1, b1;
							_RGB8888FROM32BIT(screen_ptr[xi], &tmpa, &r1, &g1, &b1);
							r0 = r0 * alpha + r1 * (255 - alpha);
							g0 = g0 * alpha + g1 * (255 - alpha);
							b0 = b0 * alpha + b1 * (255 - alpha);
							screen_ptr[xi] = _RGB32BIT(255, r0 >> 8, g0 >> 8, b0 >> 8);
 
 							// update z-buffer
 							z_ptr[xi] = zi;           
 						} // end if
 
 
 						// interpolate z
 						zi+=dz;
 					} // end for xi
 
 					// interpolate x,z along right and left edge
 					xl+=dxdyl;
 					zl+=dzdyl;
 
 					xr+=dxdyr;
 					zr+=dzdyr;
 
 					// advance screen ptr
 					screen_ptr+=mempitch;
 
 					// advance z-buffer ptr
 					z_ptr+=zpitch;
 
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
 					dzdyl = ((tz2 - tz1) << 0)/dyl; 
 
 					// RHS
 					dyr = (y2 - y0);	
 
 					dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
 					dzdyr = ((tz2 - tz0) << 0)/dyr;  
 
 					// compute overclip
 					dyr = (min_clip_y - y0);
 					dyl = (min_clip_y - y1);
 
 					// computer new LHS starting values
 					xl = dxdyl*dyl + (x1  << FIXP16_SHIFT);
 					zl = dzdyl*dyl + (tz1 << 0);
 
 					// compute new RHS starting values
 					xr = dxdyr*dyr + (x0  << FIXP16_SHIFT);
 					zr = dzdyr*dyr + (tz0 << 0);
 
 					// compute new starting y
 					ystart = min_clip_y;
 
 					// test if we need swap to keep rendering left to right
 					if (dxdyr > dxdyl)
 					{
 						std::swap(dxdyl,dxdyr);
 						std::swap(dzdyl,dzdyr);
 						std::swap(xl,xr);
 						std::swap(zl,zr);
 						std::swap(x1,x2);
 						std::swap(y1,y2);
 						std::swap(tz1,tz2);
 
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
 						dzdyl = ((tz1 - tz0) << 0)/dyl; 
 
 						// RHS
 						dyr = (y2 - y0);	
 
 						dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
 						dzdyr = ((tz2 - tz0) << 0)/dyr;  
 
 						// compute overclip
 						dy = (min_clip_y - y0);
 
 						// computer new LHS starting values
 						xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
 						zl = dzdyl*dy + (tz0 << 0);
 
 						// compute new RHS starting values
 						xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
 						zr = dzdyr*dy + (tz0 << 0);
 
 						// compute new starting y
 						ystart = min_clip_y;
 
 						// test if we need swap to keep rendering left to right
 						if (dxdyr < dxdyl)
 						{
 							std::swap(dxdyl,dxdyr);
 							std::swap(dzdyl,dzdyr);
 							std::swap(xl,xr);
 							std::swap(zl,zr);
 							std::swap(x1,x2);
 							std::swap(y1,y2);
 							std::swap(tz1,tz2);
 
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
 						dzdyl = ((tz1 - tz0) << 0)/dyl; 
 
 						// RHS
 						dyr = (y2 - y0);	
 
 						dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dyr;
 						dzdyr = ((tz2 - tz0) << 0)/dyr;
 
 						// no clipping y
 
 						// set starting values
 						xl = (x0 << FIXP16_SHIFT);
 						xr = (x0 << FIXP16_SHIFT);
 
 						zl = (tz0 << 0);
 						zr = (tz0 << 0);
 
 						// set starting y
 						ystart = y0;
 
 						// test if we need swap to keep rendering left to right
 						if (dxdyr < dxdyl)
 						{
 							std::swap(dxdyl,dxdyr);
 							std::swap(dzdyl,dzdyr);
 							std::swap(xl,xr);
 							std::swap(zl,zr);
 							std::swap(x1,x2);
 							std::swap(y1,y2);
 							std::swap(tz1,tz2);
 
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
 						screen_ptr = dest_buffer + (ystart * mempitch);
 
 						// point zbuffer to starting line
 						z_ptr = zbuffer + (ystart * zpitch);
 
 						for (yi = ystart; yi < yend; yi++)
 						{
 							// compute span endpoints
 							xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
 							xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
 
 							// compute starting points for z interpolants
 							zi = zl;
 
 							// compute z interpolants
 							if ((dx = (xend - xstart))>0)
 							{
 								dz = (zr - zl)/dx;
 							} // end if
 							else
 							{
 								dz = (zr - zl);
 							} // end else
 
 							///////////////////////////////////////////////////////////////////////
 
 							// test for x clipping, LHS
 							if (xstart < min_clip_x)
 							{
 								// compute x overlap
 								dx = min_clip_x - xstart;
 
 								// slide interpolants over
 								zi+=dx*dz;
 
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
 								// test if z of current pixel is nearer than current z buffer value
 								if (zi > z_ptr[xi])
 								{
 									// write textel assume 8.8.8
									int tmpa;
									int r0, g0, b0;
									_RGB8888FROM32BIT(color, &tmpa, &r0, &g0, &b0);
									int r1, g1, b1;
									_RGB8888FROM32BIT(screen_ptr[xi], &tmpa, &r1, &g1, &b1);
									r0 = r0 * alpha + r1 * (255 - alpha);
									g0 = g0 * alpha + g1 * (255 - alpha);
									b0 = b0 * alpha + b1 * (255 - alpha);
									screen_ptr[xi] = _RGB32BIT(255, r0 >> 8, g0 >> 8, b0 >> 8);
 
 									// update z-buffer
 									z_ptr[xi] = zi;           
 								} // end if
 
 								// interpolate z
 								zi+=dz;
 							} // end for xi
 
 							// interpolate z,x along right and left edge
 							xl+=dxdyl;
 							zl+=dzdyl;
 
 							xr+=dxdyr;
 							zr+=dzdyr;
 
 							// advance screen ptr
 							screen_ptr+=mempitch;
 
 							// advance z-buffer ptr
 							z_ptr+=zpitch;
 
 							// test for yi hitting second region, if so change interpolant
 							if (yi==yrestart)
 							{
 								// test interpolation side change flag
 
 								if (irestart == INTERP_LHS)
 								{
 									// LHS
 									dyl = (y2 - y1);	
 
 									dxdyl = ((x2 - x1)   << FIXP16_SHIFT)/dyl;
 									dzdyl = ((tz2 - tz1) << 0)/dyl;  
 
 									// set starting values
 									xl = (x1  << FIXP16_SHIFT);
 									zl = (tz1 << 0);
 
 									// interpolate down on LHS to even up
 									xl+=dxdyl;
 									zl+=dzdyl;
 								} // end if
 								else
 								{
 									// RHS
 									dyr = (y1 - y2);	
 
 									dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
 									dzdyr = ((tz1 - tz2) << 0)/dyr;   
 
 									// set starting values
 									xr = (x2  << FIXP16_SHIFT);
 									zr = (tz2 << 0);
 
 									// interpolate down on RHS to even up
 									xr+=dxdyr;
 									zr+=dzdyr;
 
 								} // end else
 
 							} // end if
 
 						} // end for y
 
 					} // end if
 					else
 					{
 						// no x clipping
 						// point screen ptr to starting line
 						screen_ptr = dest_buffer + (ystart * mempitch);
 
 						// point zbuffer to starting line
 						z_ptr = zbuffer + (ystart * zpitch);
 
 						for (yi = ystart; yi < yend; yi++)
 						{
 							// compute span endpoints
 							xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
 							xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
 
 							// compute starting points for u,v,w,z interpolants
 							zi = zl;
 
 							// compute u,v interpolants
 							if ((dx = (xend - xstart))>0)
 							{
 								dz = (zr - zl)/dx;
 							} // end if
 							else
 							{
 								dz = (zr - zl);
 							} // end else
 
 							// draw span
 							for (xi=xstart; xi < xend; xi++)
 							{
 								// test if z of current pixel is nearer than current z buffer value
 								if (zi > z_ptr[xi])
 								{
 									// write textel assume 8.8.8
									int tmpa;
									int r0, g0, b0;
									_RGB8888FROM32BIT(color, &tmpa, &r0, &g0, &b0);
									int r1, g1, b1;
									_RGB8888FROM32BIT(screen_ptr[xi], &tmpa, &r1, &g1, &b1);
									r0 = r0 * alpha + r1 * (255 - alpha);
									g0 = g0 * alpha + g1 * (255 - alpha);
									b0 = b0 * alpha + b1 * (255 - alpha);
									screen_ptr[xi] = _RGB32BIT(255, r0 >> 8, g0 >> 8, b0 >> 8);

 									// update z-buffer
 									z_ptr[xi] = zi;           
 								} // end if
 
 								// interpolate z
 								zi+=dz;
 							} // end for xi
 
 							// interpolate x,z along right and left edge
 							xl+=dxdyl;
 							zl+=dzdyl;
 
 							xr+=dxdyr;
 							zr+=dzdyr;
 
 							// advance screen ptr
 							screen_ptr+=mempitch;
 
 							// advance z-buffer ptr
 							z_ptr+=zpitch;
 
 							// test for yi hitting second region, if so change interpolant
 							if (yi==yrestart)
 							{
 								// test interpolation side change flag
 
 								if (irestart == INTERP_LHS)
 								{
 									// LHS
 									dyl = (y2 - y1);	
 
 									dxdyl = ((x2 - x1)   << FIXP16_SHIFT)/dyl;
 									dzdyl = ((tz2 - tz1) << 0)/dyl;   
 
 									// set starting values
 									xl = (x1  << FIXP16_SHIFT);
 									zl = (tz1 << 0);
 
 									// interpolate down on LHS to even up
 									xl+=dxdyl;
 									zl+=dzdyl;
 								} // end if
 								else
 								{
 									// RHS
 									dyr = (y1 - y2);	
 
 									dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
 									dzdyr = ((tz1 - tz2) << 0)/dyr;   
 
 									// set starting values
 									xr = (x2  << FIXP16_SHIFT);
 									zr = (tz2 << 0);
 
 									// interpolate down on RHS to even up
 									xr+=dxdyr;
 									zr+=dzdyr;
 								} // end else
 
 							} // end if
 
 						} // end for y
 
 					} // end else	
 
 			} // end if	
 }


}