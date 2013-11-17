#include "tools.h"
#include "tmath.h"
#include "defines.h"
#include "Modules.h"
#include "Graphics.h"
#include "Polygon.h"
#include "BmpImg.h"

namespace t3d {

void DrawTexturedBilerpTriangleINVZB32(PolygonF* face, unsigned char* _dest_buffer, int mem_pitch, 
									   unsigned char* _zbuffer, int zpitch)
{

	// this function draws a textured triangle in 16-bit mode

	int v0=0,
		v1=1,
		v2=2,
		tri_type = TRI_TYPE_NONE,
		irestart = INTERP_LHS;

	int dx,dy,dyl,dyr,      // general deltas
		u,v,z,
		du,dv,dz,
		xi,yi,              // the current interpolated x,y
		ui,vi,zi,           // the current interpolated u,v,z
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
		dzdyl,   
		zl,
		dudyr,
		ur,
		dvdyr,
		vr,
		dzdyr,
		zr;

	int x0,y0,tu0,tv0,tz0,    // cached vertices
		x1,y1,tu1,tv1,tz1,
		x2,y2,tu2,tv2,tz2;

	unsigned int *screen_ptr  = NULL,
				 *screen_line = NULL,
				 *textmap     = NULL,
				 *dest_buffer = (unsigned int*)_dest_buffer;

	unsigned int  *z_ptr = NULL,
				  *zbuffer = (unsigned int*)_zbuffer;

#ifdef DEBUG_ON
	// track rendering stats
	debug_polys_rendered_per_frame++;
#endif

	// extract texture map
	textmap = (unsigned int*)face->texture->Buffer();

	// extract base 2 of texture width
	int texture_shift2 = logbase2ofx[face->texture->Width()];

	// compute actual size of texture and store it
	int texture_size = face->texture->Width()-1;

	// adjust memory pitch to words, divide by 2
	mem_pitch >>=2;

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
		if (FCMP(face->tvlist[v1].y ,face->tvlist[v2].y))
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
		tu0 = (int)(face->tvlist[v0].u0);
		tv0 = (int)(face->tvlist[v0].v0);
		tz0 = (1 << FIXP28_SHIFT) / (int)(face->tvlist[v0].z+0.5);

		x1  = (int)(face->tvlist[v1].x+0.0);
		y1  = (int)(face->tvlist[v1].y+0.0);
		tu1 = (int)(face->tvlist[v1].u0);
		tv1 = (int)(face->tvlist[v1].v0);
		tz1 = (1 << FIXP28_SHIFT) / (int)(face->tvlist[v1].z+0.5);

		x2  = (int)(face->tvlist[v2].x+0.0);
		y2  = (int)(face->tvlist[v2].y+0.0);
		tu2 = (int)(face->tvlist[v2].u0);
		tv2 = (int)(face->tvlist[v2].v0);
		tz2 = (1 << FIXP28_SHIFT) / (int)(face->tvlist[v2].z+0.5);


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
				dzdyl = ((tz2 - tz0) << 0)/dy;    

				dxdyr = ((x2 - x1)   << FIXP16_SHIFT)/dy;
				dudyr = ((tu2 - tu1) << FIXP16_SHIFT)/dy;  
				dvdyr = ((tv2 - tv1) << FIXP16_SHIFT)/dy;   
				dzdyr = ((tz2 - tz1) << 0)/dy;  

				// test for y clipping
				if (y0 < min_clip_y)
				{
					// compute overclip
					dy = (min_clip_y - y0);

					// computer new LHS starting values
					xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
					ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
					vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
					zl = dzdyl*dy + (tz0 << 0);

					// compute new RHS starting values
					xr = dxdyr*dy + (x1  << FIXP16_SHIFT);
					ur = dudyr*dy + (tu1 << FIXP16_SHIFT);
					vr = dvdyr*dy + (tv1 << FIXP16_SHIFT);
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

					ul = (tu0 << FIXP16_SHIFT);
					vl = (tv0 << FIXP16_SHIFT);
					zl = (tz0 << 0);

					ur = (tu1 << FIXP16_SHIFT);
					vr = (tv1 << FIXP16_SHIFT);
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
				dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dy;  
				dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dy;    
				dzdyl = ((tz1 - tz0) << 0)/dy;   

				dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dy;
				dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dy;  
				dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dy;   
				dzdyr = ((tz2 - tz0) << 0)/dy;   

				// test for y clipping
				if (y0 < min_clip_y)
				{
					// compute overclip
					dy = (min_clip_y - y0);

					// computer new LHS starting values
					xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
					ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
					vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
					zl = dzdyl*dy + (tz0 << 0);

					// compute new RHS starting values
					xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
					ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
					vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);
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

					ul = (tu0 << FIXP16_SHIFT);
					vl = (tv0 << FIXP16_SHIFT);
					zl = (tz0 << FIXP16_SHIFT);

					ur = (tu0 << FIXP16_SHIFT);
					vr = (tv0 << FIXP16_SHIFT);
					zr = (tz0 << FIXP16_SHIFT);

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

					// compute starting points for u,v interpolants
					ui = ul + FIXP16_ROUND_UP;
					vi = vl + FIXP16_ROUND_UP;
					zi = zl;

					// compute u,v interpolants
					if ((dx = (xend - xstart))>0)
					{
						du = (ur - ul)/dx;
						dv = (vr - vl)/dx;
						dz = (zr - zl)/dx;
					} // end if
					else
					{
						du = (ur - ul);
						dv = (vr - vl);
						dz = (zr - zl);
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
							// compute integral values of u,v
							int uint = ui >> FIXP16_SHIFT;
							int vint = vi >> FIXP16_SHIFT;

							int uint_pls_1 = uint+1;
							if (uint_pls_1 > texture_size) uint_pls_1 = texture_size;

							int vint_pls_1 = vint+1;
							if (vint_pls_1 > texture_size) vint_pls_1 = texture_size;

							int textel00 = textmap[(uint+0)     + ((vint+0) << texture_shift2)];
							int textel10 = textmap[(uint_pls_1) + ((vint+0) << texture_shift2)];
							int textel01 = textmap[(uint+0)     + ((vint_pls_1) << texture_shift2)];
							int textel11 = textmap[(uint_pls_1) + ((vint_pls_1) << texture_shift2)];

							// extract rgb components
							int r_textel00  = ((textel00 >> 16) & 0xff); 
							int g_textel00  = ((textel00 >> 8)  & 0xff); 
							int b_textel00  =  (textel00        & 0xff);

							int r_textel10  = ((textel10 >> 16) & 0xff); 
							int g_textel10  = ((textel10 >> 8)  & 0xff); 
							int b_textel10  =  (textel10        & 0xff);

							int r_textel01  = ((textel01 >> 16) & 0xff); 
							int g_textel01  = ((textel01 >> 8)  & 0xff); 
							int b_textel01  =  (textel01        & 0xff);

							int r_textel11  = ((textel11 >> 16) & 0xff); 
							int g_textel11  = ((textel11 >> 8)  & 0xff); 
							int b_textel11  =  (textel11        & 0xff);

							// compute fractional components of u,v in fixed 24.8 point format
							int dtu = (ui & (0xffff)) >> 8;
							int dtv = (vi & (0xffff)) >> 8;

							int one_minus_dtu = (1 << 8) - dtu;
							int one_minus_dtv = (1 << 8) - dtv;

							// each interpolant has 3 terms, (du), (dv), textel, however
							// the (du) and (dv) terms repeat during each computation of
							// r_textel, g_textel, and b_textel, so we can compute them once
							int one_minus_dtu_x_one_minus_dtv = (one_minus_dtu) * (one_minus_dtv);
							int dtu_x_one_minus_dtv           = (dtu)           * (one_minus_dtv);
							int dtu_x_dtv                     = (dtu)           * (dtv);
							int one_minus_dtu_x_dtv           = (one_minus_dtu) * (dtv);

							// now we are ready to sample the texture 
							int r_textel = one_minus_dtu_x_one_minus_dtv * r_textel00 + 
								dtu_x_one_minus_dtv           * r_textel10 +
								dtu_x_dtv                     * r_textel11 +
								one_minus_dtu_x_dtv           * r_textel01;

							int g_textel = one_minus_dtu_x_one_minus_dtv * g_textel00 + 
								dtu_x_one_minus_dtv           * g_textel10 +
								dtu_x_dtv                     * g_textel11 +
								one_minus_dtu_x_dtv           * g_textel01;

							int b_textel = one_minus_dtu_x_one_minus_dtv * b_textel00 + 
								dtu_x_one_minus_dtv           * b_textel10 +
								dtu_x_dtv                     * b_textel11 +
								one_minus_dtu_x_dtv           * b_textel01;

							// write textel
							screen_ptr[xi] = _RGB32BIT(255, r_textel >> 16, g_textel >> 16, b_textel >> 16);

							// update z-buffer
							z_ptr[xi] = zi;           
						} // end if

						// interpolate u,v,z
						ui+=du;
						vi+=dv;
						zi+=dz;
					} // end for xi

					// interpolate u,v,x along right and left edge
					xl+=dxdyl;
					ul+=dudyl;
					vl+=dvdyl;
					zl+=dzdyl;

					xr+=dxdyr;
					ur+=dudyr;
					vr+=dvdyr;
					zr+=dzdyr;

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

					// compute starting points for u,v interpolants
					ui = ul + FIXP16_ROUND_UP;
					vi = vl + FIXP16_ROUND_UP;
					zi = zl;

					// compute u,v interpolants
					if ((dx = (xend - xstart))>0)
					{
						du = (ur - ul)/dx;
						dv = (vr - vl)/dx;
						dz = (zr - zl)/dx;
					} // end if
					else
					{
						du = (ur - ul);
						dv = (vr - vl);
						dz = (zr - zl);
					} // end else

					// draw span
					for (xi=xstart; xi < xend; xi++)
					{
						// test if z of current pixel is nearer than current z buffer value
						if (zi > z_ptr[xi])
						{
							// compute integral values of u,v
							int uint = ui >> FIXP16_SHIFT;
							int vint = vi >> FIXP16_SHIFT;

							int uint_pls_1 = uint+1;
							if (uint_pls_1 > texture_size) uint_pls_1 = texture_size;

							int vint_pls_1 = vint+1;
							if (vint_pls_1 > texture_size) vint_pls_1 = texture_size;

							int textel00 = textmap[(uint+0)     + ((vint+0) << texture_shift2)];
							int textel10 = textmap[(uint_pls_1) + ((vint+0) << texture_shift2)];
							int textel01 = textmap[(uint+0)     + ((vint_pls_1) << texture_shift2)];
							int textel11 = textmap[(uint_pls_1) + ((vint_pls_1) << texture_shift2)];

							// extract rgb components
							int r_textel00  = ((textel00 >> 16) & 0xff); 
							int g_textel00  = ((textel00 >> 8)  & 0xff); 
							int b_textel00  =  (textel00        & 0xff);

							int r_textel10  = ((textel10 >> 16) & 0xff); 
							int g_textel10  = ((textel10 >> 8)  & 0xff); 
							int b_textel10  =  (textel10        & 0xff);

							int r_textel01  = ((textel01 >> 16) & 0xff); 
							int g_textel01  = ((textel01 >> 8)  & 0xff); 
							int b_textel01  =  (textel01        & 0xff);

							int r_textel11  = ((textel11 >> 16) & 0xff); 
							int g_textel11  = ((textel11 >> 8)  & 0xff); 
							int b_textel11  =  (textel11        & 0xff);

							// compute fractional components of u,v in fixed 24.8 point format
							int dtu = (ui & (0xffff)) >> 8;
							int dtv = (vi & (0xffff)) >> 8;

							int one_minus_dtu = (1 << 8) - dtu;
							int one_minus_dtv = (1 << 8) - dtv;

							// each interpolant has 3 terms, (du), (dv), textel, however
							// the (du) and (dv) terms repeat during each computation of
							// r_textel, g_textel, and b_textel, so we can compute them once
							int one_minus_dtu_x_one_minus_dtv = (one_minus_dtu) * (one_minus_dtv);
							int dtu_x_one_minus_dtv           = (dtu)           * (one_minus_dtv);
							int dtu_x_dtv                     = (dtu)           * (dtv);
							int one_minus_dtu_x_dtv           = (one_minus_dtu) * (dtv);

							// now we are ready to sample the texture 
							int r_textel = one_minus_dtu_x_one_minus_dtv * r_textel00 + 
								dtu_x_one_minus_dtv           * r_textel10 +
								dtu_x_dtv                     * r_textel11 +
								one_minus_dtu_x_dtv           * r_textel01;

							int g_textel = one_minus_dtu_x_one_minus_dtv * g_textel00 + 
								dtu_x_one_minus_dtv           * g_textel10 +
								dtu_x_dtv                     * g_textel11 +
								one_minus_dtu_x_dtv           * g_textel01;

							int b_textel = one_minus_dtu_x_one_minus_dtv * b_textel00 + 
								dtu_x_one_minus_dtv           * b_textel10 +
								dtu_x_dtv                     * b_textel11 +
								one_minus_dtu_x_dtv           * b_textel01;

							// write textel
							screen_ptr[xi] = _RGB32BIT(255, r_textel >> 16, g_textel >> 16, b_textel >> 16);

							// update z-buffer
							z_ptr[xi] = zi;           
						} // end if

						// interpolate u,v,z
						ui+=du;
						vi+=dv;
						zi+=dz;
					} // end for xi

					// interpolate u,v,x along right and left edge
					xl+=dxdyl;
					ul+=dudyl;
					vl+=dvdyl;
					zl+=dzdyl;

					xr+=dxdyr;
					ur+=dudyr;
					vr+=dvdyr;
					zr+=dzdyr;

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
					dzdyl = ((tz2 - tz1) << 0)/dyl;  

					// RHS
					dyr = (y2 - y0);	

					dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
					dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
					dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   
					dzdyr = ((tz2 - tz0) << 0)/dyr;   

					// compute overclip
					dyr = (min_clip_y - y0);
					dyl = (min_clip_y - y1);

					// computer new LHS starting values
					xl = dxdyl*dyl + (x1  << FIXP16_SHIFT);
					ul = dudyl*dyl + (tu1 << FIXP16_SHIFT);
					vl = dvdyl*dyl + (tv1 << FIXP16_SHIFT);
					zl = dzdyl*dyl + (tz1 << 0);

					// compute new RHS starting values
					xr = dxdyr*dyr + (x0  << FIXP16_SHIFT);
					ur = dudyr*dyr + (tu0 << FIXP16_SHIFT);
					vr = dvdyr*dyr + (tv0 << FIXP16_SHIFT);
					zr = dzdyr*dyr + (tz0 << 0);

					// compute new starting y
					ystart = min_clip_y;

					// test if we need swap to keep rendering left to right
					if (dxdyr > dxdyl)
					{
						std::swap(dxdyl,dxdyr);
						std::swap(dudyl,dudyr);
						std::swap(dvdyl,dvdyr);
						std::swap(dzdyl,dzdyr);
						std::swap(xl,xr);
						std::swap(ul,ur);
						std::swap(vl,vr);
						std::swap(zl,zr);
						std::swap(x1,x2);
						std::swap(y1,y2);
						std::swap(tu1,tu2);
						std::swap(tv1,tv2);
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
						dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dyl;  
						dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dyl;    
						dzdyl = ((tz1 - tz0) << 0)/dyl;  

						// RHS
						dyr = (y2 - y0);	

						dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
						dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
						dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   
						dzdyr = ((tz2 - tz0) << 0)/dyr;   

						// compute overclip
						dy = (min_clip_y - y0);

						// computer new LHS starting values
						xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
						ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
						vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
						zl = dzdyl*dy + (tz0 << 0);

						// compute new RHS starting values
						xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
						ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
						vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);
						zr = dzdyr*dy + (tz0 << 0);

						// compute new starting y
						ystart = min_clip_y;

						// test if we need swap to keep rendering left to right
						if (dxdyr < dxdyl)
						{
							std::swap(dxdyl,dxdyr);
							std::swap(dudyl,dudyr);
							std::swap(dvdyl,dvdyr);
							std::swap(dzdyl,dzdyr);
							std::swap(xl,xr);
							std::swap(ul,ur);
							std::swap(vl,vr);
							std::swap(zl,zr);
							std::swap(x1,x2);
							std::swap(y1,y2);
							std::swap(tu1,tu2);
							std::swap(tv1,tv2);
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
						dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dyl;  
						dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dyl;    
						dzdyl = ((tz1 - tz0) << 0)/dyl;   

						// RHS
						dyr = (y2 - y0);	

						dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dyr;
						dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
						dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   		
						dzdyr = ((tz2 - tz0) << 0)/dyr;  

						// no clipping y

						// set starting values
						xl = (x0 << FIXP16_SHIFT);
						xr = (x0 << FIXP16_SHIFT);

						ul = (tu0 << FIXP16_SHIFT);
						vl = (tv0 << FIXP16_SHIFT);
						zl = (tz0 << 0);

						ur = (tu0 << FIXP16_SHIFT);
						vr = (tv0 << FIXP16_SHIFT);
						zr = (tz0 << 0);

						// set starting y
						ystart = y0;

						// test if we need swap to keep rendering left to right
						if (dxdyr < dxdyl)
						{
							std::swap(dxdyl,dxdyr);
							std::swap(dudyl,dudyr);
							std::swap(dvdyl,dvdyr);
							std::swap(dzdyl,dzdyr);
							std::swap(xl,xr);
							std::swap(ul,ur);
							std::swap(vl,vr);
							std::swap(zl,zr);
							std::swap(x1,x2);
							std::swap(y1,y2);
							std::swap(tu1,tu2);
							std::swap(tv1,tv2);
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
						screen_ptr = dest_buffer + (ystart * mem_pitch);

						// point zbuffer to starting line
						z_ptr = zbuffer + (ystart * zpitch);

						for (yi = ystart; yi < yend; yi++)
						{
							// compute span endpoints
							xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
							xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

							// compute starting points for u,v interpolants
							ui = ul + FIXP16_ROUND_UP;
							vi = vl + FIXP16_ROUND_UP;
							zi = zl;

							// compute u,v interpolants
							if ((dx = (xend - xstart))>0)
							{
								du = (ur - ul)/dx;
								dv = (vr - vl)/dx;
								dz = (zr - zl)/dx;
							} // end if
							else
							{
								du = (ur - ul);
								dv = (vr - vl);
								dz = (zr - zl);
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
									// compute integral values of u,v
									int uint = ui >> FIXP16_SHIFT;
									int vint = vi >> FIXP16_SHIFT;

									int uint_pls_1 = uint+1;
									if (uint_pls_1 > texture_size) uint_pls_1 = texture_size;

									int vint_pls_1 = vint+1;
									if (vint_pls_1 > texture_size) vint_pls_1 = texture_size;

									int textel00 = textmap[(uint+0)     + ((vint+0) << texture_shift2)];
									int textel10 = textmap[(uint_pls_1) + ((vint+0) << texture_shift2)];
									int textel01 = textmap[(uint+0)     + ((vint_pls_1) << texture_shift2)];
									int textel11 = textmap[(uint_pls_1) + ((vint_pls_1) << texture_shift2)];

									// extract rgb components
									int r_textel00  = ((textel00 >> 16) & 0xff); 
									int g_textel00  = ((textel00 >> 8)  & 0xff); 
									int b_textel00  =  (textel00        & 0xff);

									int r_textel10  = ((textel10 >> 16) & 0xff); 
									int g_textel10  = ((textel10 >> 8)  & 0xff); 
									int b_textel10  =  (textel10        & 0xff);

									int r_textel01  = ((textel01 >> 16) & 0xff); 
									int g_textel01  = ((textel01 >> 8)  & 0xff); 
									int b_textel01  =  (textel01        & 0xff);

									int r_textel11  = ((textel11 >> 16) & 0xff); 
									int g_textel11  = ((textel11 >> 8)  & 0xff); 
									int b_textel11  =  (textel11        & 0xff);

									// compute fractional components of u,v in fixed 24.8 point format
									int dtu = (ui & (0xffff)) >> 8;
									int dtv = (vi & (0xffff)) >> 8;

									int one_minus_dtu = (1 << 8) - dtu;
									int one_minus_dtv = (1 << 8) - dtv;

									// each interpolant has 3 terms, (du), (dv), textel, however
									// the (du) and (dv) terms repeat during each computation of
									// r_textel, g_textel, and b_textel, so we can compute them once
									int one_minus_dtu_x_one_minus_dtv = (one_minus_dtu) * (one_minus_dtv);
									int dtu_x_one_minus_dtv           = (dtu)           * (one_minus_dtv);
									int dtu_x_dtv                     = (dtu)           * (dtv);
									int one_minus_dtu_x_dtv           = (one_minus_dtu) * (dtv);

									// now we are ready to sample the texture 
									int r_textel = one_minus_dtu_x_one_minus_dtv * r_textel00 + 
										dtu_x_one_minus_dtv           * r_textel10 +
										dtu_x_dtv                     * r_textel11 +
										one_minus_dtu_x_dtv           * r_textel01;

									int g_textel = one_minus_dtu_x_one_minus_dtv * g_textel00 + 
										dtu_x_one_minus_dtv           * g_textel10 +
										dtu_x_dtv                     * g_textel11 +
										one_minus_dtu_x_dtv           * g_textel01;

									int b_textel = one_minus_dtu_x_one_minus_dtv * b_textel00 + 
										dtu_x_one_minus_dtv           * b_textel10 +
										dtu_x_dtv                     * b_textel11 +
										one_minus_dtu_x_dtv           * b_textel01;

									// write textel
									screen_ptr[xi] = _RGB32BIT(255, r_textel >> 16, g_textel >> 16, b_textel >> 16);

									// update z-buffer
									z_ptr[xi] = zi;           
								} // end if

								// interpolate u,v,z
								ui+=du;
								vi+=dv;
								zi+=dz;
							} // end for xi

							// interpolate u,v,x along right and left edge
							xl+=dxdyl;
							ul+=dudyl;
							vl+=dvdyl;
							zl+=dzdyl;

							xr+=dxdyr;
							ur+=dudyr;
							vr+=dvdyr;
							zr+=dzdyr;

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
									dzdyl = ((tz2 - tz1) << 0)/dyl;   

									// set starting values
									xl = (x1  << FIXP16_SHIFT);
									ul = (tu1 << FIXP16_SHIFT);
									vl = (tv1 << FIXP16_SHIFT);
									zl = (tz1 << 0);

									// interpolate down on LHS to even up
									xl+=dxdyl;
									ul+=dudyl;
									vl+=dvdyl;
									zl+=dzdyl;
								} // end if
								else
								{
									// RHS
									dyr = (y1 - y2);	

									dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
									dudyr = ((tu1 - tu2) << FIXP16_SHIFT)/dyr;  
									dvdyr = ((tv1 - tv2) << FIXP16_SHIFT)/dyr;   		
									dzdyr = ((tz1 - tz2) << 0)/dyr;  

									// set starting values
									xr = (x2  << FIXP16_SHIFT);
									ur = (tu2 << FIXP16_SHIFT);
									vr = (tv2 << FIXP16_SHIFT);
									zr = (tz2 << 0);

									// interpolate down on RHS to even up
									xr+=dxdyr;
									ur+=dudyr;
									vr+=dvdyr;
									zr+=dzdyr;

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

							// compute starting points for u,v interpolants
							ui = ul + FIXP16_ROUND_UP;
							vi = vl + FIXP16_ROUND_UP;
							zi = zl;

							// compute u,v interpolants
							if ((dx = (xend - xstart))>0)
							{
								du = (ur - ul)/dx;
								dv = (vr - vl)/dx;
								dz = (zr - zl)/dx;
							} // end if
							else
							{
								du = (ur - ul);
								dv = (vr - vl);
								dz = (zr - zl);
							} // end else

							// draw span
							for (xi=xstart; xi < xend; xi++)
							{
								// test if z of current pixel is nearer than current z buffer value
								if (zi > z_ptr[xi])
								{
									// compute integral values of u,v
									int uint = ui >> FIXP16_SHIFT;
									int vint = vi >> FIXP16_SHIFT;

									int uint_pls_1 = uint+1;
									if (uint_pls_1 > texture_size) uint_pls_1 = texture_size;

									int vint_pls_1 = vint+1;
									if (vint_pls_1 > texture_size) vint_pls_1 = texture_size;

									int textel00 = textmap[(uint+0)     + ((vint+0) << texture_shift2)];
									int textel10 = textmap[(uint_pls_1) + ((vint+0) << texture_shift2)];
									int textel01 = textmap[(uint+0)     + ((vint_pls_1) << texture_shift2)];
									int textel11 = textmap[(uint_pls_1) + ((vint_pls_1) << texture_shift2)];

									// extract rgb components
									int r_textel00  = ((textel00 >> 16) & 0xff); 
									int g_textel00  = ((textel00 >> 8)  & 0xff); 
									int b_textel00  =  (textel00        & 0xff);

									int r_textel10  = ((textel10 >> 16) & 0xff); 
									int g_textel10  = ((textel10 >> 8)  & 0xff); 
									int b_textel10  =  (textel10        & 0xff);

									int r_textel01  = ((textel01 >> 16) & 0xff); 
									int g_textel01  = ((textel01 >> 8)  & 0xff); 
									int b_textel01  =  (textel01        & 0xff);

									int r_textel11  = ((textel11 >> 16) & 0xff); 
									int g_textel11  = ((textel11 >> 8)  & 0xff); 
									int b_textel11  =  (textel11        & 0xff);

									// compute fractional components of u,v in fixed 24.8 point format
									int dtu = (ui & (0xffff)) >> 8;
									int dtv = (vi & (0xffff)) >> 8;

									int one_minus_dtu = (1 << 8) - dtu;
									int one_minus_dtv = (1 << 8) - dtv;

									// each interpolant has 3 terms, (du), (dv), textel, however
									// the (du) and (dv) terms repeat during each computation of
									// r_textel, g_textel, and b_textel, so we can compute them once
									int one_minus_dtu_x_one_minus_dtv = (one_minus_dtu) * (one_minus_dtv);
									int dtu_x_one_minus_dtv           = (dtu)           * (one_minus_dtv);
									int dtu_x_dtv                     = (dtu)           * (dtv);
									int one_minus_dtu_x_dtv           = (one_minus_dtu) * (dtv);

									// now we are ready to sample the texture 
									int r_textel = one_minus_dtu_x_one_minus_dtv * r_textel00 + 
										dtu_x_one_minus_dtv           * r_textel10 +
										dtu_x_dtv                     * r_textel11 +
										one_minus_dtu_x_dtv           * r_textel01;

									int g_textel = one_minus_dtu_x_one_minus_dtv * g_textel00 + 
										dtu_x_one_minus_dtv           * g_textel10 +
										dtu_x_dtv                     * g_textel11 +
										one_minus_dtu_x_dtv           * g_textel01;

									int b_textel = one_minus_dtu_x_one_minus_dtv * b_textel00 + 
										dtu_x_one_minus_dtv           * b_textel10 +
										dtu_x_dtv                     * b_textel11 +
										one_minus_dtu_x_dtv           * b_textel01;

									// write textel
									screen_ptr[xi] = _RGB32BIT(255, r_textel >> 16, g_textel >> 16, b_textel >> 16);

									// update z-buffer
									z_ptr[xi] = zi;           
								} // end if

								// interpolate u,v
								ui+=du;
								vi+=dv;
								zi+=dz;
							} // end for xi

							// interpolate u,v,x along right and left edge
							xl+=dxdyl;
							ul+=dudyl;
							vl+=dvdyl;
							zl+=dzdyl;

							xr+=dxdyr;
							ur+=dudyr;
							vr+=dvdyr;
							zr+=dzdyr;

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
									dzdyl = ((tz2 - tz1) << 0)/dyl;   

									// set starting values
									xl = (x1  << FIXP16_SHIFT);
									ul = (tu1 << FIXP16_SHIFT);
									vl = (tv1 << FIXP16_SHIFT);
									zl = (tz1 << 0);

									// interpolate down on LHS to even up
									xl+=dxdyl;
									ul+=dudyl;
									vl+=dvdyl;
									zl+=dzdyl;
								} // end if
								else
								{
									// RHS
									dyr = (y1 - y2);	

									dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
									dudyr = ((tu1 - tu2) << FIXP16_SHIFT)/dyr;  
									dvdyr = ((tv1 - tv2) << FIXP16_SHIFT)/dyr;   		
									dzdyr = ((tz1 - tz2) << 0)/dyr; 

									// set starting values
									xr = (x2  << FIXP16_SHIFT);
									ur = (tu2 << FIXP16_SHIFT);
									vr = (tv2 << FIXP16_SHIFT);
									zr = (tz2 << 0);

									// interpolate down on RHS to even up
									xr+=dxdyr;
									ur+=dudyr;
									vr+=dvdyr;
									zr+=dzdyr;

								} // end else

							} // end if

						} // end for y

					} // end else	

			} // end if
}


}