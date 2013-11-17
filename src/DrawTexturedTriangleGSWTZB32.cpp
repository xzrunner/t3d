#include "tools.h"
#include "tmath.h"
#include "defines.h"
#include "Modules.h"
#include "Graphics.h"
#include "Polygon.h"
#include "BmpImg.h"

namespace t3d {

void DrawTexturedTriangleGSWTZB32(PolygonF* face, unsigned char* _dest_buffer, int mem_pitch, 
								  unsigned char* _zbuffer, int zpitch)
{

	// this function draws a textured gouraud shaded polygon, and z bufferedbased on the affine texture mapper, 
	// we simply interpolate the (R,G,B) values across the polygons along with the texture coordinates
	// and then modulate to get the final color 

	int v0=0,
		v1=1,
		v2=2,
		temp=0,
		tri_type = TRI_TYPE_NONE,
		irestart = INTERP_LHS;


	int dx,dy,dyl,dyr,      // general deltas
		u,v,w,z, s,t,
		du,dv,dw,dz, ds, dt, 
		xi,yi,             // the current interpolated x,y
		ui,vi,wi,zi, si, ti,    // the current interpolated u,v
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
		dudyl,    
		ul,
		dvdyl,   
		vl,
		dwdyl,   
		wl,
		dzdyl,   
		zl,
		dsdyl,    
		sl,
		dtdyl,   
		tl,
		dudyr,
		ur,
		dvdyr,
		vr,
		dwdyr,
		wr,
		dzdyr,
		zr,
		dsdyr,
		sr,
		dtdyr,
		tr;

	int x0,y0,tu0,tv0,tw0, tz0, ts0,tt0,    // cached vertices
		x1,y1,tu1,tv1,tw1, tz1, ts1,tt1,
		x2,y2,tu2,tv2,tw2, tz2, ts2,tt2;

	int tmpa;
	int r_base0, g_base0, b_base0,
		r_base1, g_base1, b_base1,
		r_base2, g_base2, b_base2;


	unsigned int r_textel, g_textel, b_textel;
	unsigned int textel;

	unsigned int *screen_ptr  = NULL,
				 *screen_line = NULL,
				 *textmap     = NULL,
				 *dest_buffer = (unsigned int*)_dest_buffer;

	unsigned int *z_ptr = NULL,
				 *zbuffer = (unsigned int*)_zbuffer;

#ifdef DEBUG_ON
	// track rendering stats
	debug_polys_rendered_per_frame++;
#endif


	// extract texture map
	textmap = (unsigned int*)face->texture->Buffer();

	// extract base 2 of texture width
	int texture_shift2 = logbase2ofx[face->texture->Width()];

	// adjust memory pitch to words, divide by 2
	mem_pitch >>= 2;

	// adjust zbuffer pitch for 32 bit alignment
	zpitch >>= 2;

	// apply fill convention to coordinates
	face->tvlist[0].x = (int)(face->tvlist[0].x+0.0);
	face->tvlist[0].y = (int)(face->tvlist[0].y+0.0);

	face->tvlist[1].x = (int)(face->tvlist[1].x+0.0);
	face->tvlist[1].y = (int)(face->tvlist[1].y+0.0);

	face->tvlist[2].x = (int)(face->tvlist[2].x+0.0);
	face->tvlist[2].y = (int)(face->tvlist[2].y+0.0);

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

		// assume 8.8.8 format -- sorry!
		// we can't afford a function call in the inner loops, so we must write 
		// two hard coded versions, if we want support for both 5.6.5, and 5.5.5
		_RGB8888FROM32BIT(face->lit_color[v0], &tmpa, &r_base0, &g_base0, &b_base0);
		_RGB8888FROM32BIT(face->lit_color[v1], &tmpa, &r_base1, &g_base1, &b_base1);
		_RGB8888FROM32BIT(face->lit_color[v2], &tmpa, &r_base2, &g_base2, &b_base2);

		// extract vertices for processing, now that we have order
		x0  = (int)(face->tvlist[v0].x+0.0);
		y0  = (int)(face->tvlist[v0].y+0.0);

		tz0 = (int)(face->tvlist[v0].z+0.5);
		ts0 = (int)(face->tvlist[v0].u0);
		tt0 = (int)(face->tvlist[v0].v0);

		tu0 = r_base0;
		tv0 = g_base0; 
		tw0 = b_base0; 

		x1  = (int)(face->tvlist[v1].x+0.0);
		y1  = (int)(face->tvlist[v1].y+0.0);

		tz1 = (int)(face->tvlist[v1].z+0.5);
		ts1 = (int)(face->tvlist[v1].u0);
		tt1 = (int)(face->tvlist[v1].v0);

		tu1 = r_base1;
		tv1 = g_base1; 
		tw1 = b_base1; 

		x2  = (int)(face->tvlist[v2].x+0.0);
		y2  = (int)(face->tvlist[v2].y+0.0);

		tz2 = (int)(face->tvlist[v2].z+0.5);
		ts2 = (int)(face->tvlist[v2].u0);
		tt2 = (int)(face->tvlist[v2].v0);

		tu2 = r_base2; 
		tv2 = g_base2; 
		tw2 = b_base2; 

		// degenerate triangle
		if ( ((x0 == x1) && (x1 == x2)) || ((y0 ==  y1) && (y1 == y2)))
			return;

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
				dudyl = ((tu2 - tu0) << FIXP16_SHIFT)/dy;  
				dvdyl = ((tv2 - tv0) << FIXP16_SHIFT)/dy;    
				dwdyl = ((tw2 - tw0) << FIXP16_SHIFT)/dy;  
				dzdyl = ((tz2 - tz0) << FIXP16_SHIFT)/dy; 

				dsdyl = ((ts2 - ts0) << FIXP16_SHIFT)/dy;    
				dtdyl = ((tt2 - tt0) << FIXP16_SHIFT)/dy;  

				dxdyr = ((x2 - x1)   << FIXP16_SHIFT)/dy;
				dudyr = ((tu2 - tu1) << FIXP16_SHIFT)/dy;  
				dvdyr = ((tv2 - tv1) << FIXP16_SHIFT)/dy;   
				dwdyr = ((tw2 - tw1) << FIXP16_SHIFT)/dy;   
				dzdyr = ((tz2 - tz1) << FIXP16_SHIFT)/dy;   

				dsdyr = ((ts2 - ts1) << FIXP16_SHIFT)/dy;   
				dtdyr = ((tt2 - tt1) << FIXP16_SHIFT)/dy;   

				// test for y clipping
				if (y0 < min_clip_y)
				{
					// compute overclip
					dy = (min_clip_y - y0);

					// computer new LHS starting values
					xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
					ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
					vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
					wl = dwdyl*dy + (tw0 << FIXP16_SHIFT);
					zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

					sl = dsdyl*dy + (ts0 << FIXP16_SHIFT);
					tl = dtdyl*dy + (tt0 << FIXP16_SHIFT);

					// compute new RHS starting values
					xr = dxdyr*dy + (x1  << FIXP16_SHIFT);
					ur = dudyr*dy + (tu1 << FIXP16_SHIFT);
					vr = dvdyr*dy + (tv1 << FIXP16_SHIFT);
					wr = dwdyr*dy + (tw1 << FIXP16_SHIFT);
					zr = dzdyr*dy + (tz1 << FIXP16_SHIFT);

					sr = dsdyr*dy + (ts1 << FIXP16_SHIFT);
					tr = dtdyr*dy + (tt1 << FIXP16_SHIFT);

					// compute new starting y
					ystart = min_clip_y;

				} // end if
				else
				{
					// no clipping

					// set starting values
					xl = (x0 << FIXP16_SHIFT);
					xr = (x1 << FIXP16_SHIFT);

					ul = (tu0 << FIXP16_SHIFT);
					vl = (tv0 << FIXP16_SHIFT);
					wl = (tw0 << FIXP16_SHIFT);
					zl = (tz0 << FIXP16_SHIFT);

					sl = (ts0 << FIXP16_SHIFT);
					tl = (tt0 << FIXP16_SHIFT);


					ur = (tu1 << FIXP16_SHIFT);
					vr = (tv1 << FIXP16_SHIFT);
					wr = (tw1 << FIXP16_SHIFT);
					zr = (tz1 << FIXP16_SHIFT);

					sr = (ts1 << FIXP16_SHIFT);
					tr = (tt1 << FIXP16_SHIFT);

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
				dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dy;  
				dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dy;    
				dwdyl = ((tw1 - tw0) << FIXP16_SHIFT)/dy; 
				dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dy; 

				dsdyl = ((ts1 - ts0) << FIXP16_SHIFT)/dy;    
				dtdyl = ((tt1 - tt0) << FIXP16_SHIFT)/dy; 

				dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dy;
				dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dy;  
				dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dy;   
				dwdyr = ((tw2 - tw0) << FIXP16_SHIFT)/dy;   
				dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dy;   

				dsdyr = ((ts2 - ts0) << FIXP16_SHIFT)/dy;   
				dtdyr = ((tt2 - tt0) << FIXP16_SHIFT)/dy;   


				// test for y clipping
				if (y0 < min_clip_y)
				{
					// compute overclip
					dy = (min_clip_y - y0);

					// computer new LHS starting values
					xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
					ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
					vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
					wl = dwdyl*dy + (tw0 << FIXP16_SHIFT);
					zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

					sl = dsdyl*dy + (ts0 << FIXP16_SHIFT);
					tl = dtdyl*dy + (tt0 << FIXP16_SHIFT);

					// compute new RHS starting values
					xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
					ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
					vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);
					wr = dwdyr*dy + (tw0 << FIXP16_SHIFT);
					zr = dzdyr*dy + (tz0 << FIXP16_SHIFT);

					sr = dsdyr*dy + (ts0 << FIXP16_SHIFT);
					tr = dtdyr*dy + (tt0 << FIXP16_SHIFT);


					// compute new starting y
					ystart = min_clip_y;

				} // end if
				else
				{
					// no clipping

					// set starting values
					xl = (x0 << FIXP16_SHIFT);
					xr = (x0 << FIXP16_SHIFT);

					ul = (tu0 << FIXP16_SHIFT);
					vl = (tv0 << FIXP16_SHIFT);
					wl = (tw0 << FIXP16_SHIFT);
					zl = (tz0 << FIXP16_SHIFT);

					sl = (ts0 << FIXP16_SHIFT);
					tl = (tt0 << FIXP16_SHIFT);


					ur = (tu0 << FIXP16_SHIFT);
					vr = (tv0 << FIXP16_SHIFT);
					wr = (tw0 << FIXP16_SHIFT);
					zr = (tz0 << FIXP16_SHIFT);

					sr = (ts0 << FIXP16_SHIFT);
					tr = (tt0 << FIXP16_SHIFT);


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

				// point zbuffer to starting line
				z_ptr = zbuffer + (ystart * zpitch);

				for (yi = ystart; yi < yend; yi++)
				{
					// compute span endpoints
					xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
					xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

					// compute starting points for u,v,w interpolants
					ui = ul + FIXP16_ROUND_UP;
					vi = vl + FIXP16_ROUND_UP;
					wi = wl + FIXP16_ROUND_UP;
					zi = zl + FIXP16_ROUND_UP;

					si = sl + FIXP16_ROUND_UP;
					ti = tl + FIXP16_ROUND_UP;

					// compute u,v interpolants
					if ((dx = (xend - xstart))>0)
					{
						du = (ur - ul)/dx;
						dv = (vr - vl)/dx;
						dw = (wr - wl)/dx;
						dz = (zr - zl)/dx;

						ds = (sr - sl)/dx;
						dt = (tr - tl)/dx;

					} // end if
					else
					{
						du = (ur - ul);
						dv = (vr - vl);
						dw = (wr - wl);
						dz = (zr - zl);

						ds = (sr - sl);
						dt = (tr - tl);

					} // end else

					///////////////////////////////////////////////////////////////////////

					// test for x clipping, LHS
					if (xstart < min_clip_x)
					{
						// compute x overlap
						dx = min_clip_x - xstart;

						// slide interpolants over
						ui+=dx*du;
						vi+=dx*dv;
						wi+=dx*dw;
						zi+=dx*dz;

						si+=dx*ds;
						ti+=dx*dt;

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
						// write thru z buffer
						// write textel assume 8.8.8
						// get textel first
						textel = textmap[(si >> FIXP16_SHIFT) + ((ti >> FIXP16_SHIFT) << texture_shift2)];

						// extract rgb components
						_RGB8888FROM32BIT(textel, &tmpa, &r_textel, &g_textel, &b_textel);

						// modulate textel with gouraud shading
						r_textel*=ui; 
						g_textel*=vi;
						b_textel*=wi;

						// finally write pixel, note that we did the math such that the results are r*32, g*64, b*32
						// hence we need to divide the results by 32,64,32 respetively, BUT since we need to shift
						// the results to fit into the destination 8.8.8 word, we can take advantage of the shifts
						// and they all cancel out for the most part, but we will need logical anding, we will do
						// it later when we optimize more...
						screen_ptr[xi] = _RGB32BIT(255, r_textel >> (FIXP16_SHIFT+8), g_textel >> (FIXP16_SHIFT+8), b_textel >> (FIXP16_SHIFT+8));

						// update z-buffer
						z_ptr[xi] = zi;   

						// interpolate u,v
						ui+=du;
						vi+=dv;
						wi+=dw;
						zi+=dz;

						si+=ds;
						ti+=dt;

					} // end for xi

					// interpolate u,v,w,x along right and left edge
					xl+=dxdyl;
					ul+=dudyl;
					vl+=dvdyl;
					wl+=dwdyl;
					zl+=dzdyl;

					sl+=dsdyl;
					tl+=dtdyl;

					xr+=dxdyr;
					ur+=dudyr;
					vr+=dvdyr;
					wr+=dwdyr;
					zr+=dzdyr;

					sr+=dsdyr;
					tr+=dtdyr;

					// advance screen ptr
					screen_ptr+=mem_pitch;

					// advance zbuffer ptr
					z_ptr+=zpitch;

				} // end for y

			} // end if clip
			else
			{
				// non-clip version

				// point screen ptr to starting line
				screen_ptr = dest_buffer + (ystart * mem_pitch);

				// point zbuffer to starting line
				z_ptr = zbuffer + (ystart * zpitch);

				for (yi = ystart; yi < yend; yi++)
				{
					// compute span endpoints
					xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
					xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

					// compute starting points for u,v,w interpolants
					ui = ul + FIXP16_ROUND_UP;
					vi = vl + FIXP16_ROUND_UP;
					wi = wl + FIXP16_ROUND_UP;
					zi = zl + FIXP16_ROUND_UP;

					si = sl + FIXP16_ROUND_UP;
					ti = tl + FIXP16_ROUND_UP;

					// compute u,v interpolants
					if ((dx = (xend - xstart))>0)
					{
						du = (ur - ul)/dx;
						dv = (vr - vl)/dx;
						dw = (wr - wl)/dx;
						dz = (zr - zl)/dx;

						ds = (sr - sl)/dx;
						dt = (tr - tl)/dx;

					} // end if
					else
					{
						du = (ur - ul);
						dv = (vr - vl);
						dw = (wr - wl);
						dz = (zr - zl);

						ds = (sr - sl);
						dt = (tr - tl);

					} // end else

					// draw span
					for (xi=xstart; xi < xend; xi++)
					{
						// write textel assume 8.8.8
						// write thru z buffer
						// get textel first
						textel = textmap[(si >> FIXP16_SHIFT) + ((ti >> FIXP16_SHIFT) << texture_shift2)];

						// extract rgb components
						_RGB8888FROM32BIT(textel, &tmpa, &r_textel, &g_textel, &b_textel);

						// modulate textel with gouraud shading
						r_textel*=ui; 
						g_textel*=vi;
						b_textel*=wi;

						// finally write pixel, note that we did the math such that the results are r*32, g*64, b*32
						// hence we need to divide the results by 32,64,32 respetively, BUT since we need to shift
						// the results to fit into the destination 8.8.8 word, we can take advantage of the shifts
						// and they all cancel out for the most part, but we will need logical anding, we will do
						// it later when we optimize more...
						screen_ptr[xi] = _RGB32BIT(255, 
												   r_textel >> (FIXP16_SHIFT+8), 
												   g_textel >> (FIXP16_SHIFT+8), 
												   b_textel >> (FIXP16_SHIFT+8));

						// update z-buffer
						z_ptr[xi] = zi;   

						// interpolate u,v
						ui+=du;
						vi+=dv;
						wi+=dw;
						zi+=dz;

						si+=ds;
						ti+=dt;

					} // end for xi

					// interpolate u,v,w,x along right and left edge
					xl+=dxdyl;
					ul+=dudyl;
					vl+=dvdyl;
					wl+=dwdyl;
					zl+=dzdyl;

					sl+=dsdyl;
					tl+=dtdyl;

					xr+=dxdyr;
					ur+=dudyr;
					vr+=dvdyr;
					wr+=dwdyr;
					zr+=dzdyr;

					sr+=dsdyr;
					tr+=dtdyr;

					// advance screen ptr
					screen_ptr+=mem_pitch;

					// advance zbuffer ptr
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
					dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
					dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;    
					dwdyl = ((tw2 - tw1) << FIXP16_SHIFT)/dyl;  
					dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;  

					dsdyl = ((ts2 - ts1) << FIXP16_SHIFT)/dyl;    
					dtdyl = ((tt2 - tt1) << FIXP16_SHIFT)/dyl;  

					// RHS
					dyr = (y2 - y0);	

					dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
					dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
					dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   
					dwdyr = ((tw2 - tw0) << FIXP16_SHIFT)/dyr;   
					dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;   

					dsdyr = ((ts2 - ts0) << FIXP16_SHIFT)/dyr;   
					dtdyr = ((tt2 - tt0) << FIXP16_SHIFT)/dyr;  

					// compute overclip
					dyr = (min_clip_y - y0);
					dyl = (min_clip_y - y1);

					// computer new LHS starting values
					xl = dxdyl*dyl + (x1  << FIXP16_SHIFT);

					ul = dudyl*dyl + (tu1 << FIXP16_SHIFT);
					vl = dvdyl*dyl + (tv1 << FIXP16_SHIFT);
					wl = dwdyl*dyl + (tw1 << FIXP16_SHIFT);
					zl = dzdyl*dyl + (tz1 << FIXP16_SHIFT);

					sl = dsdyl*dyl + (ts1 << FIXP16_SHIFT);
					tl = dtdyl*dyl + (tt1 << FIXP16_SHIFT);

					// compute new RHS starting values
					xr = dxdyr*dyr + (x0  << FIXP16_SHIFT);

					ur = dudyr*dyr + (tu0 << FIXP16_SHIFT);
					vr = dvdyr*dyr + (tv0 << FIXP16_SHIFT);
					wr = dwdyr*dyr + (tw0 << FIXP16_SHIFT);
					zr = dzdyr*dyr + (tz0 << FIXP16_SHIFT);

					sr = dsdyr*dyr + (ts0 << FIXP16_SHIFT);
					tr = dtdyr*dyr + (tt0 << FIXP16_SHIFT);

					// compute new starting y
					ystart = min_clip_y;

					// test if we need swap to keep rendering left to right
					if (dxdyr > dxdyl)
					{
						std::swap(dxdyl,dxdyr);
						std::swap(dudyl,dudyr);
						std::swap(dvdyl,dvdyr);
						std::swap(dwdyl,dwdyr);
						std::swap(dzdyl,dzdyr);

						std::swap(dsdyl,dsdyr);
						std::swap(dtdyl,dtdyr);

						std::swap(xl,xr);
						std::swap(ul,ur);
						std::swap(vl,vr);
						std::swap(wl,wr);
						std::swap(zl,zr);

						std::swap(sl,sr);
						std::swap(tl,tr);

						std::swap(x1,x2);
						std::swap(y1,y2);
						std::swap(tu1,tu2);
						std::swap(tv1,tv2);
						std::swap(tw1,tw2);
						std::swap(tz1,tz2);

						std::swap(ts1,ts2);
						std::swap(tt1,tt2);

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
						dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dyl;  
						dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dyl;    
						dwdyl = ((tw1 - tw0) << FIXP16_SHIFT)/dyl; 
						dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dyl; 

						dsdyl = ((ts1 - ts0) << FIXP16_SHIFT)/dyl;    
						dtdyl = ((tt1 - tt0) << FIXP16_SHIFT)/dyl; 

						// RHS
						dyr = (y2 - y0);	

						dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
						dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
						dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   
						dwdyr = ((tw2 - tw0) << FIXP16_SHIFT)/dyr;   
						dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;   

						dsdyr = ((ts2 - ts0) << FIXP16_SHIFT)/dyr;   
						dtdyr = ((tt2 - tt0) << FIXP16_SHIFT)/dyr;   

						// compute overclip
						dy = (min_clip_y - y0);

						// computer new LHS starting values
						xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
						ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
						vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
						wl = dwdyl*dy + (tw0 << FIXP16_SHIFT);
						zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

						sl = dsdyl*dy + (ts0 << FIXP16_SHIFT);
						tl = dtdyl*dy + (tt0 << FIXP16_SHIFT);

						// compute new RHS starting values
						xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
						ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
						vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);
						wr = dwdyr*dy + (tw0 << FIXP16_SHIFT);
						zr = dzdyr*dy + (tz0 << FIXP16_SHIFT);

						sr = dsdyr*dy + (ts0 << FIXP16_SHIFT);
						tr = dtdyr*dy + (tt0 << FIXP16_SHIFT);

						// compute new starting y
						ystart = min_clip_y;

						// test if we need swap to keep rendering left to right
						if (dxdyr < dxdyl)
						{
							std::swap(dxdyl,dxdyr);
							std::swap(dudyl,dudyr);
							std::swap(dvdyl,dvdyr);
							std::swap(dwdyl,dwdyr);
							std::swap(dzdyl,dzdyr);

							std::swap(dsdyl,dsdyr);
							std::swap(dtdyl,dtdyr);

							std::swap(xl,xr);
							std::swap(ul,ur);
							std::swap(vl,vr);
							std::swap(wl,wr);
							std::swap(zl,zr);

							std::swap(sl,sr);
							std::swap(tl,tr);

							std::swap(x1,x2);
							std::swap(y1,y2);
							std::swap(tu1,tu2);
							std::swap(tv1,tv2);
							std::swap(tw1,tw2);
							std::swap(tz1,tz2);

							std::swap(ts1,ts2);
							std::swap(tt1,tt2);

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
						dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dyl;  
						dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dyl;    
						dwdyl = ((tw1 - tw0) << FIXP16_SHIFT)/dyl;   
						dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dyl;  

						dsdyl = ((ts1 - ts0) << FIXP16_SHIFT)/dyl;    
						dtdyl = ((tt1 - tt0) << FIXP16_SHIFT)/dyl;   

						// RHS
						dyr = (y2 - y0);	

						dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dyr;
						dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
						dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   		
						dwdyr = ((tw2 - tw0) << FIXP16_SHIFT)/dyr;
						dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;

						dsdyr = ((ts2 - ts0) << FIXP16_SHIFT)/dyr;   		
						dtdyr = ((tt2 - tt0) << FIXP16_SHIFT)/dyr;

						// no clipping y

						// set starting values
						xl = (x0 << FIXP16_SHIFT);
						xr = (x0 << FIXP16_SHIFT);

						ul = (tu0 << FIXP16_SHIFT);
						vl = (tv0 << FIXP16_SHIFT);
						wl = (tw0 << FIXP16_SHIFT);
						zl = (tz0 << FIXP16_SHIFT);

						sl = (ts0 << FIXP16_SHIFT);
						tl = (tt0 << FIXP16_SHIFT);

						ur = (tu0 << FIXP16_SHIFT);
						vr = (tv0 << FIXP16_SHIFT);
						wr = (tw0 << FIXP16_SHIFT);
						zr = (tz0 << FIXP16_SHIFT);

						sr = (ts0 << FIXP16_SHIFT);
						tr = (tt0 << FIXP16_SHIFT);

						// set starting y
						ystart = y0;

						// test if we need swap to keep rendering left to right
						if (dxdyr < dxdyl)
						{
							std::swap(dxdyl,dxdyr);
							std::swap(dudyl,dudyr);
							std::swap(dvdyl,dvdyr);
							std::swap(dwdyl,dwdyr);
							std::swap(dzdyl,dzdyr);

							std::swap(dsdyl,dsdyr);
							std::swap(dtdyl,dtdyr);


							std::swap(xl,xr);
							std::swap(ul,ur);
							std::swap(vl,vr);
							std::swap(wl,wr);
							std::swap(zl,zr);

							std::swap(sl,sr);
							std::swap(tl,tr);


							std::swap(x1,x2);
							std::swap(y1,y2);
							std::swap(tu1,tu2);
							std::swap(tv1,tv2);
							std::swap(tw1,tw2);
							std::swap(tz1,tz2);

							std::swap(ts1,ts2);
							std::swap(tt1,tt2);

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

						// point zbuffer to starting line
						z_ptr = zbuffer + (ystart * zpitch);

						for (yi = ystart; yi < yend; yi++)
						{
							// compute span endpoints
							xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
							xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

							// compute starting points for u,v,w interpolants
							ui = ul + FIXP16_ROUND_UP;
							vi = vl + FIXP16_ROUND_UP;
							wi = wl + FIXP16_ROUND_UP;
							zi = zl + FIXP16_ROUND_UP;

							si = sl + FIXP16_ROUND_UP;
							ti = tl + FIXP16_ROUND_UP;

							// compute u,v interpolants
							if ((dx = (xend - xstart))>0)
							{
								du = (ur - ul)/dx;
								dv = (vr - vl)/dx;
								dw = (wr - wl)/dx;
								dz = (zr - zl)/dx;

								ds = (sr - sl)/dx;
								dt = (tr - tl)/dx;

							} // end if
							else
							{
								du = (ur - ul);
								dv = (vr - vl);
								dw = (wr - wl);
								dz = (zr - zl);

								ds = (sr - sl);
								dt = (tr - tl);

							} // end else

							///////////////////////////////////////////////////////////////////////

							// test for x clipping, LHS
							if (xstart < min_clip_x)
							{
								// compute x overlap
								dx = min_clip_x - xstart;

								// slide interpolants over
								ui+=dx*du;
								vi+=dx*dv;
								wi+=dx*dw;
								zi+=dx*dz;

								si+=dx*ds;
								ti+=dx*dt;

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
								// write thru z buffer
								// get textel first
								textel = textmap[(si >> FIXP16_SHIFT) + ((ti >> FIXP16_SHIFT) << texture_shift2)];

								// extract rgb components
								_RGB8888FROM32BIT(textel, &tmpa, &r_textel, &g_textel, &b_textel);

								// modulate textel with gouraud shading
								r_textel*=ui; 
								g_textel*=vi;
								b_textel*=wi;

								// finally write pixel, note that we did the math such that the results are r*32, g*64, b*32
								// hence we need to divide the results by 32,64,32 respetively, BUT since we need to shift
								// the results to fit into the destination 8.8.8 word, we can take advantage of the shifts
								// and they all cancel out for the most part, but we will need logical anding, we will do
								// it later when we optimize more...
								screen_ptr[xi] = _RGB32BIT(255, 
														   r_textel >> (FIXP16_SHIFT+8), 
														   g_textel >> (FIXP16_SHIFT+8), 
														   b_textel >> (FIXP16_SHIFT+8));

								// update z-buffer
								z_ptr[xi] = zi;   

								// interpolate u,v
								ui+=du;
								vi+=dv;
								wi+=dw;
								zi+=dz;

								si+=ds;
								ti+=dt;

							} // end for xi

							// interpolate u,v,w,x along right and left edge
							xl+=dxdyl;
							ul+=dudyl;
							vl+=dvdyl;
							wl+=dwdyl;
							zl+=dzdyl;

							sl+=dsdyl;
							tl+=dtdyl;

							xr+=dxdyr;
							ur+=dudyr;
							vr+=dvdyr;
							wr+=dwdyr;
							zr+=dzdyr;

							sr+=dsdyr;
							tr+=dtdyr;

							// advance screen ptr
							screen_ptr+=mem_pitch;

							// advance zbuffer ptr
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
									dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
									dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;   		
									dwdyl = ((tw2 - tw1) << FIXP16_SHIFT)/dyl;  
									dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;  

									dsdyl = ((ts2 - ts1) << FIXP16_SHIFT)/dyl;   		
									dtdyl = ((tt2 - tt1) << FIXP16_SHIFT)/dyl;  


									// set starting values
									xl = (x1  << FIXP16_SHIFT);
									ul = (tu1 << FIXP16_SHIFT);
									vl = (tv1 << FIXP16_SHIFT);
									wl = (tw1 << FIXP16_SHIFT);
									zl = (tz1 << FIXP16_SHIFT);

									sl = (ts1 << FIXP16_SHIFT);
									tl = (tt1 << FIXP16_SHIFT);

									// interpolate down on LHS to even up
									xl+=dxdyl;
									ul+=dudyl;
									vl+=dvdyl;
									wl+=dwdyl;
									zl+=dzdyl;

									sl+=dsdyl;
									tl+=dtdyl;

								} // end if
								else
								{
									// RHS
									dyr = (y1 - y2);	

									dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
									dudyr = ((tu1 - tu2) << FIXP16_SHIFT)/dyr;  
									dvdyr = ((tv1 - tv2) << FIXP16_SHIFT)/dyr;   		
									dwdyr = ((tw1 - tw2) << FIXP16_SHIFT)/dyr;   		
									dzdyr = ((tz1 - tz2) << FIXP16_SHIFT)/dyr;   

									dsdyr = ((ts1 - ts2) << FIXP16_SHIFT)/dyr;   		
									dtdyr = ((tt1 - tt2) << FIXP16_SHIFT)/dyr;  

									// set starting values
									xr = (x2  << FIXP16_SHIFT);
									ur = (tu2 << FIXP16_SHIFT);
									vr = (tv2 << FIXP16_SHIFT);
									wr = (tw2 << FIXP16_SHIFT);
									zr = (tz2 << FIXP16_SHIFT);

									sr = (ts2 << FIXP16_SHIFT);
									tr = (tt2 << FIXP16_SHIFT);

									// interpolate down on RHS to even up
									xr+=dxdyr;
									ur+=dudyr;
									vr+=dvdyr;
									wr+=dwdyr;
									zr+=dzdyr;

									sr+=dsdyr;
									tr+=dtdyr;
								} // end else

							} // end if

						} // end for y

					} // end if
					else
					{
						// no x clipping
						// point screen ptr to starting line
						screen_ptr = dest_buffer + (ystart * mem_pitch);

						// point zbuffer to starting line
						z_ptr = zbuffer + (ystart * zpitch);

						for (yi = ystart; yi < yend; yi++)
						{
							// compute span endpoints
							xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
							xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

							// compute starting points for u,v,w interpolants
							ui = ul + FIXP16_ROUND_UP;
							vi = vl + FIXP16_ROUND_UP;
							wi = wl + FIXP16_ROUND_UP;
							zi = zl + FIXP16_ROUND_UP;

							si = sl + FIXP16_ROUND_UP;
							ti = tl + FIXP16_ROUND_UP;

							// compute u,v interpolants
							if ((dx = (xend - xstart))>0)
							{
								du = (ur - ul)/dx;
								dv = (vr - vl)/dx;
								dw = (wr - wl)/dx;
								dz = (zr - zl)/dx;

								ds = (sr - sl)/dx;
								dt = (tr - tl)/dx;

							} // end if
							else
							{
								du = (ur - ul);
								dv = (vr - vl);
								dw = (wr - wl);
								dz = (zr - zl);

								ds = (sr - sl);
								dt = (tr - tl);

							} // end else

							// draw span
							for (xi=xstart; xi < xend; xi++)
							{
								// write textel assume 8.8.8
								// write thru z buffer
								// get textel first
								textel = textmap[(si >> FIXP16_SHIFT) + ((ti >> FIXP16_SHIFT) << texture_shift2)];

								// extract rgb components
								_RGB8888FROM32BIT(textel, &tmpa, &r_textel, &g_textel, &b_textel);

								// modulate textel with gouraud shading
								r_textel*=ui; 
								g_textel*=vi;
								b_textel*=wi;

								// finally write pixel, note that we did the math such that the results are r*32, g*64, b*32
								// hence we need to divide the results by 32,64,32 respetively, BUT since we need to shift
								// the results to fit into the destination 8.8.8 word, we can take advantage of the shifts
								// and they all cancel out for the most part, but we will need logical anding, we will do
								// it later when we optimize more...
								screen_ptr[xi] = _RGB32BIT(255, 
														   r_textel >> (FIXP16_SHIFT+8), 
														   g_textel >> (FIXP16_SHIFT+8), 
														   b_textel >> (FIXP16_SHIFT+8));

								// update z-buffer
								z_ptr[xi] = zi;   

								// interpolate u,v
								ui+=du;
								vi+=dv;
								wi+=dw;
								zi+=dz;

								si+=ds;
								ti+=dt;
							} // end for xi

							// interpolate u,v,w,x along right and left edge
							xl+=dxdyl;
							ul+=dudyl;
							vl+=dvdyl;
							wl+=dwdyl;
							zl+=dzdyl;

							sl+=dsdyl;
							tl+=dtdyl;

							xr+=dxdyr;
							ur+=dudyr;
							vr+=dvdyr;
							wr+=dwdyr;
							zr+=dzdyr;

							sr+=dsdyr;
							tr+=dtdyr;

							// advance screen ptr
							screen_ptr+=mem_pitch;

							// advance zbuffer ptr
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
									dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
									dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;   		
									dwdyl = ((tw2 - tw1) << FIXP16_SHIFT)/dyl;   
									dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;  

									dsdyl = ((ts2 - ts1) << FIXP16_SHIFT)/dyl;   		
									dtdyl = ((tt2 - tt1) << FIXP16_SHIFT)/dyl;   

									// set starting values
									xl = (x1  << FIXP16_SHIFT);
									ul = (tu1 << FIXP16_SHIFT);
									vl = (tv1 << FIXP16_SHIFT);
									wl = (tw1 << FIXP16_SHIFT);
									zl = (tz1 << FIXP16_SHIFT);

									sl = (ts1 << FIXP16_SHIFT);
									tl = (tt1 << FIXP16_SHIFT);

									// interpolate down on LHS to even up
									xl+=dxdyl;
									ul+=dudyl;
									vl+=dvdyl;
									wl+=dwdyl;
									zl+=dzdyl;

									sl+=dsdyl;
									tl+=dtdyl;

								} // end if
								else
								{
									// RHS
									dyr = (y1 - y2);	

									dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
									dudyr = ((tu1 - tu2) << FIXP16_SHIFT)/dyr;  
									dvdyr = ((tv1 - tv2) << FIXP16_SHIFT)/dyr;   		
									dwdyr = ((tw1 - tw2) << FIXP16_SHIFT)/dyr;   
									dzdyr = ((tz1 - tz2) << FIXP16_SHIFT)/dyr;   

									dsdyr = ((ts1 - ts2) << FIXP16_SHIFT)/dyr;   		
									dtdyr = ((tt1 - tt2) << FIXP16_SHIFT)/dyr;   

									// set starting values
									xr = (x2  << FIXP16_SHIFT);
									ur = (tu2 << FIXP16_SHIFT);
									vr = (tv2 << FIXP16_SHIFT);
									wr = (tw2 << FIXP16_SHIFT);
									zr = (tz2 << FIXP16_SHIFT);

									sr = (ts2 << FIXP16_SHIFT);
									tr = (tt2 << FIXP16_SHIFT);

									// interpolate down on RHS to even up
									xr+=dxdyr;
									ur+=dudyr;
									vr+=dvdyr;
									wr+=dwdyr;
									zr+=dzdyr;

									sr+=dsdyr;
									tr+=dtdyr;

								} // end else

							} // end if

						} // end for y

					} // end else	

			} // end if	
}



}