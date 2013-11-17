#include "RenderList.h"

#include <locale.h>
#include <string.h>
#include <float.h>

#include "defines.h"
#include "Camera.h"
#include "PrimitiveDraw.h"
#include "RenderObject.h"
#include "Modules.h"
#include "Graphics.h"
#include "Light.h"
#include "Log.h"
#include "BmpImg.h"

namespace t3d {

bool RenderList::Insert(const Polygon& poly)
{
	// step 0: are we full?
	if (_num_polys >= MAX_POLYS)
		return(0);

	// step 1: copy polygon into next opening in polygon render list

	// point pointer to polygon structure
	_poly_ptrs[_num_polys] = &_poly_data[_num_polys];

	// copy fields
	_poly_data[_num_polys].state		= poly.state;
	_poly_data[_num_polys].attr			= poly.attr;
	_poly_data[_num_polys].color		= poly.color;
	_poly_data[_num_polys].nlength		= poly.nlength;
	_poly_data[_num_polys].texture		= poly.texture;

	// poly could be lit, so copy these too...
	for (size_t i = 0; i < 3; ++i)
		_poly_data[_num_polys].lit_color[i] = poly.lit_color[i];

	// now copy vertices, be careful! later put a loop, but for now
	// know there are 3 vertices always!
	for (size_t i = 0; i < 3; ++i)
		_poly_data[_num_polys].tvlist[i] = poly.vlist[poly.vert[i]];

	// and copy into local vertices too
	for (size_t i = 0; i < 3; ++i)
		_poly_data[_num_polys].vlist[i] = poly.vlist[poly.vert[i]];

	// finally the texture coordinates, this has to be performed manually
	// since at this point in the pipeline the vertices do NOT have texture
	// coordinate, the polygons DO, however, now, there are 3 vertices for 
	// EVERY polygon, rather than vertex sharing, so we can copy the texture
	// coordinates out of the indexed arrays into the VERTEX4DTV1 structures
	for (size_t i = 0; i < 3; ++i) {
		_poly_data[_num_polys].tvlist[i].t = poly.tlist[poly.text[i]];
		_poly_data[_num_polys].vlist[i].t = poly.tlist[poly.text[i]];
	}

	// now the polygon is loaded into the next free array position, but
	// we need to fix up the links

	// test if this is the first entry
	if (_num_polys == 0)
	{
		// set pointers to null, could loop them around though to self
		_poly_data[0].next = NULL;
		_poly_data[0].prev = NULL;
	}
	else
	{
		// first set this node to point to previous node and next node (null)
		_poly_data[_num_polys].next = NULL;
		_poly_data[_num_polys].prev = &_poly_data[_num_polys-1];

		// now set previous node to point to this node
		_poly_data[_num_polys-1].next = &_poly_data[_num_polys];
	}

	// increment number of polys in list
	_num_polys++;

	return true;
}

bool RenderList::Insert(const PolygonF& poly)
{
	// inserts the sent polyface POLYF4DV1 into the render list

	// step 0: are we full?
	if (_num_polys >= MAX_POLYS)
		return false;

	// step 1: copy polygon into next opening in polygon render list

	// point pointer to polygon structure
	_poly_ptrs[_num_polys] = &_poly_data[_num_polys];

	// copy face right into array, thats it
	memcpy((void *)&_poly_data[_num_polys],(void *)&poly, sizeof(poly));

	// now the polygon is loaded into the next free array position, but
	// we need to fix up the links
	// test if this is the first entry
	if (_num_polys == 0)
	{
		// set pointers to null, could loop them around though to self
		_poly_data[0].next = NULL;
		_poly_data[0].prev = NULL;
	} // end if
	else
	{
		// first set this node to point to previous node and next node (null)
		_poly_data[_num_polys].next = NULL;
		_poly_data[_num_polys].prev = &_poly_data[_num_polys-1];

		// now set previous node to point to this node
		_poly_data[_num_polys-1].next = &_poly_data[_num_polys];
	} // end else

	// increment number of polys in list
	_num_polys++;

	// return successful insertion
	return true;
}

bool RenderList::Insert(const RenderObject& obj, bool insert_local)
{
	// converts the entire object into a face list and then inserts
	// the visible, active, non-clipped, non-culled polygons into
	// the render list, also note the flag insert_local control 
	// whether or not the vlist_local or vlist_trans vertex list
	// is used, thus you can insert an object "raw" totally untranformed
	// if you set insert_local to 1, default is 0, that is you would
	// only insert an object after at least the local to world transform
	// the last parameter is used to control if their has been
	// a lighting step that has generated a light value stored
	// in the upper 16-bits of color, if lighting_on = 1 then
	// this value is used to overwrite the base color of the 
	// polygon when its sent to the rendering list

	unsigned int base_color; // save base color of polygon

	// is this objective inactive or culled or invisible?
	if (!(obj._state & OBJECT_STATE_ACTIVE) ||
		(obj._state & OBJECT_STATE_CULLED) ||
		!(obj._state & OBJECT_STATE_VISIBLE))
		return(0); 

	// the object is valid, let's rip it apart polygon by polygon
	for (int poly = 0; poly < obj._num_polys; poly++)
	{
		// acquire polygon
		Polygon* curr_poly = const_cast<Polygon*>(&obj._plist[poly]);

		// first is this polygon even visible?
		if (!(curr_poly->state & POLY_STATE_ACTIVE) ||
			(curr_poly->state & POLY_STATE_CLIPPED ) ||
			(curr_poly->state & POLY_STATE_BACKFACE) )
			continue; // move onto next poly

		// override vertex list polygon refers to
		// the case that you want the local coords used
		// first save old pointer
		Vertex* vlist_old = curr_poly->vlist;

		if (insert_local)
			curr_poly->vlist = const_cast<Vertex*>(obj._vlist_local);
		else
			curr_poly->vlist = const_cast<Vertex*>(obj._vlist_trans);

		// now insert this polygon
		if (!Insert(*curr_poly))
		{
			// fix vertex list pointer
			curr_poly->vlist = vlist_old;

			// the whole object didn't fit!
			return false;
		}

		// fix vertex list pointer
		curr_poly->vlist = vlist_old;
	}

	return true;
}

void RenderList::Transform(const mat4& mt, int coord_select)
{
	// this function simply transforms all of the polygons vertices in the local or trans
	// array of the render list by the sent matrix

	// what coordinates should be transformed?
	switch(coord_select)
	{
	case TRANSFORM_LOCAL_ONLY:
		{
			for (int poly = 0; poly < _num_polys; poly++)
			{
				// acquire current polygon
				PolygonF* curr_poly = _poly_ptrs[poly];

				// is this polygon valid?
				// transform this polygon if and only if it's not clipped, not culled,
				// active, and visible, note however the concept of "backface" is 
				// irrelevant in a wire frame engine though
				if ((curr_poly==NULL) || !(curr_poly->state & POLY_STATE_ACTIVE) ||
					(curr_poly->state & POLY_STATE_CLIPPED ) ||
					(curr_poly->state & POLY_STATE_BACKFACE) )
					continue; // move onto next poly

				// all good, let's transform 
				for (int vertex = 0; vertex < 3; vertex++)
				{
					// transform the vertex by mt
					vec4 presult; // hold result of each transformation

					// transform point
					presult = mt * curr_poly->vlist[vertex].v;

					// store result back
					curr_poly->vlist[vertex].v = presult;

				} // end for vertex

			} // end for poly

		} break;

	case TRANSFORM_TRANS_ONLY:
		{
			// transform each "transformed" vertex of the render list
			// remember, the idea of the tvlist[] array is to accumulate
			// transformations
			for (int poly = 0; poly < _num_polys; poly++)
			{
				// acquire current polygon
				PolygonF* curr_poly = _poly_ptrs[poly];

				// is this polygon valid?
				// transform this polygon if and only if it's not clipped, not culled,
				// active, and visible, note however the concept of "backface" is 
				// irrelevant in a wire frame engine though
				if ((curr_poly==NULL) || !(curr_poly->state & POLY_STATE_ACTIVE) ||
					(curr_poly->state & POLY_STATE_CLIPPED ) ||
					(curr_poly->state & POLY_STATE_BACKFACE) )
					continue; // move onto next poly

				// all good, let's transform 
				for (int vertex = 0; vertex < 3; vertex++)
				{
					// transform the vertex by mt
					vec4 presult; // hold result of each transformation

					// transform point
					presult = mt * curr_poly->tvlist[vertex].v;

					// store result back
					curr_poly->tvlist[vertex].v = presult;
				} // end for vertex

			} // end for poly

		} break;

	case TRANSFORM_LOCAL_TO_TRANS:
		{
			// transform each local/model vertex of the render list and store result
			// in "transformed" vertex list
			for (int poly = 0; poly < _num_polys; poly++)
			{
				// acquire current polygon
				PolygonF* curr_poly = _poly_ptrs[poly];

				// is this polygon valid?
				// transform this polygon if and only if it's not clipped, not culled,
				// active, and visible, note however the concept of "backface" is 
				// irrelevant in a wire frame engine though
				if ((curr_poly==NULL) || !(curr_poly->state & POLY_STATE_ACTIVE) ||
					(curr_poly->state & POLY_STATE_CLIPPED ) ||
					(curr_poly->state & POLY_STATE_BACKFACE) )
					continue; // move onto next poly

				// all good, let's transform 
				for (int vertex = 0; vertex < 3; vertex++)
					// transform the vertex by mt
					curr_poly->tvlist[vertex].v = mt * curr_poly->vlist[vertex].v;

			} // end for poly

		} break;

	default: break;

	} // end switch	
}

void RenderList::ModelToWorld(const vec4& world_pos, 
							  int coord_select)
{
	// NOTE: Not matrix based
	// this function converts the local model coordinates of the
	// sent render list into world coordinates, the results are stored
	// in the transformed vertex list (tvlist) within the renderlist

	// interate thru vertex list and transform all the model/local 
	// coords to world coords by translating the vertex list by
	// the amount world_pos and storing the results in tvlist[]
	// is this polygon valid?

	if (coord_select == TRANSFORM_LOCAL_TO_TRANS)
	{
		for (int poly = 0; poly < _num_polys; poly++)
		{
			// acquire current polygon
			PolygonF* curr_poly = _poly_ptrs[poly];

			// transform this polygon if and only if it's not clipped, not culled,
			// active, and visible, note however the concept of "backface" is 
			// irrelevant in a wire frame engine though
			if ((curr_poly==NULL) || !(curr_poly->state & POLY_STATE_ACTIVE) ||
				(curr_poly->state & POLY_STATE_CLIPPED ) ||
				(curr_poly->state & POLY_STATE_BACKFACE) )
				continue; // move onto next poly

			// all good, let's transform 
			for (int vertex = 0; vertex < 3; vertex++)
				curr_poly->tvlist[vertex].v = curr_poly->vlist[vertex].v + world_pos;

		} // end for poly
	} // end if local
	else // TRANSFORM_TRANS_ONLY
	{
		for (int poly = 0; poly < _num_polys; poly++)
		{
			// acquire current polygon
			PolygonF* curr_poly = _poly_ptrs[poly];

			// transform this polygon if and only if it's not clipped, not culled,
			// active, and visible, note however the concept of "backface" is 
			// irrelevant in a wire frame engine though
			if ((curr_poly==NULL) || !(curr_poly->state & POLY_STATE_ACTIVE) ||
				(curr_poly->state & POLY_STATE_CLIPPED ) ||
				(curr_poly->state & POLY_STATE_BACKFACE) )
				continue; // move onto next poly

			for (int vertex = 0; vertex < 3; vertex++)
				curr_poly->tvlist[vertex].v = curr_poly->tvlist[vertex].v + world_pos;

		} // end for poly

	} // end else	
}

void RenderList::RemoveBackfaces(const Camera& cam)
{
	// NOTE: this is not a matrix based function
	// this function removes the backfaces from polygon list
	// the function does this based on the polygon list data
	// tvlist along with the camera position (only)
	// note that only the backface state is set in each polygon

	for (int poly = 0; poly < _num_polys; poly++)
	{
		// acquire current polygon
		PolygonF* curr_poly = _poly_ptrs[poly];

		// is this polygon valid?
		// test this polygon if and only if it's not clipped, not culled,
		// active, and visible and not 2 sided. Note we test for backface in the event that
		// a previous call might have already determined this, so why work
		// harder!
		if ((curr_poly==NULL) || !(curr_poly->state & POLY_STATE_ACTIVE) ||
			(curr_poly->state & POLY_STATE_CLIPPED ) || 
			(curr_poly->attr  & POLY_ATTR_2SIDED)    ||
			(curr_poly->state & POLY_STATE_BACKFACE) )
			continue; // move onto next poly

		// we need to compute the normal of this polygon face, and recall
		// that the vertices are in cw order, u = p0->p1, v=p0->p2, n=uxv
		vec3 u, v, n;

		// build u, v
		vec3 p0(curr_poly->tvlist[0].x, curr_poly->tvlist[0].y, curr_poly->tvlist[0].z),
			 p1(curr_poly->tvlist[1].x, curr_poly->tvlist[1].y, curr_poly->tvlist[1].z),
			 p2(curr_poly->tvlist[2].x, curr_poly->tvlist[2].y, curr_poly->tvlist[2].z);
		u = p1 - p0;
		v = p2 - p0;

		// compute cross product
		n = u.Cross(v);

		// now create eye vector to viewpoint
		vec3 cam_pos(cam.Pos().x, cam.Pos().y, cam.Pos().z);
		vec3 view = cam_pos - p0;

		// and finally, compute the dot product
		float dp = n.Dot(view);

		// if the sign is > 0 then visible, 0 = scathing, < 0 invisible
		if (dp <= 0.0 )
			SET_BIT(curr_poly->state, POLY_STATE_BACKFACE);
	}
}

void RenderList::WorldToCamera(const Camera& cam)
{
	// NOTE: this is a matrix based function
	// this function transforms each polygon in the global render list
	// to camera coordinates based on the sent camera transform matrix
	// you would use this function instead of the object based function
	// if you decided earlier in the pipeline to turn each object into 
	// a list of polygons and then add them to the global render list
	// the conversion of an object into polygons probably would have
	// happened after object culling, local transforms, local to world
	// and backface culling, so the minimum number of polygons from
	// each object are in the list, note that the function assumes
	// that at LEAST the local to world transform has been called
	// and the polygon data is in the transformed list tvlist of
	// the POLYF4DV1 object

	// transform each polygon in the render list into camera coordinates
	// assumes the render list has already been transformed to world
	// coordinates and the result is in tvlist[] of each polygon object

	for (int poly = 0; poly < _num_polys; poly++)
	{
		// acquire current polygon
		PolygonF* curr_poly = _poly_ptrs[poly];

		// is this polygon valid?
		// transform this polygon if and only if it's not clipped, not culled,
		// active, and visible, note however the concept of "backface" is 
		// irrelevant in a wire frame engine though
		if ((curr_poly==NULL) || !(curr_poly->state & POLY_STATE_ACTIVE) ||
			(curr_poly->state & POLY_STATE_CLIPPED ) ||
			(curr_poly->state & POLY_STATE_BACKFACE) )
			continue; // move onto next poly

		// all good, let's transform 
		for (int vertex = 0; vertex < 3; vertex++)
		{
			// transform the vertex by the mcam matrix within the camera
			// it better be valid!
			// transform point
			curr_poly->tvlist[vertex].v = cam.CameraMat() * curr_poly->tvlist[vertex].v;

		} // end for vertex

	} // end for poly
}

void RenderList::CameraToPerspective(const Camera& cam)
{
	// NOTE: this is not a matrix based function
	// this function transforms each polygon in the global render list
	// into perspective coordinates, based on the 
	// sent camera object, 
	// you would use this function instead of the object based function
	// if you decided earlier in the pipeline to turn each object into 
	// a list of polygons and then add them to the global render list

	// transform each polygon in the render list into camera coordinates
	// assumes the render list has already been transformed to world
	// coordinates and the result is in tvlist[] of each polygon object

	for (int poly = 0; poly < _num_polys; poly++)
	{
		// acquire current polygon
		PolygonF* curr_poly = _poly_ptrs[poly];

		// is this polygon valid?
		// transform this polygon if and only if it's not clipped, not culled,
		// active, and visible, note however the concept of "backface" is 
		// irrelevant in a wire frame engine though
		if ((curr_poly==NULL) || !(curr_poly->state & POLY_STATE_ACTIVE) ||
			(curr_poly->state & POLY_STATE_CLIPPED ) ||
			(curr_poly->state & POLY_STATE_BACKFACE) )
			continue; // move onto next poly

		// all good, let's transform 
		for (int vertex = 0; vertex < 3; vertex++)
		{
			float z = curr_poly->tvlist[vertex].z;

			// transform the vertex by the view parameters in the camera
			curr_poly->tvlist[vertex].x = cam.ViewDist()*curr_poly->tvlist[vertex].x/z;
			curr_poly->tvlist[vertex].y = cam.ViewDist()*curr_poly->tvlist[vertex].y*cam.AspectRatio()/z;
			// z = z, so no change

			// not that we are NOT dividing by the homogenous w coordinate since
			// we are not using a matrix operation for this version of the function 

		} // end for vertex

	} // end for poly
}

void RenderList::PerspectiveToScreen(const Camera& cam)
{
	// NOTE: this is not a matrix based function
	// this function transforms the perspective coordinates of the render
	// list into screen coordinates, based on the sent viewport in the camera
	// assuming that the viewplane coordinates were normalized
	// you would use this function instead of the object based function
	// if you decided earlier in the pipeline to turn each object into 
	// a list of polygons and then add them to the global render list
	// you would only call this function if you previously performed
	// a normalized perspective transform

	// transform each polygon in the render list from perspective to screen 
	// coordinates assumes the render list has already been transformed 
	// to normalized perspective coordinates and the result is in tvlist[]
	for (int poly = 0; poly < _num_polys; poly++)
	{
		// acquire current polygon
		PolygonF* curr_poly = _poly_ptrs[poly];

		// is this polygon valid?
		// transform this polygon if and only if it's not clipped, not culled,
		// active, and visible, note however the concept of "backface" is 
		// irrelevant in a wire frame engine though
		if ((curr_poly==NULL) || !(curr_poly->state & POLY_STATE_ACTIVE) ||
			(curr_poly->state & POLY_STATE_CLIPPED ) ||
			(curr_poly->state & POLY_STATE_BACKFACE) )
			continue; // move onto next poly

		float alpha = (0.5*cam.ViewportWidth()-0.5);
		float beta  = (0.5*cam.ViewportHeight()-0.5);

		// all good, let's transform 
		for (int vertex = 0; vertex < 3; vertex++)
		{
			// the vertex is in perspective normalized coords from -1 to 1
			// on each axis, simple scale them and invert y axis and project
			// to screen

			// transform the vertex by the view parameters in the camera
			curr_poly->tvlist[vertex].x = alpha + alpha*curr_poly->tvlist[vertex].x;
			curr_poly->tvlist[vertex].y = beta  - beta *curr_poly->tvlist[vertex].y;
		} // end for vertex

	} // end for poly
}

void RenderList::Reset()
{
	// this function intializes and resets the sent render list and
	// redies it for polygons/faces to be inserted into it
	// note that the render list in this version is composed
	// of an array FACE4DV1 pointer objects, you call this
	// function each frame

	// since we are tracking the number of polys in the
	// list via _num_polys we can set it to 0
	// but later we will want a more robust scheme if
	// we generalize the linked list more and disconnect
	// it from the polygon pointer list
	_num_polys = 0; // that was hard!	
}

void RenderList::DrawContext(const RenderContext& rc)
{
	// this function renders the rendering list, it's based on the new
	// rendering context data structure which is container for everything
	// we need to consider when rendering, z, 1/z buffering, alpha, mipmapping,
	// perspective, bilerp, etc. the function is basically a merge of all the functions
	// we have written thus far, so its rather long, but better than having 
	// 20-30 rendering functions for all possible permutations!

	PolygonF face; // temp face used to render polygon
	int alpha;      // alpha of the face

	// we need to try and separate as much conditional logic as possible
	// at the beginning of the function, so we can minimize it inline during
	// the traversal of the polygon list, let's start by subclassing which
	// kind of rendering we are doing none, z buffered, or 1/z buffered
	                                      
	if (rc.attr & RENDER_ATTR_NOBUFFER) ////////////////////////////////////
	{
	// no buffering at all

	// at this point, all we have is a list of polygons and it's time
	// to draw them
	for (int poly=0; poly < _num_polys; poly++)
		{
		// render this polygon if and only if it's not clipped, not culled,
		// active, and visible, note however the concecpt of "backface" is 
		// irrelevant in a wire frame engine though
		if (!(_poly_ptrs[poly]->state & POLY_STATE_ACTIVE) ||
			 (_poly_ptrs[poly]->state & POLY_STATE_CLIPPED ) ||
			 (_poly_ptrs[poly]->state & POLY_STATE_BACKFACE) )
		   continue; // move onto next poly

		// test for alpha override
		if (rc.alpha_override>= 0)
		   {
		   // set alpha to override value
		   alpha = rc.alpha_override;
		   }  // end if 
		else
			{
			// extract alpha (even if there isn't any)
			alpha = ((_poly_ptrs[poly]->color & 0xff000000) >> 24);
			} // end else


		// need to test for textured first, since a textured poly can either
		// be emissive, or flat shaded, hence we need to call different
		// rasterizers    
		if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_TEXTURE)
		   {
		   // set the vertices
		   face.tvlist[0].x = (float)_poly_ptrs[poly]->tvlist[0].x;
		   face.tvlist[0].y = (float)_poly_ptrs[poly]->tvlist[0].y;
		   face.tvlist[0].z  = (float)_poly_ptrs[poly]->tvlist[0].z;
		   face.tvlist[0].u0 = (float)_poly_ptrs[poly]->tvlist[0].u0;
		   face.tvlist[0].v0 = (float)_poly_ptrs[poly]->tvlist[0].v0;

		   face.tvlist[1].x = (float)_poly_ptrs[poly]->tvlist[1].x;
		   face.tvlist[1].y = (float)_poly_ptrs[poly]->tvlist[1].y;
		   face.tvlist[1].z  = (float)_poly_ptrs[poly]->tvlist[1].z;
		   face.tvlist[1].u0 = (float)_poly_ptrs[poly]->tvlist[1].u0;
		   face.tvlist[1].v0 = (float)_poly_ptrs[poly]->tvlist[1].v0;

		   face.tvlist[2].x = (float)_poly_ptrs[poly]->tvlist[2].x;
		   face.tvlist[2].y = (float)_poly_ptrs[poly]->tvlist[2].y;
		   face.tvlist[2].z  = (float)_poly_ptrs[poly]->tvlist[2].z;
		   face.tvlist[2].u0 = (float)_poly_ptrs[poly]->tvlist[2].u0;
		   face.tvlist[2].v0 = (float)_poly_ptrs[poly]->tvlist[2].v0;

		   // test if this is a mipmapped polygon?
		   if (_poly_ptrs[poly]->attr & POLY_ATTR_MIPMAP)
			  {
			  // determine if mipmapping is desired at all globally
			  if (rc.attr & RENDER_ATTR_MIPMAP)
				 {
				 // determine mip level for this polygon

				 // first determine how many miplevels there are in mipchain for this polygon
				 int tmiplevels = logbase2ofx[((BmpImg* *)(_poly_ptrs[poly]->texture))[0]->Width()];

				 // now based on the requested linear miplevel fall off distance, cut
				 // the viewdistance into segments, determine what segment polygon is
				 // in and select mip level -- simple! later you might want something more
				 // robust, also note I only use a single vertex, you might want to find the average
				 // since for long walls perpendicular to view direction this might causing mip
				 // popping mid surface
				 int miplevel = (tmiplevels * _poly_ptrs[poly]->tvlist[0].z / rc.mip_dist);
	          
				 // clamp miplevel
				 if (miplevel > tmiplevels) miplevel = tmiplevels;

				 // based on miplevel select proper texture
				 face.texture = ((BmpImg* *)(_poly_ptrs[poly]->texture))[miplevel];

				 // now we must divide each texture coordinate by 2 per miplevel
				 for (int ts = 0; ts < miplevel; ts++)
					 {
					 face.tvlist[0].u0*=.5;
					 face.tvlist[0].v0*=.5;

					 face.tvlist[1].u0*=.5;
					 face.tvlist[1].v0*=.5;

					 face.tvlist[2].u0*=.5;
					 face.tvlist[2].v0*=.5;
					} // end for

				 } // end if mipmmaping enabled globally
			  else // mipmapping not selected globally
				 {
				 // in this case the polygon IS mipmapped, but the caller has requested NO
				 // mipmapping, so we will support this by selecting mip level 0 since the
				 // texture pointer is pointing to a mip chain regardless
				 face.texture = ((BmpImg* *)(_poly_ptrs[poly]->texture))[0];
	 
				 // note: texture coordinate manipulation is unneeded

				 } // end else

			  } // end if
		   else
			  {
			  // assign the texture without change
			  face.texture = _poly_ptrs[poly]->texture;
			  } // end if
	       
		   // is this a plain emissive texture?
		   if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_CONSTANT)
			  {
			  // draw the textured triangle as emissive

			  if ((rc.attr & RENDER_ATTR_ALPHA) &&
				  ((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
				 {
				 // alpha version
	             
				 // which texture mapper?
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
					DrawTexturedTriangleAlpha32(&face, rc.video_buffer, rc.lpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
					{
					// not supported yet!
					DrawTexturedTriangleAlpha32(&face, rc.video_buffer, rc.lpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
					{
					// not supported yet
					DrawTexturedTriangleAlpha32(&face, rc.video_buffer, rc.lpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
					{
					// test z distance again perspective transition gate
					if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
						{
						// default back to affine
						DrawTexturedTriangleAlpha32(&face, rc.video_buffer, rc.lpitch, alpha);
						} // end if
					else
						{
						// use perspective linear
						// not supported yet
						DrawTexturedTriangleAlpha32(&face, rc.video_buffer, rc.lpitch, alpha);
						} // end if

					 } // end if

				 } // end if
			  else
				 {
				 // non alpha
				 // which texture mapper?
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
					// use bilerp?
					if (rc.attr & RENDER_ATTR_BILERP)
					   DrawTexturedBilerpTriangle32(&face, rc.video_buffer, rc.lpitch);               
					else
					   DrawTexturedTriangle32(&face, rc.video_buffer, rc.lpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
					{
					// not supported yet
					DrawTexturedTriangle32(&face, rc.video_buffer, rc.lpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
					{
					// not supported yet
					DrawTexturedTriangle32(&face, rc.video_buffer, rc.lpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
					{
					// test z distance again perspective transition gate
					if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
						{
						// default back to affine
						DrawTexturedTriangle32(&face, rc.video_buffer, rc.lpitch);
						} // end if
					else
						{
						// use perspective linear
						// not supported yet
						DrawTexturedTriangle32(&face, rc.video_buffer, rc.lpitch);
						} // end if

					} // end if

				 } // end if

			  } // end if
		   else
		   if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_FLAT)
			  {
			  // draw as flat shaded
			  face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];

			  if ((rc.attr & RENDER_ATTR_ALPHA) &&
				  ((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
				 {
				 // alpha version
	             
				 // which texture mapper?
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
					DrawTexturedTriangleFSAlpha32(&face, rc.video_buffer, rc.lpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
					{
					// not supported yet!
					DrawTexturedTriangleFSAlpha32(&face, rc.video_buffer, rc.lpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
					{
					// not supported yet
					DrawTexturedTriangleFSAlpha32(&face, rc.video_buffer, rc.lpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
					{
					// test z distance again perspective transition gate
					if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
						{
						// default back to affine
						DrawTexturedTriangleFSAlpha32(&face, rc.video_buffer, rc.lpitch, alpha);
						} // end if
					else
						{
						// use perspective linear
						// not supported yet
						DrawTexturedTriangleFSAlpha32(&face, rc.video_buffer, rc.lpitch, alpha);
						} // end if

					 } // end if

				 } // end if
			  else
				 {
				 // non alpha
				 // which texture mapper?
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
					DrawTexturedTriangleFS32(&face, rc.video_buffer, rc.lpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
					{
					// not supported yet
					DrawTexturedTriangleFS32(&face, rc.video_buffer, rc.lpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
					{
					// not supported yet
					DrawTexturedTriangleFS32(&face, rc.video_buffer, rc.lpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
					{
					// test z distance again perspective transition gate
					if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
						{
						// default back to affine
						DrawTexturedTriangleFS32(&face, rc.video_buffer, rc.lpitch);
						} // end if
					else
						{
						// use perspective linear
						// not supported yet
						DrawTexturedTriangleFS32(&face, rc.video_buffer, rc.lpitch);
						} // end if

					} // end if

				 } // end if

			  } // end else
		   else
			  {
			  // must be gouraud POLY_ATTR_SHADE_MODE_GOURAUD
			  face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];
			  face.lit_color[1] = _poly_ptrs[poly]->lit_color[1];
			  face.lit_color[2] = _poly_ptrs[poly]->lit_color[2];

			  if ((rc.attr & RENDER_ATTR_ALPHA) &&
				  ((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
				 {
				 // alpha version
	             
				 // which texture mapper?
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
					DrawTexturedTriangleGSAlpha32(&face, rc.video_buffer, rc.lpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
					{
					// not supported yet!
					DrawTexturedTriangleGSAlpha32(&face, rc.video_buffer, rc.lpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
					{
					// not supported yet
					DrawTexturedTriangleGSAlpha32(&face, rc.video_buffer, rc.lpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
					{
					// test z distance again perspective transition gate
					if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
						{
						// default back to affine
						DrawTexturedTriangleGSAlpha32(&face, rc.video_buffer, rc.lpitch, alpha);
						} // end if
					else
						{
						// use perspective linear
						// not supported yet
						DrawTexturedTriangleGSAlpha32(&face, rc.video_buffer, rc.lpitch, alpha);
						} // end if

					 } // end if

				 } // end if
			  else
				 {
				 // non alpha
				 // which texture mapper?
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
					DrawTexturedTriangleGS32(&face, rc.video_buffer, rc.lpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
					{
					// not supported yet
					DrawTexturedTriangleGS32(&face, rc.video_buffer, rc.lpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
					{
					// not supported yet
					DrawTexturedTriangleGS32(&face, rc.video_buffer, rc.lpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
					{
					// test z distance again perspective transition gate
					if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
						{
						// default back to affine
						DrawTexturedTriangleGS32(&face, rc.video_buffer, rc.lpitch);
						} // end if
					else
						{
						// use perspective linear
						// not supported yet
						DrawTexturedTriangleGS32(&face, rc.video_buffer, rc.lpitch);
						} // end if

					} // end if

				 } // end if

			  } // end else

		   } // end if      
		else
		if ((_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_FLAT) || 
			(_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_CONSTANT) )
		   {
		   // draw as constant shaded
		   face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];
	       
		   // set the vertices
		   face.tvlist[0].x = (float)_poly_ptrs[poly]->tvlist[0].x;
		   face.tvlist[0].y = (float)_poly_ptrs[poly]->tvlist[0].y;
		   face.tvlist[0].z  = (float)_poly_ptrs[poly]->tvlist[0].z;

		   face.tvlist[1].x = (float)_poly_ptrs[poly]->tvlist[1].x;
		   face.tvlist[1].y = (float)_poly_ptrs[poly]->tvlist[1].y;
		   face.tvlist[1].z  = (float)_poly_ptrs[poly]->tvlist[1].z;

		   face.tvlist[2].x = (float)_poly_ptrs[poly]->tvlist[2].x;
		   face.tvlist[2].y = (float)_poly_ptrs[poly]->tvlist[2].y;
		   face.tvlist[2].z  = (float)_poly_ptrs[poly]->tvlist[2].z;

		   // draw the triangle with basic flat rasterizer

		   // test for transparent
		   if ((rc.attr & RENDER_ATTR_ALPHA) &&
				  ((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
			  {
			  DrawTriangleAlpha32(&face, rc.video_buffer, rc.lpitch,alpha);
			  } // end if
		   else
			  {
			  DrawTriangle32(&face, rc.video_buffer, rc.lpitch);
			  } // end if
	                          
		   } // end if                    
		else
		if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_GOURAUD)
		   {
			// {andre take advantage of the data structures later..}
			// set the vertices
			face.tvlist[0].x  = (float)_poly_ptrs[poly]->tvlist[0].x;
			face.tvlist[0].y  = (float)_poly_ptrs[poly]->tvlist[0].y;
			face.tvlist[0].z  = (float)_poly_ptrs[poly]->tvlist[0].z;
			face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];

			face.tvlist[1].x  = (float)_poly_ptrs[poly]->tvlist[1].x;
			face.tvlist[1].y  = (float)_poly_ptrs[poly]->tvlist[1].y;
			face.tvlist[1].z  = (float)_poly_ptrs[poly]->tvlist[1].z;
			face.lit_color[1] = _poly_ptrs[poly]->lit_color[1];

			face.tvlist[2].x  = (float)_poly_ptrs[poly]->tvlist[2].x;
			face.tvlist[2].y  = (float)_poly_ptrs[poly]->tvlist[2].y;
			face.tvlist[2].z  = (float)_poly_ptrs[poly]->tvlist[2].z;
			face.lit_color[2] = _poly_ptrs[poly]->lit_color[2];

			// draw the gouraud shaded triangle
			// test for transparent
			  if ((rc.attr & RENDER_ATTR_ALPHA) &&
				  ((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
			   {
			   DrawGouraudTriangleAlpha32(&face, rc.video_buffer, rc.lpitch,alpha);
			   } // end if
			else
			   {
			   DrawGouraudTriangle32(&face, rc.video_buffer, rc.lpitch);
			   } // end if

		   } // end if gouraud

		} // end for poly

	} // end if RENDER_ATTR_NOBUFFER

	else
	if (rc.attr & RENDER_ATTR_ZBUFFER) ////////////////////////////////////
	{
	// use the z buffer

	// we have is a list of polygons and it's time draw them
	for (int poly=0; poly < _num_polys; poly++)
		{
		// render this polygon if and only if it's not clipped, not culled,
		// active, and visible, note however the concecpt of "backface" is 
		// irrelevant in a wire frame engine though
		if (!(_poly_ptrs[poly]->state & POLY_STATE_ACTIVE) ||
			 (_poly_ptrs[poly]->state & POLY_STATE_CLIPPED ) ||
			 (_poly_ptrs[poly]->state & POLY_STATE_BACKFACE) )
		   continue; // move onto next poly

		// test for alpha override
		if (rc.alpha_override>= 0)
		   {
		   // set alpha to override value
		   alpha = rc.alpha_override;
		   }  // end if 
		else
			{
			// extract alpha (even if there isn't any)
			alpha = ((_poly_ptrs[poly]->color & 0xff000000) >> 24);
			} // end else

		// need to test for textured first, since a textured poly can either
		// be emissive, or flat shaded, hence we need to call different
		// rasterizers    
		if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_TEXTURE)
		   {
		   // set the vertices
		   face.tvlist[0].x = (float)_poly_ptrs[poly]->tvlist[0].x;
		   face.tvlist[0].y = (float)_poly_ptrs[poly]->tvlist[0].y;
		   face.tvlist[0].z  = (float)_poly_ptrs[poly]->tvlist[0].z;
		   face.tvlist[0].u0 = (float)_poly_ptrs[poly]->tvlist[0].u0;
		   face.tvlist[0].v0 = (float)_poly_ptrs[poly]->tvlist[0].v0;

		   face.tvlist[1].x = (float)_poly_ptrs[poly]->tvlist[1].x;
		   face.tvlist[1].y = (float)_poly_ptrs[poly]->tvlist[1].y;
		   face.tvlist[1].z  = (float)_poly_ptrs[poly]->tvlist[1].z;
		   face.tvlist[1].u0 = (float)_poly_ptrs[poly]->tvlist[1].u0;
		   face.tvlist[1].v0 = (float)_poly_ptrs[poly]->tvlist[1].v0;

		   face.tvlist[2].x = (float)_poly_ptrs[poly]->tvlist[2].x;
		   face.tvlist[2].y = (float)_poly_ptrs[poly]->tvlist[2].y;
		   face.tvlist[2].z  = (float)_poly_ptrs[poly]->tvlist[2].z;
		   face.tvlist[2].u0 = (float)_poly_ptrs[poly]->tvlist[2].u0;
		   face.tvlist[2].v0 = (float)_poly_ptrs[poly]->tvlist[2].v0;
	    
		   // test if this is a mipmapped polygon?
		   if (_poly_ptrs[poly]->attr & POLY_ATTR_MIPMAP)
			  {
			  // determine if mipmapping is desired at all globally
			  if (rc.attr & RENDER_ATTR_MIPMAP)
				 {
				 // determine mip level for this polygon

				 // first determine how many miplevels there are in mipchain for this polygon
				 int tmiplevels = logbase2ofx[((BmpImg* *)(_poly_ptrs[poly]->texture))[0]->Width()];

				 // now based on the requested linear miplevel fall off distance, cut
				 // the viewdistance into segments, determine what segment polygon is
				 // in and select mip level -- simple! later you might want something more
				 // robust, also note I only use a single vertex, you might want to find the average
				 // since for long walls perpendicular to view direction this might causing mip
				 // popping mid surface
				 int miplevel = (tmiplevels * _poly_ptrs[poly]->tvlist[0].z / rc.mip_dist);
	          
				 // clamp miplevel
				 if (miplevel > tmiplevels) miplevel = tmiplevels;

				 // based on miplevel select proper texture
				 face.texture = ((BmpImg* *)(_poly_ptrs[poly]->texture))[miplevel];

				 // now we must divide each texture coordinate by 2 per miplevel
				 for (int ts = 0; ts < miplevel; ts++)
					 {
					 face.tvlist[0].u0*=.5;
					 face.tvlist[0].v0*=.5;

					 face.tvlist[1].u0*=.5;
					 face.tvlist[1].v0*=.5;

					 face.tvlist[2].u0*=.5;
					 face.tvlist[2].v0*=.5;
					} // end for

				 } // end if mipmmaping enabled globally
			  else // mipmapping not selected globally
				 {
				 // in this case the polygon IS mipmapped, but the caller has requested NO
				 // mipmapping, so we will support this by selecting mip level 0 since the
				 // texture pointer is pointing to a mip chain regardless
				 face.texture = ((BmpImg* *)(_poly_ptrs[poly]->texture))[0];
	 
				 // note: texture coordinate manipulation is unneeded

				 } // end else

			  } // end if
		   else
			  {
			  // assign the texture without change
			  face.texture = _poly_ptrs[poly]->texture;
			  } // end if
	       
		   // is this a plain emissive texture?
		   if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_CONSTANT)
			  {
			  // draw the textured triangle as emissive
			  if ((rc.attr & RENDER_ATTR_ALPHA) &&
				  ((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
				 {
				 // alpha version
	             
				 // which texture mapper?
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
					DrawTexturedTriangleZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
					{
					// not supported yet!
					DrawTexturedTriangleZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
					{
					// not supported yet
					DrawTexturedTriangleZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
					{
					// test z distance again perspective transition gate
					if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
						{
						// default back to affine
						DrawTexturedTriangleZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
						} // end if
					else
						{
						// use perspective linear
						// not supported yet
						DrawTexturedTriangleZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
						} // end if

					 } // end if

				 } // end if
			  else
				 {
				 // non alpha
				 // which texture mapper?
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
					// use bilerp?
					if (rc.attr & RENDER_ATTR_BILERP)
					   DrawTexturedBilerpTriangleZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);               
					else
					   DrawTexturedTriangleZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);

					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
					{
					// not supported yet
//					Draw_Textured_TriangleZB2_16(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
					{
					// not supported yet
//					Draw_Textured_TriangleZB2_16(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
					{
					// test z distance again perspective transition gate
					if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
						{
						// default back to affine
						DrawTexturedTriangleZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
						} // end if
					else
						{
						// use perspective linear
						// not supported yet
						DrawTexturedTriangleZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
						} // end if

					} // end if

				 } // end if

			  } // end if
		   else
		   if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_FLAT)
			  {
			  // draw as flat shaded
			  face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];

			  // test for transparency
			  if ((rc.attr & RENDER_ATTR_ALPHA) &&
				  ((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
				 {
				 // alpha version

				 // which texture mapper?
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
					DrawTexturedTriangleFSZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
					{
					// not supported yet
					DrawTexturedTriangleFSZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
					{
					// not supported yet
					DrawTexturedTriangleFSZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
					{
					// test z distance again perspective transition gate
					if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
						{
						// default back to affine
						DrawTexturedTriangleFSZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
						} // end if
					else
						{
						// use perspective linear
						// not supported yet
						DrawTexturedTriangleFSZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
						} // end if

					 } // end if

				 } // end if
			  else
				 {
				 // non alpha
				 // which texture mapper?
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
					DrawTexturedTriangleFSZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
					{
					// not supported yet
					DrawTexturedTriangleFSZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
					{
					// not supported yet
					DrawTexturedTriangleFSZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
					{
					// test z distance again perspective transition gate
					if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
						{
						// default back to affine
						DrawTexturedTriangleFSZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
						} // end if
					else
						{
						// use perspective linear
						// not supported yet
						DrawTexturedTriangleFSZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
						} // end if

					 } // end if

				 } // end if

			  } // end else if
		  else
			 { // POLY_ATTR_SHADE_MODE_GOURAUD

			  // must be gouraud POLY_ATTR_SHADE_MODE_GOURAUD
			  face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];
			  face.lit_color[1] = _poly_ptrs[poly]->lit_color[1];
			  face.lit_color[2] = _poly_ptrs[poly]->lit_color[2];

			 // test for transparency
			 if ((rc.attr & RENDER_ATTR_ALPHA) &&
				  ((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
				 {
				 // alpha version

				 // which texture mapper?
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
//					Draw_Textured_TriangleGSZB_Alpha16(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
					{
					// not supported yet :)
//					Draw_Textured_TriangleGSZB_Alpha16(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
					{
					// not supported yet :)
//					Draw_Textured_TriangleGSZB_Alpha16(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
					{
					// test z distance again perspective transition gate
					if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
						{
						// default back to affine
//						Draw_Textured_TriangleGSZB_Alpha16(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
						} // end if
					else
						{
						// use perspective linear
						// not supported yet :)
//						Draw_Textured_TriangleGSZB_Alpha16(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
						} // end if

					 } // end if

				 } // end if
			  else
				 {
				 // non alpha
				 // which texture mapper?
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
					DrawTexturedTriangleGSZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
					{
					// not supported yet :)
					DrawTexturedTriangleGSZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
					{
					// not supported yet :)
					DrawTexturedTriangleGSZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
					{
					// test z distance again perspective transition gate
					if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
						{
						// default back to affine
						DrawTexturedTriangleGSZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
						} // end if
					else
						{
						// use perspective linear
						// not supported yet :)
						DrawTexturedTriangleGSZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
						} // end if

					 } // end if

				 } // end if

			 } // end else

		   } // end if      
		else
		if ((_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_FLAT) || 
			(_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_CONSTANT) )
		   {
		   // draw as constant shaded
		   face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];
	       
		   // set the vertices
		   face.tvlist[0].x = (float)_poly_ptrs[poly]->tvlist[0].x;
		   face.tvlist[0].y = (float)_poly_ptrs[poly]->tvlist[0].y;
		   face.tvlist[0].z = (float)_poly_ptrs[poly]->tvlist[0].z;

		   face.tvlist[1].x = (float)_poly_ptrs[poly]->tvlist[1].x;
		   face.tvlist[1].y = (float)_poly_ptrs[poly]->tvlist[1].y;
		   face.tvlist[1].z = (float)_poly_ptrs[poly]->tvlist[1].z;

		   face.tvlist[2].x = (float)_poly_ptrs[poly]->tvlist[2].x;
		   face.tvlist[2].y = (float)_poly_ptrs[poly]->tvlist[2].y;
		   face.tvlist[2].z = (float)_poly_ptrs[poly]->tvlist[2].z;

		   // draw the triangle with basic flat rasterizer
		   // test for transparency
		   if ((rc.attr & RENDER_ATTR_ALPHA) &&
				  ((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
			  {
			  // alpha version
			  DrawTriangleZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch,alpha);
			  } // end if
		   else
			  {
			  // non alpha version
			  DrawTriangleZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
			  }  // end if

		   } // end if
		else
		if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_GOURAUD)
		   {
			// {andre take advantage of the data structures later..}
			// set the vertices
			face.tvlist[0].x  = (float)_poly_ptrs[poly]->tvlist[0].x;
			face.tvlist[0].y  = (float)_poly_ptrs[poly]->tvlist[0].y;
			face.tvlist[0].z  = (float)_poly_ptrs[poly]->tvlist[0].z;
			face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];

			face.tvlist[1].x  = (float)_poly_ptrs[poly]->tvlist[1].x;
			face.tvlist[1].y  = (float)_poly_ptrs[poly]->tvlist[1].y;
			face.tvlist[1].z  = (float)_poly_ptrs[poly]->tvlist[1].z;
			face.lit_color[1] = _poly_ptrs[poly]->lit_color[1];

			face.tvlist[2].x  = (float)_poly_ptrs[poly]->tvlist[2].x;
			face.tvlist[2].y  = (float)_poly_ptrs[poly]->tvlist[2].y;
			face.tvlist[2].z  = (float)_poly_ptrs[poly]->tvlist[2].z;
			face.lit_color[2] = _poly_ptrs[poly]->lit_color[2];

			// draw the gouraud shaded triangle
			// test for transparency
			if ((rc.attr & RENDER_ATTR_ALPHA) &&
				  ((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
			   {
			   // alpha version
			   DrawGouraudTriangleZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch,alpha);
			   } // end if
			else
			   { 
			   // non alpha
			   DrawGouraudTriangleZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
			   } // end if

		   } // end if gouraud

		} // end for poly

	} // end if RENDER_ATTR_ZBUFFER
	else
	if (rc.attr & RENDER_ATTR_INVZBUFFER) ////////////////////////////////////
	{
	// use the inverse z buffer

	// we have is a list of polygons and it's time draw them
	for (int poly=0; poly < _num_polys; poly++)
		{
		// render this polygon if and only if it's not clipped, not culled,
		// active, and visible, note however the concecpt of "backface" is 
		// irrelevant in a wire frame engine though
		if (!(_poly_ptrs[poly]->state & POLY_STATE_ACTIVE) ||
			 (_poly_ptrs[poly]->state & POLY_STATE_CLIPPED ) ||
			 (_poly_ptrs[poly]->state & POLY_STATE_BACKFACE) )
		   continue; // move onto next poly

		// test for alpha override
		if (rc.alpha_override>= 0)
		   {
		   // set alpha to override value
		   alpha = rc.alpha_override;
		   }  // end if 
		else
		   {
		   // extract alpha (even if there isn't any)
		   alpha = ((_poly_ptrs[poly]->color & 0xff000000) >> 24);
		   } // end else

		// need to test for textured first, since a textured poly can either
		// be emissive, or flat shaded, hence we need to call different
		// rasterizers    
		if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_TEXTURE)
		   {
		   // set the vertices
		   face.tvlist[0].x = (float)_poly_ptrs[poly]->tvlist[0].x;
		   face.tvlist[0].y = (float)_poly_ptrs[poly]->tvlist[0].y;
		   face.tvlist[0].z  = (float)_poly_ptrs[poly]->tvlist[0].z;
		   face.tvlist[0].u0 = (float)_poly_ptrs[poly]->tvlist[0].u0;
		   face.tvlist[0].v0 = (float)_poly_ptrs[poly]->tvlist[0].v0;

		   face.tvlist[1].x = (float)_poly_ptrs[poly]->tvlist[1].x;
		   face.tvlist[1].y = (float)_poly_ptrs[poly]->tvlist[1].y;
		   face.tvlist[1].z  = (float)_poly_ptrs[poly]->tvlist[1].z;
		   face.tvlist[1].u0 = (float)_poly_ptrs[poly]->tvlist[1].u0;
		   face.tvlist[1].v0 = (float)_poly_ptrs[poly]->tvlist[1].v0;

		   face.tvlist[2].x = (float)_poly_ptrs[poly]->tvlist[2].x;
		   face.tvlist[2].y = (float)_poly_ptrs[poly]->tvlist[2].y;
		   face.tvlist[2].z  = (float)_poly_ptrs[poly]->tvlist[2].z;
		   face.tvlist[2].u0 = (float)_poly_ptrs[poly]->tvlist[2].u0;
		   face.tvlist[2].v0 = (float)_poly_ptrs[poly]->tvlist[2].v0;
	    
	    
		   // test if this is a mipmapped polygon?
		   if (_poly_ptrs[poly]->attr & POLY_ATTR_MIPMAP)
			  {
			  // determine if mipmapping is desired at all globally
			  if (rc.attr & RENDER_ATTR_MIPMAP)
				 {
				 // determine mip level for this polygon

				 // first determine how many miplevels there are in mipchain for this polygon
				 int tmiplevels = logbase2ofx[((BmpImg* *)(_poly_ptrs[poly]->texture))[0]->Width()];

				 // now based on the requested linear miplevel fall off distance, cut
				 // the viewdistance into segments, determine what segment polygon is
				 // in and select mip level -- simple! later you might want something more
				 // robust, also note I only use a single vertex, you might want to find the average
				 // since for long walls perpendicular to view direction this might causing mip
				 // popping mid surface
				 int miplevel = (tmiplevels * _poly_ptrs[poly]->tvlist[0].z / rc.mip_dist);
	          
				 // clamp miplevel
				 if (miplevel > tmiplevels) miplevel = tmiplevels;

				 // based on miplevel select proper texture
				 face.texture = ((BmpImg* *)(_poly_ptrs[poly]->texture))[miplevel];

				 // now we must divide each texture coordinate by 2 per miplevel
				 for (int ts = 0; ts < miplevel; ts++)
					 {
					 face.tvlist[0].u0*=.5;
					 face.tvlist[0].v0*=.5;

					 face.tvlist[1].u0*=.5;
					 face.tvlist[1].v0*=.5;

					 face.tvlist[2].u0*=.5;
					 face.tvlist[2].v0*=.5;
					} // end for

				 } // end if mipmmaping enabled globally
			  else // mipmapping not selected globally
				 {
				 // in this case the polygon IS mipmapped, but the caller has requested NO
				 // mipmapping, so we will support this by selecting mip level 0 since the
				 // texture pointer is pointing to a mip chain regardless
				 face.texture = ((BmpImg* *)(_poly_ptrs[poly]->texture))[0];
	 
				 // note: texture coordinate manipulation is unneeded

				 } // end else

			  } // end if
		   else
			  {
			  // assign the texture without change
			  face.texture = _poly_ptrs[poly]->texture;
			  } // end if
	              
		   // is this a plain emissive texture?
		   if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_CONSTANT)
			  {
			  // draw the textured triangle as emissive
			  if ((rc.attr & RENDER_ATTR_ALPHA) &&
				  ((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
				 {
				 // alpha version
	             
				 // which texture mapper?
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
					DrawTexturedTriangleINVZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
					{
//					Draw_Textured_Perspective_Triangle_INVZB_Alpha16(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
					{
//					Draw_Textured_PerspectiveLP_Triangle_INVZB_Alpha16(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
					{
					// test z distance again perspective transition gate
					if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
						{
						// default back to affine
						DrawTexturedTriangleINVZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
						} // end if
					else
						{
						// use perspective linear
//						Draw_Textured_PerspectiveLP_Triangle_INVZB_Alpha16(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
						} // end if

					 } // end if

				 } // end if
			  else
				 {
				 // non alpha
				 // which texture mapper?
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
					// use bilerp?
					if (rc.attr & RENDER_ATTR_BILERP)
						DrawTexturedBilerpTriangleINVZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);             
					else
						DrawTexturedTriangleINVZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);

					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
					{
//					Draw_Textured_Perspective_Triangle_INVZB_16(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
					{
//					Draw_Textured_PerspectiveLP_Triangle_INVZB_16(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
					{
					// test z distance again perspective transition gate
					if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
						{
						// default back to affine
						DrawTexturedTriangleINVZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
						} // end if
					else
						{
						// use perspective linear
//						Draw_Textured_PerspectiveLP_Triangle_INVZB_16(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
						} // end if

					} // end if

				 } // end if

			  } // end if
		   else
		   if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_FLAT)
			  {
			  // draw as flat shaded
			  face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];

			  // test for transparency
			  if ((rc.attr & RENDER_ATTR_ALPHA) &&
				  ((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
				 {
				 // alpha version

				 // which texture mapper?
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
					DrawTexturedTriangleFSINVZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
					{
					DrawTexturedTriangleFSINVZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
					{
					DrawTexturedTriangleFSINVZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
					{
					// test z distance again perspective transition gate
					if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
						{
						// default back to affine
						DrawTexturedTriangleFSINVZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
						} // end if
					else
						{
						// use perspective linear
//						Draw_Textured_PerspectiveLP_Triangle_FSINVZB_Alpha16(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
						} // end if

					 } // end if

				 } // end if
			  else
				 {
				 // non alpha
				 // which texture mapper?
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
					DrawTexturedTriangleFSINVZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
					{
					DrawTexturedPerspectiveTriangleFSINVZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
					{
					DrawTexturedPerspectiveLPTriangleFSINVZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
					{
					// test z distance again perspective transition gate
					if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
						{
						// default back to affine
						DrawTexturedTriangleFSINVZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
						} // end if
					else
						{
						// use perspective linear
						DrawTexturedPerspectiveLPTriangleFSINVZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
						} // end if

					 } // end if

				 } // end if

			  } // end else if
		  else
			 { // POLY_ATTR_SHADE_MODE_GOURAUD

			  // must be gouraud POLY_ATTR_SHADE_MODE_GOURAUD
			  face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];
			  face.lit_color[1] = _poly_ptrs[poly]->lit_color[1];
			  face.lit_color[2] = _poly_ptrs[poly]->lit_color[2];

			 // test for transparency
			 if ((rc.attr & RENDER_ATTR_ALPHA) &&
				  ((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
				 {
				 // alpha version

				 // which texture mapper?
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
					DrawTexturedTriangleGSINVZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
					{
					// not supported yet :)
					DrawTexturedTriangleGSINVZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
					{
					// not supported yet :)
					DrawTexturedTriangleGSINVZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
					{
					// test z distance again perspective transition gate
					if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
						{
						// default back to affine
						DrawTexturedTriangleGSINVZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
						} // end if
					else
						{
						// use perspective linear
						// not supported yet :)
						DrawTexturedTriangleGSINVZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
						} // end if

					 } // end if

				 } // end if
			  else
				 {
				 // non alpha
				 // which texture mapper?
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
					DrawTexturedTriangleGSINVZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
					{
					// not supported yet :)
					DrawTexturedTriangleGSINVZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
					{
					// not supported yet :)
					DrawTexturedTriangleGSINVZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
					} // end if
				 else
				 if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
					{
					// test z distance again perspective transition gate
					if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
						{
						// default back to affine
 						DrawTexturedTriangleFSINVZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
						} // end if
					else
						{
						// use perspective linear
						// not supported yet :)
						DrawTexturedTriangleGSINVZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
						} // end if

					 } // end if

				 } // end if

			 } // end else

		   } // end if      
		else
		if ((_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_FLAT) || 
			(_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_CONSTANT) )
		   {
		   // draw as constant shaded
		   face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];
	       
		   // set the vertices
		   face.tvlist[0].x = (float)_poly_ptrs[poly]->tvlist[0].x;
		   face.tvlist[0].y = (float)_poly_ptrs[poly]->tvlist[0].y;
		   face.tvlist[0].z = (float)_poly_ptrs[poly]->tvlist[0].z;

		   face.tvlist[1].x = (float)_poly_ptrs[poly]->tvlist[1].x;
		   face.tvlist[1].y = (float)_poly_ptrs[poly]->tvlist[1].y;
		   face.tvlist[1].z = (float)_poly_ptrs[poly]->tvlist[1].z;

		   face.tvlist[2].x = (float)_poly_ptrs[poly]->tvlist[2].x;
		   face.tvlist[2].y = (float)_poly_ptrs[poly]->tvlist[2].y;
		   face.tvlist[2].z = (float)_poly_ptrs[poly]->tvlist[2].z;

		   // draw the triangle with basic flat rasterizer
		   // test for transparency
		   if ((rc.attr & RENDER_ATTR_ALPHA) &&
				  ((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
			  {
			  // alpha version
			  DrawTriangleINVZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch,alpha);
			  } // end if
		   else
			  {
			  // non alpha version
			  DrawTriangleINVZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
			  }  // end if

		   } // end if
		else
		if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_GOURAUD)
		   {
			// {andre take advantage of the data structures later..}
			// set the vertices
			face.tvlist[0].x  = (float)_poly_ptrs[poly]->tvlist[0].x;
			face.tvlist[0].y  = (float)_poly_ptrs[poly]->tvlist[0].y;
			face.tvlist[0].z  = (float)_poly_ptrs[poly]->tvlist[0].z;
			face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];

			face.tvlist[1].x  = (float)_poly_ptrs[poly]->tvlist[1].x;
			face.tvlist[1].y  = (float)_poly_ptrs[poly]->tvlist[1].y;
			face.tvlist[1].z  = (float)_poly_ptrs[poly]->tvlist[1].z;
			face.lit_color[1] = _poly_ptrs[poly]->lit_color[1];

			face.tvlist[2].x  = (float)_poly_ptrs[poly]->tvlist[2].x;
			face.tvlist[2].y  = (float)_poly_ptrs[poly]->tvlist[2].y;
			face.tvlist[2].z  = (float)_poly_ptrs[poly]->tvlist[2].z;
			face.lit_color[2] = _poly_ptrs[poly]->lit_color[2];

			// draw the gouraud shaded triangle
			// test for transparency
			if ((rc.attr & RENDER_ATTR_ALPHA) &&
				  ((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
			   {
			   // alpha version
			   DrawGouraudTriangleINVZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch,alpha);
			   } // end if
			else
			   { 
			   // non alpha
			   DrawGouraudTriangleINVZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
			   } // end if

		   } // end if gouraud

		} // end for poly

	} // end if RENDER_ATTR_INVZBUFFER
else
if (rc.attr & RENDER_ATTR_WRITETHRUZBUFFER) ////////////////////////////////////
{
	// use the write thru z buffer

	// we have is a list of polygons and it's time draw them
	for (int poly=0; poly < _num_polys; poly++)
	{
		// render this polygon if and only if it's not clipped, not culled,
		// active, and visible, note however the concecpt of "backface" is 
		// irrelevant in a wire frame engine though
		if (!(_poly_ptrs[poly]->state & POLY_STATE_ACTIVE) ||
			(_poly_ptrs[poly]->state & POLY_STATE_CLIPPED ) ||
			(_poly_ptrs[poly]->state & POLY_STATE_BACKFACE) )
			continue; // move onto next poly

		// test for alpha override
		if (rc.alpha_override>= 0)
		{
			// set alpha to override value
			alpha = rc.alpha_override;
		}  // end if 
		else
		{
			// extract alpha (even if there isn't any)
			alpha = ((_poly_ptrs[poly]->color & 0xff000000) >> 24);
		} // end else

		// need to test for textured first, since a textured poly can either
		// be emissive, or flat shaded, hence we need to call different
		// rasterizers    
		if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_TEXTURE)
		{
			// set the vertices
			face.tvlist[0].x = (float)_poly_ptrs[poly]->tvlist[0].x;
			face.tvlist[0].y = (float)_poly_ptrs[poly]->tvlist[0].y;
			face.tvlist[0].z  = (float)_poly_ptrs[poly]->tvlist[0].z;
			face.tvlist[0].u0 = (float)_poly_ptrs[poly]->tvlist[0].u0;
			face.tvlist[0].v0 = (float)_poly_ptrs[poly]->tvlist[0].v0;

			face.tvlist[1].x = (float)_poly_ptrs[poly]->tvlist[1].x;
			face.tvlist[1].y = (float)_poly_ptrs[poly]->tvlist[1].y;
			face.tvlist[1].z  = (float)_poly_ptrs[poly]->tvlist[1].z;
			face.tvlist[1].u0 = (float)_poly_ptrs[poly]->tvlist[1].u0;
			face.tvlist[1].v0 = (float)_poly_ptrs[poly]->tvlist[1].v0;

			face.tvlist[2].x = (float)_poly_ptrs[poly]->tvlist[2].x;
			face.tvlist[2].y = (float)_poly_ptrs[poly]->tvlist[2].y;
			face.tvlist[2].z  = (float)_poly_ptrs[poly]->tvlist[2].z;
			face.tvlist[2].u0 = (float)_poly_ptrs[poly]->tvlist[2].u0;
			face.tvlist[2].v0 = (float)_poly_ptrs[poly]->tvlist[2].v0;

			// test if this is a mipmapped polygon?
			if (_poly_ptrs[poly]->attr & POLY_ATTR_MIPMAP)
			{
				// determine if mipmapping is desired at all globally
				if (rc.attr & RENDER_ATTR_MIPMAP)
				{
					// determine mip level for this polygon

					// first determine how many miplevels there are in mipchain for this polygon
					int tmiplevels = logbase2ofx[((BmpImg**)(_poly_ptrs[poly]->texture))[0]->Width()];

					// now based on the requested linear miplevel fall off distance, cut
					// the viewdistance into segments, determine what segment polygon is
					// in and select mip level -- simple! later you might want something more
					// robust, also note I only use a single vertex, you might want to find the average
					// since for long walls perpendicular to view direction this might causing mip
					// popping mid surface
					int miplevel = (tmiplevels * _poly_ptrs[poly]->tvlist[0].z / rc.mip_dist);

					// clamp miplevel
					if (miplevel > tmiplevels) miplevel = tmiplevels;

					// based on miplevel select proper texture
					face.texture = ((BmpImg**)(_poly_ptrs[poly]->texture))[miplevel];

					// now we must divide each texture coordinate by 2 per miplevel
					for (int ts = 0; ts < miplevel; ts++)
					{
						face.tvlist[0].u0*=.5;
						face.tvlist[0].v0*=.5;

						face.tvlist[1].u0*=.5;
						face.tvlist[1].v0*=.5;

						face.tvlist[2].u0*=.5;
						face.tvlist[2].v0*=.5;
					} // end for

				} // end if mipmmaping enabled globally
				else // mipmapping not selected globally
				{
					// in this case the polygon IS mipmapped, but the caller has requested NO
					// mipmapping, so we will support this by selecting mip level 0 since the
					// texture pointer is pointing to a mip chain regardless
					face.texture = ((BmpImg**)(_poly_ptrs[poly]->texture))[0];

					// note: texture coordinate manipulation is unneeded

				} // end else

			} // end if
			else
			{
				// assign the texture without change
				face.texture = _poly_ptrs[poly]->texture;
			} // end if

			// is this a plain emissive texture?
			if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_CONSTANT)
			{
				// draw the textured triangle as emissive
				if ((rc.attr & RENDER_ATTR_ALPHA) &&
					((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
				{
					// alpha version

					// which texture mapper?
					if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
						DrawTexturedTriangleZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
					} // end if
					else
						if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
						{
							// not supported yet!
							DrawTexturedTriangleZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
						} // end if
						else
							if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
							{
								// not supported yet
								DrawTexturedTriangleZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
							} // end if
							else
								if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
								{
									// test z distance again perspective transition gate
									if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
									{
										// default back to affine
										DrawTexturedTriangleZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
									} // end if
									else
									{
										// use perspective linear
										// not supported yet
										DrawTexturedTriangleZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
									} // end if

								} // end if

				} // end if
				else
				{
					// non alpha
					// which texture mapper?
					if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
					{
						// use bilerp?
						if (rc.attr & RENDER_ATTR_BILERP)
							DrawTexturedBilerpTriangleZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);               
						else
							DrawTexturedTriangleWTZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);

					} // end if
					else
						if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
						{
							// not supported yet
							DrawTexturedTriangleZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
						} // end if
						else
							if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
							{
								// not supported yet
								DrawTexturedTriangleZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
							} // end if
							else
								if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
								{
									// test z distance again perspective transition gate
									if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
									{
										// default back to affine
										DrawTexturedTriangleZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
									} // end if
									else
									{
										// use perspective linear
										// not supported yet
										DrawTexturedTriangleZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
									} // end if

								} // end if

				} // end if

			} // end if
			else
				if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_FLAT)
				{
					// draw as flat shaded
					face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];

					// test for transparency
					if ((rc.attr & RENDER_ATTR_ALPHA) &&
						((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
					{
						// alpha version

						// which texture mapper?
						if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
						{
							DrawTexturedTriangleFSZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
						} // end if
						else
							if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
							{
								// not supported yet
								DrawTexturedTriangleFSZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
							} // end if
							else
								if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
								{
									// not supported yet
									DrawTexturedTriangleFSZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
								} // end if
								else
									if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
									{
										// test z distance again perspective transition gate
										if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
										{
											// default back to affine
											DrawTexturedTriangleFSZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
										} // end if
										else
										{
											// use perspective linear
											// not supported yet
											DrawTexturedTriangleFSZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
										} // end if

									} // end if

					} // end if
					else
					{
						// non alpha
						// which texture mapper?
						if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
						{
							DrawTexturedTriangleFSWTZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
						} // end if
						else
							if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
							{
								// not supported yet
								DrawTexturedTriangleFSZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
							} // end if
							else
								if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
								{
									// not supported yet
									DrawTexturedTriangleFSZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
								} // end if
								else
									if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
									{
										// test z distance again perspective transition gate
										if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
										{
											// default back to affine
											DrawTexturedTriangleFSZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
										} // end if
										else
										{
											// use perspective linear
											// not supported yet
											DrawTexturedTriangleFSZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
										} // end if

									} // end if

					} // end if

				} // end else if
				else
				{ // POLY_ATTR_SHADE_MODE_GOURAUD

					// must be gouraud POLY_ATTR_SHADE_MODE_GOURAUD
					face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];
					face.lit_color[1] = _poly_ptrs[poly]->lit_color[1];
					face.lit_color[2] = _poly_ptrs[poly]->lit_color[2];

					// test for transparency
					if ((rc.attr & RENDER_ATTR_ALPHA) &&
						((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
					{
						// alpha version

						// which texture mapper?
						if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
						{
							DrawTexturedTriangleGSZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
						} // end if
						else
							if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
							{
								// not supported yet :)
								DrawTexturedTriangleGSZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
							} // end if
							else
								if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
								{
									// not supported yet :)
									DrawTexturedTriangleGSZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
								} // end if
								else
									if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
									{
										// test z distance again perspective transition gate
										if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
										{
											// default back to affine
											DrawTexturedTriangleGSZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
										} // end if
										else
										{
											// use perspective linear
											// not supported yet :)
											DrawTexturedTriangleGSZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch, alpha);
										} // end if

									} // end if

					} // end if
					else
					{
						// non alpha
						// which texture mapper?
						if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
						{
							DrawTexturedTriangleGSZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
						} // end if
						else
							if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
							{
								// not supported yet :)
								DrawTexturedTriangleGSZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
							} // end if
							else
								if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
								{
									// not supported yet :)
									DrawTexturedTriangleGSZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
								} // end if
								else
									if (rc.attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
									{
										// test z distance again perspective transition gate
										if (_poly_ptrs[poly]->tvlist[0].z > rc. texture_dist)
										{
											// default back to affine
											DrawTexturedTriangleGSZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
										} // end if
										else
										{
											// use perspective linear
											// not supported yet :)
											DrawTexturedTriangleGSZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
										} // end if

									} // end if

					} // end if

				} // end else

		} // end if      
		else
			if ((_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_FLAT) || 
				(_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_CONSTANT) )
			{
				// draw as constant shaded
				face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];

				// set the vertices
				face.tvlist[0].x = (float)_poly_ptrs[poly]->tvlist[0].x;
				face.tvlist[0].y = (float)_poly_ptrs[poly]->tvlist[0].y;
				face.tvlist[0].z = (float)_poly_ptrs[poly]->tvlist[0].z;

				face.tvlist[1].x = (float)_poly_ptrs[poly]->tvlist[1].x;
				face.tvlist[1].y = (float)_poly_ptrs[poly]->tvlist[1].y;
				face.tvlist[1].z = (float)_poly_ptrs[poly]->tvlist[1].z;

				face.tvlist[2].x = (float)_poly_ptrs[poly]->tvlist[2].x;
				face.tvlist[2].y = (float)_poly_ptrs[poly]->tvlist[2].y;
				face.tvlist[2].z = (float)_poly_ptrs[poly]->tvlist[2].z;

				// draw the triangle with basic flat rasterizer
				// test for transparency
				if ((rc.attr & RENDER_ATTR_ALPHA) &&
					((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
				{
					// alpha version
					DrawTriangleZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch,alpha);
				} // end if
				else
				{
					// non alpha version
					DrawTriangleZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
				}  // end if

			} // end if
			else
				if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_GOURAUD)
				{
					// {andre take advantage of the data structures later..}
					// set the vertices
					face.tvlist[0].x  = (float)_poly_ptrs[poly]->tvlist[0].x;
					face.tvlist[0].y  = (float)_poly_ptrs[poly]->tvlist[0].y;
					face.tvlist[0].z  = (float)_poly_ptrs[poly]->tvlist[0].z;
					face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];

					face.tvlist[1].x  = (float)_poly_ptrs[poly]->tvlist[1].x;
					face.tvlist[1].y  = (float)_poly_ptrs[poly]->tvlist[1].y;
					face.tvlist[1].z  = (float)_poly_ptrs[poly]->tvlist[1].z;
					face.lit_color[1] = _poly_ptrs[poly]->lit_color[1];

					face.tvlist[2].x  = (float)_poly_ptrs[poly]->tvlist[2].x;
					face.tvlist[2].y  = (float)_poly_ptrs[poly]->tvlist[2].y;
					face.tvlist[2].z  = (float)_poly_ptrs[poly]->tvlist[2].z;
					face.lit_color[2] = _poly_ptrs[poly]->lit_color[2];

					// draw the gouraud shaded triangle
					// test for transparency
					if ((rc.attr & RENDER_ATTR_ALPHA) &&
						((_poly_ptrs[poly]->attr & POLY_ATTR_TRANSPARENT) || rc.alpha_override>=0) )
					{
						// alpha version
						DrawGouraudTriangleZBAlpha32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch,alpha);
					} // end if
					else
					{ 
						// non alpha
						DrawGouraudTriangleWTZB32(&face, rc.video_buffer, rc.lpitch,rc.zbuffer,rc.zpitch);
					} // end if

				} // end if gouraud

	} // end for poly

} // end if RENDER_ATTR_WRITETHRUZBUFFER
}

void RenderList::DrawHybridTexturedSolidINVZB32(unsigned char* video_buffer, int lpitch,
												unsigned char* zbuffer, int zpitch, float dist1, float dist2)
{
	// 16-bit version
	// this function "executes" the render list or in other words
	// draws all the faces in the list, the function will call the 
	// proper rasterizer based on the lighting model of the polygons
	// testing function only for demo, place maxperspective distance
	// into dist1, and max linear into dist2, after dist2 everything
	// will be affine


	PolygonF face; // temp face used to render polygon

	// at this point, all we have is a list of polygons and it's time
	// to draw them
	for (int poly=0; poly < _num_polys; poly++)
	{
		// render this polygon if and only if it's not clipped, not culled,
		// active, and visible, note however the concecpt of "backface" is 
		// irrelevant in a wire frame engine though
		if (!(_poly_ptrs[poly]->state & POLY_STATE_ACTIVE) ||
			(_poly_ptrs[poly]->state & POLY_STATE_CLIPPED ) ||
			(_poly_ptrs[poly]->state & POLY_STATE_BACKFACE) )
			continue; // move onto next poly

		// need to test for textured first, since a textured poly can either
		// be emissive, or flat shaded, hence we need to call different
		// rasterizers    
		if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_TEXTURE)
		{
			// set the vertices
			face.tvlist[0].x = (float)_poly_ptrs[poly]->tvlist[0].x;
			face.tvlist[0].y = (float)_poly_ptrs[poly]->tvlist[0].y;
			face.tvlist[0].z  = (float)_poly_ptrs[poly]->tvlist[0].z;
			face.tvlist[0].u0 = (float)_poly_ptrs[poly]->tvlist[0].u0;
			face.tvlist[0].v0 = (float)_poly_ptrs[poly]->tvlist[0].v0;

			face.tvlist[1].x = (float)_poly_ptrs[poly]->tvlist[1].x;
			face.tvlist[1].y = (float)_poly_ptrs[poly]->tvlist[1].y;
			face.tvlist[1].z  = (float)_poly_ptrs[poly]->tvlist[1].z;
			face.tvlist[1].u0 = (float)_poly_ptrs[poly]->tvlist[1].u0;
			face.tvlist[1].v0 = (float)_poly_ptrs[poly]->tvlist[1].v0;

			face.tvlist[2].x = (float)_poly_ptrs[poly]->tvlist[2].x;
			face.tvlist[2].y = (float)_poly_ptrs[poly]->tvlist[2].y;
			face.tvlist[2].z  = (float)_poly_ptrs[poly]->tvlist[2].z;
			face.tvlist[2].u0 = (float)_poly_ptrs[poly]->tvlist[2].u0;
			face.tvlist[2].v0 = (float)_poly_ptrs[poly]->tvlist[2].v0;

			// test if this is a mipmapped polygon?
			if (_poly_ptrs[poly]->attr & POLY_ATTR_MIPMAP)
			{
				// determine mip level for this polygon
				int miplevel = (6*_poly_ptrs[poly]->tvlist[0].z/2000);

				if (miplevel > 6) 
					miplevel = 6;

				face.texture = ((BmpImg**)(_poly_ptrs[poly]->texture))[miplevel];

				for (int ts = 0; ts < miplevel; ts++)
				{
					face.tvlist[0].u0*=.5;
					face.tvlist[0].v0*=.5;

					face.tvlist[1].u0*=.5;
					face.tvlist[1].v0*=.5;

					face.tvlist[2].u0*=.5;
					face.tvlist[2].v0*=.5;
				} // end for

			} // end if
			else
			{
				// assign the texture without change
				face.texture = _poly_ptrs[poly]->texture;
			} // end if

			float avg_z = 0.33*(_poly_ptrs[poly]->tvlist[0].z + 
				                _poly_ptrs[poly]->tvlist[1].z + 
								_poly_ptrs[poly]->tvlist[2].z );

			// is this a plain emissive texture?
			if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_CONSTANT)
			{
				// draw the textured triangle as emissive
				if (avg_z < dist1)
					DrawTexturedPerspectiveTriangleINVZB32(&face, video_buffer, lpitch,zbuffer,zpitch);
				else if (avg_z > dist1 && avg_z < dist2)
					DrawTexturedPerspectiveLPTriangleINVZB32(&face, video_buffer, lpitch,zbuffer,zpitch);
				else
					DrawTexturedTriangleINVZB32(&face, video_buffer, lpitch,zbuffer,zpitch);

			} // end if
			else if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_FLAT)
			{
				// draw as flat shaded
				face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];

				if (avg_z < dist1)
					DrawTexturedPerspectiveTriangleFSINVZB32(&face, video_buffer, lpitch,zbuffer,zpitch);
				else if (avg_z > dist1 && avg_z < dist2)
					DrawTexturedPerspectiveLPTriangleFSINVZB32(&face, video_buffer, lpitch,zbuffer,zpitch);
				else
					DrawTexturedTriangleFSINVZB32(&face, video_buffer, lpitch,zbuffer,zpitch);

			} // end else if
			else
			{
				// must be gouraud POLY_ATTR_SHADE_MODE_GOURAUD
				face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];
				face.lit_color[1] = _poly_ptrs[poly]->lit_color[1];
				face.lit_color[2] = _poly_ptrs[poly]->lit_color[2];

				DrawTexturedTriangleGSINVZB32(&face, video_buffer, lpitch,zbuffer,zpitch);
			} // end else

		} // end if      
		else if ((_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_FLAT) || 
			     (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_CONSTANT) )
		{
			// draw as constant shaded
			face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];

			// set the vertices
			face.tvlist[0].x = (float)_poly_ptrs[poly]->tvlist[0].x;
			face.tvlist[0].y = (float)_poly_ptrs[poly]->tvlist[0].y;
			face.tvlist[0].z  = (float)_poly_ptrs[poly]->tvlist[0].z;

			face.tvlist[1].x = (float)_poly_ptrs[poly]->tvlist[1].x;
			face.tvlist[1].y = (float)_poly_ptrs[poly]->tvlist[1].y;
			face.tvlist[1].z  = (float)_poly_ptrs[poly]->tvlist[1].z;

			face.tvlist[2].x = (float)_poly_ptrs[poly]->tvlist[2].x;
			face.tvlist[2].y = (float)_poly_ptrs[poly]->tvlist[2].y;
			face.tvlist[2].z  = (float)_poly_ptrs[poly]->tvlist[2].z;

			// draw the triangle with basic flat rasterizer
			DrawTriangleINVZB32(&face, video_buffer, lpitch, zbuffer, zpitch);

		} // end if
		else if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_GOURAUD)
		{
			// {andre take advantage of the data structures later..}
			// set the vertices
			face.tvlist[0].x  = (float)_poly_ptrs[poly]->tvlist[0].x;
			face.tvlist[0].y  = (float)_poly_ptrs[poly]->tvlist[0].y;
			face.tvlist[0].z  = (float)_poly_ptrs[poly]->tvlist[0].z;
			face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];

			face.tvlist[1].x  = (float)_poly_ptrs[poly]->tvlist[1].x;
			face.tvlist[1].y  = (float)_poly_ptrs[poly]->tvlist[1].y;
			face.tvlist[1].z  = (float)_poly_ptrs[poly]->tvlist[1].z;
			face.lit_color[1] = _poly_ptrs[poly]->lit_color[1];

			face.tvlist[2].x  = (float)_poly_ptrs[poly]->tvlist[2].x;
			face.tvlist[2].y  = (float)_poly_ptrs[poly]->tvlist[2].y;
			face.tvlist[2].z  = (float)_poly_ptrs[poly]->tvlist[2].z;
			face.lit_color[2] = _poly_ptrs[poly]->lit_color[2];

			// draw the gouraud shaded triangle
			DrawGouraudTriangleINVZB32(&face, video_buffer, lpitch,zbuffer,zpitch);
		} // end if gouraud

	} // end for poly
}

void RenderList::LightWorld32(const Camera& cam)
{
	// 32-bit version of function
	// function lights the entire rendering list based on the sent lights and camera. the function supports
	// constant/pure shading (emmisive), flat shading with ambient, infinite, point lights, and spot lights
	// note that this lighting function is rather brute force and simply follows the math, however
	// there are some clever integer operations that are used in scale 256 rather than going to floating
	// point, but why? floating point and ints are the same speed, HOWEVER, the conversion to and from floating
	// point can be cycle intensive, so if you can keep your calcs in ints then you can gain some speed
	// also note, type 1 spot lights are simply point lights with direction, the "cone" is more of a function
	// of the falloff due to attenuation, but they still look like spot lights
	// type 2 spot lights are implemented with the intensity having a dot product relationship with the
	// angle from the surface point to the light direction just like in the optimized model, but the pf term
	// that is used for a concentration control must be 1,2,3,.... integral and non-fractional
	// this function now performs emissive, flat, and gouraud lighting, results are stored in the 
	// lit_color[] array of each polygon

	unsigned int tmpa;
	unsigned int r_base, g_base, b_base,  // base color being lit
				 r_sum,  g_sum,  b_sum,   // sum of lighting process over all lights
				 r_sum0,  g_sum0,  b_sum0,
				 r_sum1,  g_sum1,  b_sum1,
				 r_sum2,  g_sum2,  b_sum2,
				 ri,gi,bi,
				 shaded_color;            // final color

	float dp,     // dot product 
		  dist,   // distance from light to surface
		  dists, 
		  i,      // general intensities
		  nl,     // length of normal
		  atten;  // attenuation computations

	vec4 u, v, n, l, d, s; // used for cross product and light vector calculations

	//Write_Error("\nEntering lighting function");

	// for each valid poly, light it...
	for (int poly=0; poly < _num_polys; poly++)
	{
		// acquire polygon
		PolygonF* curr_poly = _poly_ptrs[poly];

		// light this polygon if and only if it's not clipped, not culled,
		// active, and visible
		if (!(curr_poly->state & POLY_STATE_ACTIVE) ||
			(curr_poly->state & POLY_STATE_CLIPPED ) ||
			(curr_poly->state & POLY_STATE_BACKFACE) ||
			(curr_poly->state & POLY_STATE_LIT) )
			continue; // move onto next poly

		//Write_Error("\npoly %d",poly);

#ifdef DEBUG_ON
		// track rendering stats
		debug_polys_lit_per_frame++;
#endif

		// set state of polygon to lit
		SET_BIT(curr_poly->state, POLY_STATE_LIT);

		// we will use the transformed polygon vertex list since the backface removal
		// only makes sense at the world coord stage further of the pipeline 

		LightsMgr& lights = Modules::GetGraphics().GetLights();

		// test the lighting mode of the polygon (use flat for flat, gouraud))
		if (curr_poly->attr & POLY_ATTR_SHADE_MODE_FLAT)
		{
			 //Write_Error("\nEntering Flat Shader");

			// step 1: extract the base color out in RGB mode
			// assume 888 format
			_RGB8888FROM32BIT(curr_poly->color, &tmpa, &r_base, &g_base, &b_base);

			//Write_Error("\nBase color=%d,%d,%d", r_base, g_base, b_base);

			// initialize color sum
			r_sum  = 0;
			g_sum  = 0;
			b_sum  = 0;

			//Write_Error("\nsum color=%d,%d,%d", r_sum, g_sum, b_sum);

			// new optimization:
			// when there are multiple lights in the system we will end up performing numerous
			// redundant calculations to minimize this my strategy is to set key variables to 
			// to MAX values on each loop, then during the lighting calcs to test the vars for
			// the max value, if they are the max value then the first light that needs the math
			// will do it, and then save the information into the variable (causing it to change state
			// from an invalid number) then any other lights that need the math can use the previously
			// computed value

			// set surface normal.z to FLT_MAX to flag it as non-computed
			n.z = FLT_MAX;

			// loop thru lights
			for (int curr_light = 0; curr_light < lights.Size(); curr_light++)
			{
				// is this light active
				if (lights[curr_light].state==LIGHT_STATE_OFF)
					continue;

				//Write_Error("\nprocessing light %d",curr_light);

				// what kind of light are we dealing with
				if (lights[curr_light].attr & LIGHT_ATTR_AMBIENT)
				{
					//Write_Error("\nEntering ambient light...");

					// simply multiply each channel against the color of the 
					// polygon then divide by 256 to scale back to 0..255
					// use a shift in real life!!! >> 8
					r_sum+= ((lights[curr_light].c_ambient.r * r_base) / 256);
					g_sum+= ((lights[curr_light].c_ambient.g * g_base) / 256);
					b_sum+= ((lights[curr_light].c_ambient.b * b_base) / 256);

					//Write_Error("\nambient sum=%d,%d,%d", r_sum, g_sum, b_sum);

					// there better only be one ambient light!

				} // end if
				else if (lights[curr_light].attr & LIGHT_ATTR_INFINITE)
				{
					//Write_Error("\nEntering infinite light...");

					// infinite lighting, we need the surface normal, and the direction
					// of the light source

					// test if we already computed poly normal in previous calculation
					if (n.z==FLT_MAX)       
					{
						// we need to compute the normal of this polygon face, and recall
						// that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

						// build u, v
						u = curr_poly->tvlist[1].v - curr_poly->tvlist[0].v;
						v = curr_poly->tvlist[2].v - curr_poly->tvlist[0].v;

						// compute cross product
						n = u.Cross(v);
					}

					// at this point, we are almost ready, but we have to normalize the normal vector!
					// this is a key optimization we can make later, we can pre-compute the length of all polygon
					// normals, so this step can be optimized
					// compute length of normal
					//nl = n.LengthFast();
					nl = curr_poly->nlength;

					// ok, recalling the lighting model for infinite lights
					// I(d)dir = I0dir * Cldir
					// and for the diffuse model
					// Itotald =   Rsdiffuse*Idiffuse * (n . l)
					// so we basically need to multiple it all together
					// notice the scaling by 128, I want to avoid floating point calculations, not because they 
					// are slower, but the conversion to and from cost cycles

					dp = n.Dot(lights[curr_light].dir);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						i = 128*dp/nl; 
						r_sum+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
						g_sum+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
						b_sum+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
					} // end if

					//Write_Error("\ninfinite sum=%d,%d,%d", r_sum, g_sum, b_sum);

				} // end if infinite light
				else if (lights[curr_light].attr & LIGHT_ATTR_POINT)
				{
					//Write_Error("\nEntering point light...");

					// perform point light computations
					// light model for point light is once again:
					//              I0point * Clpoint
					//  I(d)point = ___________________
					//              kc +  kl*d + kq*d2              
					//
					//  Where d = |p - s|
					// thus it's almost identical to the infinite light, but attenuates as a function
					// of distance from the point source to the surface point being lit

					// test if we already computed poly normal in previous calculation
					if (n.z==FLT_MAX)       
					{
						// we need to compute the normal of this polygon face, and recall
						// that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

						// build u, v
						u = curr_poly->tvlist[1].v - curr_poly->tvlist[0].v;
						v = curr_poly->tvlist[2].v - curr_poly->tvlist[0].v;

						// compute cross product
						n = u.Cross(v);
					}

					// at this point, we are almost ready, but we have to normalize the normal vector!
					// this is a key optimization we can make later, we can pre-compute the length of all polygon
					// normals, so this step can be optimized
					// compute length of normal
					//nl = n.LengthFast();
					nl = curr_poly->nlength;  

					// compute vector from surface to light
					l = lights[curr_light].pos - curr_poly->tvlist[0].v;

					// compute distance and attenuation
					dist = l.LengthFast();

					// and for the diffuse model
					// Itotald =   Rsdiffuse*Idiffuse * (n . l)
					// so we basically need to multiple it all together
					// notice the scaling by 128, I want to avoid floating point calculations, not because they 
					// are slower, but the conversion to and from cost cycles
					dp = n.Dot(l);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

						i = 128*dp / (nl * dist * atten ); 

						r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
						g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
						b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
					}

					//Write_Error("\npoint sum=%d,%d,%d",r_sum,g_sum,b_sum);

				}
				else if (lights[curr_light].attr & LIGHT_ATTR_SPOTLIGHT1)
				{
					//Write_Error("\nentering spot light1...");

					// perform spotlight/point computations simplified model that uses
					// point light WITH a direction to simulate a spotlight
					// light model for point light is once again:
					//              I0point * Clpoint
					//  I(d)point = ___________________
					//              kc +  kl*d + kq*d2              
					//
					//  Where d = |p - s|
					// thus it's almost identical to the infinite light, but attenuates as a function
					// of distance from the point source to the surface point being lit

					// test if we already computed poly normal in previous calculation
					if (n.z==FLT_MAX)       
					{
						// we need to compute the normal of this polygon face, and recall
						// that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

						// build u, v
						u = curr_poly->tvlist[1].v - curr_poly->tvlist[0].v;
						v = curr_poly->tvlist[2].v - curr_poly->tvlist[0].v;

						// compute cross product
						n = u.Cross(v);
					}

					// at this point, we are almost ready, but we have to normalize the normal vector!
					// this is a key optimization we can make later, we can pre-compute the length of all polygon
					// normals, so this step can be optimized
					// compute length of normal
					//nl = n.LengthFast();
					nl = curr_poly->nlength;  

					// compute vector from surface to light
					l = lights[curr_light].pos - curr_poly->tvlist[0].v;

					// compute distance and attenuation
					dist = l.LengthFast();

					// and for the diffuse model
					// Itotald =   Rsdiffuse*Idiffuse * (n . l)
					// so we basically need to multiple it all together
					// notice the scaling by 128, I want to avoid floating point calculations, not because they 
					// are slower, but the conversion to and from cost cycles

					// note that I use the direction of the light here rather than a the vector to the light
					// thus we are taking orientation into account which is similar to the spotlight model
					dp = n.Dot(lights[curr_light].dir);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

						i = 128*dp / (nl * atten ); 

						r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
						g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
						b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
					}

					//Write_Error("\nspotlight sum=%d,%d,%d",r_sum, g_sum, b_sum);
				}
				else if (lights[curr_light].attr & LIGHT_ATTR_SPOTLIGHT2) // simple version
				{
					//Write_Error("\nEntering spotlight2 ...");

					// perform spot light computations
					// light model for spot light simple version is once again:
					//         	     I0spotlight * Clspotlight * MAX( (l . s), 0)^pf                     
					// I(d)spotlight = __________________________________________      
					//               		 kc + kl*d + kq*d2        
					// Where d = |p - s|, and pf = power factor

					// thus it's almost identical to the point, but has the extra term in the numerator
					// relating the angle between the light source and the point on the surface

					// test if we already computed poly normal in previous calculation
					if (n.z==FLT_MAX)       
					{
						// we need to compute the normal of this polygon face, and recall
						// that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

						// build u, v
						u = curr_poly->tvlist[1].v - curr_poly->tvlist[0].v;
						v = curr_poly->tvlist[2].v - curr_poly->tvlist[0].v;

						// compute cross product
						n = u.Cross(v);
					}

					// at this point, we are almost ready, but we have to normalize the normal vector!
					// this is a key optimization we can make later, we can pre-compute the length of all polygon
					// normals, so this step can be optimized
					// compute length of normal
					//nl = n.LengthFast();
					nl = curr_poly->nlength;  

					// and for the diffuse model
					// Itotald =   Rsdiffuse*Idiffuse * (n . l)
					// so we basically need to multiple it all together
					// notice the scaling by 128, I want to avoid floating point calculations, not because they 
					// are slower, but the conversion to and from cost cycles
					dp = n.Dot(lights[curr_light].dir);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						// compute vector from light to surface (different from l which IS the light dir)
						s = curr_poly->tvlist[0].v - lights[curr_light].pos;

						// compute length of s (distance to light source) to normalize s for lighting calc
						dists = s.LengthFast();

						// compute spot light term (s . l)
						float dpsl = s.Dot(lights[curr_light].dir) / dists;

						// proceed only if term is positive
						if (dpsl > 0) 
						{
							// compute attenuation
							atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

							// for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
							// must be integral
							float dpsl_exp = dpsl;

							// exponentiate for positive integral powers
							for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
								dpsl_exp*=dpsl;

							// now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

							i = 128*dp * dpsl_exp / (nl * atten ); 

							r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
							g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
							b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);

						} // end if
					} // end if
					//Write_Error("\nSpotlight sum=%d,%d,%d",r_sum, g_sum, b_sum);
				} // end if spot light
			} // end for light

			// debug
			if (r_sum == 0 && g_sum == 0 && b_sum == 0)
				int zz = 0;

			// make sure colors aren't out of range
			if (r_sum  > 255) r_sum = 255;
			if (g_sum  > 255) g_sum = 255;
			if (b_sum  > 255) b_sum = 255;

			// write the color over current color
			curr_poly->lit_color[0] = Modules::GetGraphics().GetColor(r_sum, g_sum, b_sum);

		} // end if
		else if (curr_poly->attr & POLY_ATTR_SHADE_MODE_GOURAUD)
		{
			// gouraud shade, unfortunetly at this point in the pipeline, we have lost the original
			// mesh, and only have triangles, thus, many triangles will share the same vertices and
			// they will get lit 2x since we don't have any way to tell this, alas, performing lighting
			// at the object level is a better idea when gouraud shading is performed since the 
			// commonality of vertices is still intact, in any case, lighting here is similar to polygon
			// flat shaded, but we do it 3 times, once for each vertex, additionally there are lots
			// of opportunities for optimization, but I am going to lay off them for now, so the code
			// is intelligible, later we will optimize

			//Write_Error("\nEntering gouraud shader...");

			// step 1: extract the base color out in RGB mode
			// assume 888 format
			_RGB8888FROM32BIT(curr_poly->color, &tmpa, &r_base, &g_base, &b_base);

			//Write_Error("\nBase color=%d, %d, %d", r_base, g_base, b_base);

			// initialize color sum(s) for vertices
			r_sum0  = 0;
			g_sum0  = 0;
			b_sum0  = 0;

			r_sum1  = 0;
			g_sum1  = 0;
			b_sum1  = 0;

			r_sum2  = 0;
			g_sum2  = 0;
			b_sum2  = 0;

			//Write_Error("\nColor sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,   r_sum1, g_sum1, b_sum1, r_sum2, g_sum2, b_sum2);

			// new optimization:
			// when there are multiple lights in the system we will end up performing numerous
			// redundant calculations to minimize this my strategy is to set key variables to 
			// to MAX values on each loop, then during the lighting calcs to test the vars for
			// the max value, if they are the max value then the first light that needs the math
			// will do it, and then save the information into the variable (causing it to change state
			// from an invalid number) then any other lights that need the math can use the previously
			// computed value, however, since we already have the normals, not much here to cache on
			// a large scale, but small scale stuff is there, however, we will optimize those later

			// loop thru lights
			for (int curr_light = 0; curr_light < lights.Size(); curr_light++)
			{
				// is this light active
				if (lights[curr_light].state==LIGHT_STATE_OFF)
					continue;

				//Write_Error("\nprocessing light %d", curr_light);

				// what kind of light are we dealing with
				if (lights[curr_light].attr & LIGHT_ATTR_AMBIENT) ///////////////////////////////
				{
					//Write_Error("\nEntering ambient light....");

					// simply multiply each channel against the color of the 
					// polygon then divide by 256 to scale back to 0..255
					// use a shift in real life!!! >> 8
					ri = ((lights[curr_light].c_ambient.r * r_base) / 256);
					gi = ((lights[curr_light].c_ambient.g * g_base) / 256);
					bi = ((lights[curr_light].c_ambient.b * b_base) / 256);

					// ambient light has the same affect on each vertex
					r_sum0+=ri;
					g_sum0+=gi;
					b_sum0+=bi;

					r_sum1+=ri;
					g_sum1+=gi;
					b_sum1+=bi;

					r_sum2+=ri;
					g_sum2+=gi;
					b_sum2+=bi;

					// there better only be one ambient light!
					//Write_Error("\nexiting ambient ,sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

				} // end if
				else if (lights[curr_light].attr & LIGHT_ATTR_INFINITE) /////////////////////////////////
				{
					//Write_Error("\nentering infinite light...");

					// infinite lighting, we need the surface normal, and the direction
					// of the light source

					// no longer need to compute normal or length, we already have the vertex normal
					// and it's length is 1.0  
					// ....

					// ok, recalling the lighting model for infinite lights
					// I(d)dir = I0dir * Cldir
					// and for the diffuse model
					// Itotald =   Rsdiffuse*Idiffuse * (n . l)
					// so we basically need to multiple it all together
					// notice the scaling by 128, I want to avoid floating point calculations, not because they 
					// are slower, but the conversion to and from cost cycles

					// need to perform lighting for each vertex (lots of redundant math, optimize later!)

					//Write_Error("\nv0=[%f, %f, %f]=%f, v1=[%f, %f, %f]=%f, v2=[%f, %f, %f]=%f",
					// curr_poly->tvlist[0].n.x, curr_poly->tvlist[0].n.y,curr_poly->tvlist[0].n.z, VECTOR4D_Length(&curr_poly->tvlist[0].n),
					// curr_poly->tvlist[1].n.x, curr_poly->tvlist[1].n.y,curr_poly->tvlist[1].n.z, VECTOR4D_Length(&curr_poly->tvlist[1].n),
					// curr_poly->tvlist[2].n.x, curr_poly->tvlist[2].n.y,curr_poly->tvlist[2].n.z, VECTOR4D_Length(&curr_poly->tvlist[2].n) );

					// vertex 0
					dp = curr_poly->tvlist[0].n.Dot(lights[curr_light].dir);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						i = 128*dp; 
						r_sum0+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
						g_sum0+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
						b_sum0+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
					} // end if

					// vertex 1
					dp = curr_poly->tvlist[1].n.Dot(lights[curr_light].dir);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						i = 128*dp; 
						r_sum1+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
						g_sum1+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
						b_sum1+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
					} // end if

					// vertex 2
					dp = curr_poly->tvlist[2].n.Dot(lights[curr_light].dir);

					// only add light if dp > 0
					if (dp > 0)
					{ 
						i = 128*dp; 
						r_sum2+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
						g_sum2+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
						b_sum2+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
					} // end if

					//Write_Error("\nexiting infinite, color sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

				} // end if infinite light
				else
					if (lights[curr_light].attr & LIGHT_ATTR_POINT) //////////////////////////////////////
					{
						// perform point light computations
						// light model for point light is once again:
						//              I0point * Clpoint
						//  I(d)point = ___________________
						//              kc +  kl*d + kq*d2              
						//
						//  Where d = |p - s|
						// thus it's almost identical to the infinite light, but attenuates as a function
						// of distance from the point source to the surface point being lit

						// .. normal already in vertex

						//Write_Error("\nEntering point light....");

						// compute vector from surface to light
						l = lights[curr_light].pos - curr_poly->tvlist[0].v;

						// compute distance and attenuation
						dist = l.LengthFast(); 

						// and for the diffuse model
						// Itotald =   Rsdiffuse*Idiffuse * (n . l)
						// so we basically need to multiple it all together
						// notice the scaling by 128, I want to avoid floating point calculations, not because they 
						// are slower, but the conversion to and from cost cycles

						// perform the calculation for all 3 vertices

						// vertex 0
						dp = curr_poly->tvlist[0].n.Dot(l);

						// only add light if dp > 0
						if (dp > 0)
						{ 
							atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

							i = 128*dp / (dist * atten ); 

							r_sum0 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
							g_sum0 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
							b_sum0 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
						} // end if


						// vertex 1
						dp = curr_poly->tvlist[1].n.Dot(l);

						// only add light if dp > 0
						if (dp > 0)
						{ 
							atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

							i = 128*dp / (dist * atten ); 

							r_sum1 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
							g_sum1 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
							b_sum1 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
						} // end if

						// vertex 2
						dp = curr_poly->tvlist[2].n.Dot(l);

						// only add light if dp > 0
						if (dp > 0)
						{ 
							atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

							i = 128*dp / (dist * atten ); 

							r_sum2 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
							g_sum2 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
							b_sum2 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
						} // end if

						//Write_Error("\nexiting point light, rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1, r_sum2, g_sum2, b_sum2);

					} // end if point
					else if (lights[curr_light].attr & LIGHT_ATTR_SPOTLIGHT1) ///////////////////////////////////////
					{
						// perform spotlight/point computations simplified model that uses
						// point light WITH a direction to simulate a spotlight
						// light model for point light is once again:
						//              I0point * Clpoint
						//  I(d)point = ___________________
						//              kc +  kl*d + kq*d2              
						//
						//  Where d = |p - s|
						// thus it's almost identical to the infinite light, but attenuates as a function
						// of distance from the point source to the surface point being lit

						//Write_Error("\nentering spotlight1....");

						// .. normal is already computed

						// compute vector from surface to light
						l = lights[curr_light].pos - curr_poly->tvlist[0].v;

						// compute distance and attenuation
						dist = l.LengthFast();

						// and for the diffuse model
						// Itotald =   Rsdiffuse*Idiffuse * (n . l)
						// so we basically need to multiple it all together
						// notice the scaling by 128, I want to avoid floating point calculations, not because they 
						// are slower, but the conversion to and from cost cycles

						// note that I use the direction of the light here rather than a the vector to the light
						// thus we are taking orientation into account which is similar to the spotlight model

						// vertex 0
						dp = curr_poly->tvlist[0].n.Dot(lights[curr_light].dir);

						// only add light if dp > 0
						if (dp > 0)
						{ 
							atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

							i = 128*dp / ( atten ); 

							r_sum0 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
							g_sum0 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
							b_sum0 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
						} // end if

						// vertex 1
						dp = curr_poly->tvlist[1].n.Dot(lights[curr_light].dir);

						// only add light if dp > 0
						if (dp > 0)
						{ 
							atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

							i = 128*dp / ( atten ); 

							r_sum1 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
							g_sum1 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
							b_sum1 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
						} // end i

						// vertex 2
						dp = curr_poly->tvlist[2].n.Dot(lights[curr_light].dir);

						// only add light if dp > 0
						if (dp > 0)
						{ 
							atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

							i = 128*dp / ( atten ); 

							r_sum2 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
							g_sum2 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
							b_sum2 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
						} // end i

						//Write_Error("\nexiting spotlight1, sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

					} // end if spotlight1
					else if (lights[curr_light].attr & LIGHT_ATTR_SPOTLIGHT2) // simple version //////////////////////////
					{
						// perform spot light computations
						// light model for spot light simple version is once again:
						//         	     I0spotlight * Clspotlight * MAX( (l . s), 0)^pf                     
						// I(d)spotlight = __________________________________________      
						//               		 kc + kl*d + kq*d2        
						// Where d = |p - s|, and pf = power factor

						// thus it's almost identical to the point, but has the extra term in the numerator
						// relating the angle between the light source and the point on the surface

						// .. already have normals and length are 1.0

						// and for the diffuse model
						// Itotald =   Rsdiffuse*Idiffuse * (n . l)
						// so we basically need to multiple it all together
						// notice the scaling by 128, I want to avoid floating point calculations, not because they 
						// are slower, but the conversion to and from cost cycles

						//Write_Error("\nEntering spotlight2...");

						// tons of redundant math here! lots to optimize later!

						// vertex 0
						dp = curr_poly->tvlist[0].n.Dot(lights[curr_light].dir);

						// only add light if dp > 0
						if (dp > 0)
						{ 
							// compute vector from light to surface (different from l which IS the light dir)
							s = curr_poly->tvlist[0].v - lights[curr_light].pos;

							// compute length of s (distance to light source) to normalize s for lighting calc
							dists = s.LengthFast();

							// compute spot light term (s . l)
							float dpsl = s.Dot(lights[curr_light].dir) / dists;

							// proceed only if term is positive
							if (dpsl > 0) 
							{
								// compute attenuation
								atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

								// for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
								// must be integral
								float dpsl_exp = dpsl;

								// exponentiate for positive integral powers
								for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
									dpsl_exp*=dpsl;

								// now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

								i = 128*dp * dpsl_exp / ( atten ); 

								r_sum0 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
								g_sum0 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
								b_sum0 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);

							} // end if

						} // end if

						// vertex 1
						dp = curr_poly->tvlist[1].n.Dot(lights[curr_light].dir);

						// only add light if dp > 0
						if (dp > 0)
						{ 
							// compute vector from light to surface (different from l which IS the light dir)
							s = curr_poly->tvlist[1].v - lights[curr_light].pos;

							// compute length of s (distance to light source) to normalize s for lighting calc
							dists = s.LengthFast();

							// compute spot light term (s . l)
							float dpsl = s.Dot(lights[curr_light].dir) / dists;

							// proceed only if term is positive
							if (dpsl > 0) 
							{
								// compute attenuation
								atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

								// for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
								// must be integral
								float dpsl_exp = dpsl;

								// exponentiate for positive integral powers
								for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
									dpsl_exp*=dpsl;

								// now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

								i = 128*dp * dpsl_exp / ( atten ); 

								r_sum1 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
								g_sum1 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
								b_sum1 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);

							} // end if

						} // end if

						// vertex 2
						dp = curr_poly->tvlist[2].n.Dot(lights[curr_light].dir);

						// only add light if dp > 0
						if (dp > 0)
						{ 
							// compute vector from light to surface (different from l which IS the light dir)
							s = curr_poly->tvlist[2].v - lights[curr_light].pos;

							// compute length of s (distance to light source) to normalize s for lighting calc
							dists = s.LengthFast();

							// compute spot light term (s . l)
							float dpsl = s.Dot(lights[curr_light].dir) / dists;

							// proceed only if term is positive
							if (dpsl > 0) 
							{
								// compute attenuation
								atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

								// for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
								// must be integral
								float dpsl_exp = dpsl;

								// exponentiate for positive integral powers
								for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
									dpsl_exp*=dpsl;

								// now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

								i = 128*dp * dpsl_exp / ( atten ); 

								r_sum2 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
								g_sum2 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
								b_sum2 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
							} // end if

						} // end if

						//Write_Error("\nexiting spotlight2, sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,   r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

					} // end if spot light

			} // end for light

			// make sure colors aren't out of range
			if (r_sum0  > 255) r_sum0 = 255;
			if (g_sum0  > 255) g_sum0 = 255;
			if (b_sum0  > 255) b_sum0 = 255;

			if (r_sum1  > 255) r_sum1 = 255;
			if (g_sum1  > 255) g_sum1 = 255;
			if (b_sum1  > 255) b_sum1 = 255;

			if (r_sum2  > 255) r_sum2 = 255;
			if (g_sum2  > 255) g_sum2 = 255;
			if (b_sum2  > 255) b_sum2 = 255;

			//Write_Error("\nwriting color for poly %d", poly);

			//Write_Error("\n******** final sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1, r_sum2, g_sum2, b_sum2);

			// write the colors
			curr_poly->lit_color[0] = Modules::GetGraphics().GetColor(r_sum0, g_sum0, b_sum0);
			curr_poly->lit_color[1] = Modules::GetGraphics().GetColor(r_sum1, g_sum1, b_sum1);
			curr_poly->lit_color[2] = Modules::GetGraphics().GetColor(r_sum2, g_sum2, b_sum2);
		}
		else // assume POLY_ATTR_SHADE_MODE_CONSTANT
		{
			// emmisive shading only, do nothing
			// ...
			curr_poly->lit_color[0] = curr_poly->color;

			//Write_Error("\nentering constant shader, and exiting...");

		} // end if

	} // end for poly
}

void RenderList::DrawWire32(unsigned char* video_buffer, int lpitch)
{
	// this function "executes" the render list or in other words
	// draws all the faces in the list in wire frame 16bit mode
	// note there is no need to sort wire frame polygons, but 
	// later we will need to, so hidden surfaces stay hidden
	// also, we leave it to the function to determine the bitdepth
	// and call the correct rasterizer

	// at this point, all we have is a list of polygons and it's time
	// to draw them
	for (int poly=0; poly < _num_polys; poly++)
	{
		// render this polygon if and only if it's not clipped, not culled,
		// active, and visible, note however the concecpt of "backface" is 
		// irrelevant in a wire frame engine though
		if (!(_poly_ptrs[poly]->state & POLY_STATE_ACTIVE) ||
			(_poly_ptrs[poly]->state & POLY_STATE_CLIPPED ) ||
			(_poly_ptrs[poly]->state & POLY_STATE_BACKFACE) )
			continue; // move onto next poly

		Modules::GetLog().WriteError("color %0x", _poly_ptrs[poly]->lit_color[0]);

		// draw the triangle edge one, note that clipping was already set up
		// by 2D initialization, so line clipper will clip all polys out
		// of the 2D screen/window boundary
		DrawClipLine32(_poly_ptrs[poly]->tvlist[0].x, 
			_poly_ptrs[poly]->tvlist[0].y,
			_poly_ptrs[poly]->tvlist[1].x, 
			_poly_ptrs[poly]->tvlist[1].y,
			_poly_ptrs[poly]->lit_color[0],
			video_buffer, lpitch);

		DrawClipLine32(_poly_ptrs[poly]->tvlist[1].x, 
			_poly_ptrs[poly]->tvlist[1].y,
			_poly_ptrs[poly]->tvlist[2].x, 
			_poly_ptrs[poly]->tvlist[2].y,
			_poly_ptrs[poly]->lit_color[0],
			video_buffer, lpitch);

		DrawClipLine32(_poly_ptrs[poly]->tvlist[2].x, 
			_poly_ptrs[poly]->tvlist[2].y,
			_poly_ptrs[poly]->tvlist[0].x, 
			_poly_ptrs[poly]->tvlist[0].y,
			_poly_ptrs[poly]->lit_color[0],
			video_buffer, lpitch);

#ifdef DEBUG_ON
		debug_polys_rendered_per_frame++;
#endif
	}
}

void RenderList::DrawSolid32(unsigned char* video_buffer, int lpitch)
{
	// 32-bit version
	// this function "executes" the render list or in other words
	// draws all the faces in the list, the function will call the 
	// proper rasterizer based on the lighting model of the polygons

	PolygonF face; // temp face used to render polygon

	// at this point, all we have is a list of polygons and it's time
	// to draw them
	for (int poly=0; poly < _num_polys; poly++)
	{
		// render this polygon if and only if it's not clipped, not culled,
		// active, and visible, note however the concecpt of "backface" is 
		// irrelevant in a wire frame engine though
		if (!(_poly_ptrs[poly]->state & POLY_STATE_ACTIVE) ||
			(_poly_ptrs[poly]->state & POLY_STATE_CLIPPED ) ||
			(_poly_ptrs[poly]->state & POLY_STATE_BACKFACE) )
			continue; // move onto next poly

		// need to test for textured first, since a textured poly can either
		// be emissive, or flat shaded, hence we need to call different
		// rasterizers    
		if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_TEXTURE)
		{

			// set the vertices
			face.tvlist[0].x = _poly_ptrs[poly]->tvlist[0].x;
			face.tvlist[0].y = _poly_ptrs[poly]->tvlist[0].y;
			face.tvlist[0].z = _poly_ptrs[poly]->tvlist[0].z;
			face.tvlist[0].u0 = _poly_ptrs[poly]->tvlist[0].u0;
			face.tvlist[0].v0 = _poly_ptrs[poly]->tvlist[0].v0;

			face.tvlist[1].x = _poly_ptrs[poly]->tvlist[1].x;
			face.tvlist[1].y = _poly_ptrs[poly]->tvlist[1].y;
			face.tvlist[1].z = _poly_ptrs[poly]->tvlist[1].z;
			face.tvlist[1].u0 = _poly_ptrs[poly]->tvlist[1].u0;
			face.tvlist[1].v0 = _poly_ptrs[poly]->tvlist[1].v0;

			face.tvlist[2].x = _poly_ptrs[poly]->tvlist[2].x;
			face.tvlist[2].y = _poly_ptrs[poly]->tvlist[2].y;
			face.tvlist[2].z = _poly_ptrs[poly]->tvlist[2].z;
			face.tvlist[2].u0 = _poly_ptrs[poly]->tvlist[2].u0;
			face.tvlist[2].v0 = _poly_ptrs[poly]->tvlist[2].v0;


			// assign the texture
			face.texture = _poly_ptrs[poly]->texture;

			// is this a plain emissive texture?
			if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_CONSTANT)
			{
				// draw the textured triangle as emissive
				DrawTexturedTriangle32(&face, video_buffer, lpitch);
				//Draw_Textured_Perspective_Triangle_16(&face, video_buffer, lpitch);
			}
 			else if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_FLAT)
 			{
 				// draw as flat shaded
 				face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];
 				DrawTexturedTriangleFS32(&face, video_buffer, lpitch);
				//Draw_Textured_Perspective_Triangle_FS_16(&face, video_buffer, lpitch);
 			}
			else
			{
				// must be gouraud POLY_ATTR_SHADE_MODE_GOURAUD
				face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];
				face.lit_color[1] = _poly_ptrs[poly]->lit_color[1];
				face.lit_color[2] = _poly_ptrs[poly]->lit_color[2];
				DrawTexturedTriangleGS32(&face, video_buffer, lpitch);
			}
		}
		else if ((_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_FLAT) || 
				(_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_CONSTANT) )
		{
			// draw as constant shaded
			face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];

			// set the vertices
			face.tvlist[0].x = _poly_ptrs[poly]->tvlist[0].x;
			face.tvlist[0].y = _poly_ptrs[poly]->tvlist[0].y;
			face.tvlist[0].z  = _poly_ptrs[poly]->tvlist[0].z;

			face.tvlist[1].x = _poly_ptrs[poly]->tvlist[1].x;
			face.tvlist[1].y = _poly_ptrs[poly]->tvlist[1].y;
			face.tvlist[1].z  = _poly_ptrs[poly]->tvlist[1].z;

			face.tvlist[2].x = _poly_ptrs[poly]->tvlist[2].x;
			face.tvlist[2].y = _poly_ptrs[poly]->tvlist[2].y;
			face.tvlist[2].z  = _poly_ptrs[poly]->tvlist[2].z;

			// draw the triangle with basic flat rasterizer
			DrawTriangle32(&face, video_buffer, lpitch);
		}
		else if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_GOURAUD)
		{
			// {andre take advantage of the data structures later..}
			// set the vertices
			face.tvlist[0].x  = _poly_ptrs[poly]->tvlist[0].x;
			face.tvlist[0].y  = _poly_ptrs[poly]->tvlist[0].y;
			face.tvlist[0].z  = _poly_ptrs[poly]->tvlist[0].z;
			face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];

			face.tvlist[1].x  = _poly_ptrs[poly]->tvlist[1].x;
			face.tvlist[1].y  = _poly_ptrs[poly]->tvlist[1].y;
			face.tvlist[1].z  = _poly_ptrs[poly]->tvlist[1].z;
			face.lit_color[1] = _poly_ptrs[poly]->lit_color[1];

			face.tvlist[2].x  = _poly_ptrs[poly]->tvlist[2].x;
			face.tvlist[2].y  = _poly_ptrs[poly]->tvlist[2].y;
			face.tvlist[2].z  = _poly_ptrs[poly]->tvlist[2].z;
			face.lit_color[2] = _poly_ptrs[poly]->lit_color[2];

			// draw the gouraud shaded triangle
			DrawGouraudTriangle32(&face, video_buffer, lpitch);
		}
	}
}

void RenderList::DrawSolidZB32(unsigned char* video_buffer, int lpitch,
							   unsigned char* zbuffer, int zpitch)
{
	// 32-bit version
	// this function "executes" the render list or in other words
	// draws all the faces in the list, the function will call the 
	// proper rasterizer based on the lighting model of the polygons


	PolygonF face; // temp face used to render polygon

	// at this point, all we have is a list of polygons and it's time
	// to draw them
	for (int poly=0; poly < _num_polys; poly++)
	{
		// render this polygon if and only if it's not clipped, not culled,
		// active, and visible, note however the concecpt of "backface" is 
		// irrelevant in a wire frame engine though
		if (!(_poly_ptrs[poly]->state & POLY_STATE_ACTIVE) ||
			(_poly_ptrs[poly]->state & POLY_STATE_CLIPPED ) ||
			(_poly_ptrs[poly]->state & POLY_STATE_BACKFACE) )
			continue; // move onto next poly

		// need to test for textured first, since a textured poly can either
		// be emissive, or flat shaded, hence we need to call different
		// rasterizers    
		if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_TEXTURE)
		{

			// set the vertices
			face.tvlist[0].x = _poly_ptrs[poly]->tvlist[0].x;
			face.tvlist[0].y = _poly_ptrs[poly]->tvlist[0].y;
			face.tvlist[0].z  = _poly_ptrs[poly]->tvlist[0].z;
			face.tvlist[0].u0 = _poly_ptrs[poly]->tvlist[0].u0;
			face.tvlist[0].v0 = _poly_ptrs[poly]->tvlist[0].v0;

			face.tvlist[1].x = _poly_ptrs[poly]->tvlist[1].x;
			face.tvlist[1].y = _poly_ptrs[poly]->tvlist[1].y;
			face.tvlist[1].z  = _poly_ptrs[poly]->tvlist[1].z;
			face.tvlist[1].u0 = _poly_ptrs[poly]->tvlist[1].u0;
			face.tvlist[1].v0 = _poly_ptrs[poly]->tvlist[1].v0;

			face.tvlist[2].x = _poly_ptrs[poly]->tvlist[2].x;
			face.tvlist[2].y = _poly_ptrs[poly]->tvlist[2].y;
			face.tvlist[2].z  = _poly_ptrs[poly]->tvlist[2].z;
			face.tvlist[2].u0 = _poly_ptrs[poly]->tvlist[2].u0;
			face.tvlist[2].v0 = _poly_ptrs[poly]->tvlist[2].v0;


			// assign the texture
			face.texture = _poly_ptrs[poly]->texture;

			// is this a plain emissive texture?
			if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_CONSTANT)
			{
				// draw the textured triangle as emissive
				DrawTexturedTriangleZB32(&face, video_buffer, lpitch,zbuffer,zpitch);
			} // end if
			else if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_FLAT)
			{
				// draw as flat shaded
				face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];
				DrawTexturedTriangleFSZB32(&face, video_buffer, lpitch,zbuffer,zpitch);
			}
			else
			{
				// must be gouraud POLY_ATTR_SHADE_MODE_GOURAUD
				face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];
				face.lit_color[1] = _poly_ptrs[poly]->lit_color[1];
				face.lit_color[2] = _poly_ptrs[poly]->lit_color[2];

				DrawTexturedTriangleGSZB32(&face, video_buffer, lpitch,zbuffer,zpitch);
			} // end else

		} // end if      
		else if ((_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_FLAT) || 
				(_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_CONSTANT) )
		{
			// draw as constant shaded
			face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];

			// set the vertices
			face.tvlist[0].x = _poly_ptrs[poly]->tvlist[0].x;
			face.tvlist[0].y = _poly_ptrs[poly]->tvlist[0].y;
			face.tvlist[0].z  = _poly_ptrs[poly]->tvlist[0].z;

			face.tvlist[1].x = _poly_ptrs[poly]->tvlist[1].x;
			face.tvlist[1].y = _poly_ptrs[poly]->tvlist[1].y;
			face.tvlist[1].z  = _poly_ptrs[poly]->tvlist[1].z;

			face.tvlist[2].x = _poly_ptrs[poly]->tvlist[2].x;
			face.tvlist[2].y = _poly_ptrs[poly]->tvlist[2].y;
			face.tvlist[2].z  = _poly_ptrs[poly]->tvlist[2].z;

			// draw the triangle with basic flat rasterizer
			DrawTriangleZB32(&face, video_buffer, lpitch,zbuffer,zpitch);

		} // end if
		else if (_poly_ptrs[poly]->attr & POLY_ATTR_SHADE_MODE_GOURAUD)
		{
			// {andre take advantage of the data structures later..}
			// set the vertices
			face.tvlist[0].x  = _poly_ptrs[poly]->tvlist[0].x;
			face.tvlist[0].y  = _poly_ptrs[poly]->tvlist[0].y;
			face.tvlist[0].z  = _poly_ptrs[poly]->tvlist[0].z;
			face.lit_color[0] = _poly_ptrs[poly]->lit_color[0];

			face.tvlist[1].x  = _poly_ptrs[poly]->tvlist[1].x;
			face.tvlist[1].y  = _poly_ptrs[poly]->tvlist[1].y;
			face.tvlist[1].z  = _poly_ptrs[poly]->tvlist[1].z;
			face.lit_color[1] = _poly_ptrs[poly]->lit_color[1];

			face.tvlist[2].x  = _poly_ptrs[poly]->tvlist[2].x;
			face.tvlist[2].y  = _poly_ptrs[poly]->tvlist[2].y;
			face.tvlist[2].z  = _poly_ptrs[poly]->tvlist[2].z;
			face.lit_color[2] = _poly_ptrs[poly]->lit_color[2];

			// draw the gouraud shaded triangle
			DrawGouraudTriangleZB32(&face, video_buffer, lpitch,zbuffer,zpitch);
		} // end if gouraud

	} // end for poly
}

void RenderList::DrawTextured32(unsigned char* video_buffer, 
								int lpitch, BmpImg* texture)
{
	// TEST FUNCTION ONLY!!!!

	PolygonF face; // temp face used to render polygon

	// at this point, all we have is a list of polygons and it's time
	// to draw them
	for (int poly=0; poly < _num_polys; poly++)
	{
		// render this polygon if and only if it's not clipped, not culled,
		// active, and visible, note however the concecpt of "backface" is 
		// irrelevant in a wire frame engine though
		if (!(_poly_ptrs[poly]->state & POLY_STATE_ACTIVE) ||
			(_poly_ptrs[poly]->state & POLY_STATE_CLIPPED ) ||
			(_poly_ptrs[poly]->state & POLY_STATE_BACKFACE) )
			continue; // move onto next poly

		// set the vertices
		face.tvlist[0].x = (int)_poly_ptrs[poly]->tvlist[0].x;
		face.tvlist[0].y = (int)_poly_ptrs[poly]->tvlist[0].y;
		face.tvlist[0].u0 = 0;
		face.tvlist[0].v0 = 0;

		face.tvlist[1].x = (int)_poly_ptrs[poly]->tvlist[1].x;
		face.tvlist[1].y = (int)_poly_ptrs[poly]->tvlist[1].y;
		face.tvlist[1].u0 = 0;
		face.tvlist[1].v0 = 63;

		face.tvlist[2].x = (int)_poly_ptrs[poly]->tvlist[2].x;
		face.tvlist[2].y = (int)_poly_ptrs[poly]->tvlist[2].y;
		face.tvlist[2].u0 = 63;
		face.tvlist[2].v0 = 63;

		// assign the texture
		face.texture = texture;

		// draw the textured triangle
		DrawTexturedTriangle32(&face, video_buffer, lpitch);

	} // end for poly
}

int CompareAvgZ(const void *arg1, const void *arg2)
{
	// this function comapares the average z's of two polygons and is used by the
	// depth sort surface ordering algorithm

	float z1, z2;

	PolygonF *poly_1, *poly_2;

	// dereference the poly pointers
	poly_1 = *((PolygonF**)(arg1));
	poly_2 = *((PolygonF**)(arg2));

	// compute z average of each polygon
	z1 = (float)0.33333*(poly_1->tvlist[0].z + poly_1->tvlist[1].z + poly_1->tvlist[2].z);

	// now polygon 2
	z2 = (float)0.33333*(poly_2->tvlist[0].z + poly_2->tvlist[1].z + poly_2->tvlist[2].z);

	// compare z1 and z2, such that polys' will be sorted in descending Z order
	if (z1 > z2)
		return(-1);
	else if (z1 < z2)
		return(1);
	else
		return(0);
}

int CompareNearZ(const void *arg1, const void *arg2)
{
	// this function comapares the closest z's of two polygons and is used by the
	// depth sort surface ordering algorithm

	float z1, z2;

	PolygonF *poly_1, *poly_2;

	// dereference the poly pointers
	poly_1 = *((PolygonF**)(arg1));
	poly_2 = *((PolygonF**)(arg2));

	// compute the near z of each polygon
	z1 = min(poly_1->tvlist[0].z, poly_1->tvlist[1].z);
	z1 = min(z1, poly_1->tvlist[2].z);

	z2 = min(poly_2->tvlist[0].z, poly_2->tvlist[1].z);
	z2 = min(z2, poly_2->tvlist[2].z);

	// compare z1 and z2, such that polys' will be sorted in descending Z order
	if (z1 > z2)
		return(-1);
	else if (z1 < z2)
		return(1);
	else
		return(0);
}

int CompareFarZ(const void *arg1, const void *arg2)
{
	// this function comapares the farthest z's of two polygons and is used by the
	// depth sort surface ordering algorithm

	float z1, z2;

	PolygonF *poly_1, *poly_2;

	// dereference the poly pointers
	poly_1 = *((PolygonF**)(arg1));
	poly_2 = *((PolygonF**)(arg2));

	// compute the near z of each polygon
	z1 = max(poly_1->tvlist[0].z, poly_1->tvlist[1].z);
	z1 = max(z1, poly_1->tvlist[2].z);

	z2 = max(poly_2->tvlist[0].z, poly_2->tvlist[1].z);
	z2 = max(z2, poly_2->tvlist[2].z);

	// compare z1 and z2, such that polys' will be sorted in descending Z order
	if (z1 > z2)
		return(-1);
	else if (z1 < z2)
		return(1);
	else
		return(0);

} // end CompareFarZ

void RenderList::Sort(int sort_method)
{
	// this function sorts the rendering list based on the polygon z-values 
	// the specific sorting method is controlled by sending in control flags
	// #define SORT_POLYLIST_AVGZ  0 - sorts on average of all vertices
	// #define SORT_POLYLIST_NEARZ 1 - sorts on closest z vertex of each poly
	// #define SORT_POLYLIST_FARZ  2 - sorts on farthest z vertex of each poly

	switch(sort_method)
	{
	case SORT_POLYLIST_AVGZ:  //  - sorts on average of all vertices
		{
			qsort((void *)_poly_ptrs, _num_polys, sizeof(PolygonF*), CompareAvgZ);
		} break;

	case SORT_POLYLIST_NEARZ: // - sorts on closest z vertex of each poly
		{
			qsort((void *)_poly_ptrs, _num_polys, sizeof(PolygonF*), CompareNearZ);
		} break;

	case SORT_POLYLIST_FARZ:  //  - sorts on farthest z vertex of each poly
		{
			qsort((void *)_poly_ptrs, _num_polys, sizeof(PolygonF*), CompareFarZ);
		} break;

	default: break;
	} // end switch
}

void RenderList::ClipPolys(const Camera& cam, int clip_flags)
{
	// this function clips the polygons in the list against the requested clipping planes
	// and sets the clipped flag on the poly, so it's not rendered
	// note the function ONLY performs clipping on the near and far clipping plane
	// but will perform trivial tests on the top/bottom, left/right clipping planes
	// if a polygon is completely out of the viewing frustrum in these cases, it will
	// be culled, however, this test isn't as effective on games based on objects since
	// in most cases objects that are visible have polygons that are visible, but in the
	// case where the polygon list is based on a large object that ALWAYS has some portion
	// visible, testing for individual polys is worthwhile..
	// the function assumes the polygons have been transformed into camera space

	// internal clipping codes
#define CLIP_CODE_GZ   0x0001    // z > z_max
#define CLIP_CODE_LZ   0x0002    // z < z_min
#define CLIP_CODE_IZ   0x0004    // z_min < z < z_max

#define CLIP_CODE_GX   0x0001    // x > x_max
#define CLIP_CODE_LX   0x0002    // x < x_min
#define CLIP_CODE_IX   0x0004    // x_min < x < x_max

#define CLIP_CODE_GY   0x0001    // y > y_max
#define CLIP_CODE_LY   0x0002    // y < y_min
#define CLIP_CODE_IY   0x0004    // y_min < y < y_max

#define CLIP_CODE_NULL 0x0000

	int vertex_ccodes[3]; // used to store clipping flags
	int num_verts_in;     // number of vertices inside
	int v0, v1, v2;       // vertex indices

	float z_factor,       // used in clipping computations
		  z_test;         // used in clipping computations

	float xi, yi, x01i, y01i, x02i, y02i, // vertex intersection points
		  t1, t2,                         // parametric t values
		  ui, vi, u01i, v01i, u02i, v02i; // texture intersection points

	int last_poly_index,            // last valid polygon in polylist
		insert_poly_index;          // the current position new polygons are inserted at

	vec4 u,v,n;                 // used in vector calculations

	PolygonF temp_poly;            // used when we need to split a poly into 2 polys

	// set last, current insert index to end of polygon list
	// we don't want to clip poly's two times
	insert_poly_index = last_poly_index = _num_polys;

	// traverse polygon list and clip/cull polygons
	for (int poly = 0; poly < last_poly_index; poly++)
	{
		// acquire current polygon
		PolygonF* curr_poly = _poly_ptrs[poly];

		// is this polygon valid?
		// test this polygon if and only if it's not clipped, not culled,
		// active, and visible and not 2 sided. Note we test for backface in the event that
		// a previous call might have already determined this, so why work
		// harder!
		if ((curr_poly==NULL) || !(curr_poly->state & POLY_STATE_ACTIVE) ||
			(curr_poly->state & POLY_STATE_CLIPPED ) || 
			(curr_poly->state & POLY_STATE_BACKFACE) )
			continue; // move onto next poly

		// clip/cull to x-planes       
		if (clip_flags & CLIP_POLY_X_PLANE)
		{
			// clip/cull only based on x clipping planes
			// for each vertice determine if it's in the clipping region or beyond it and
			// set the appropriate clipping code
			// we do NOT clip the final triangles, we are only trying to trivally reject them 
			// we are going to clip polygons in the rasterizer to the screen rectangle
			// but we do want to clip/cull polys that are totally outside the viewfrustrum

			// since we are clipping to the right/left x-planes we need to use the FOV or
			// the plane equations to find the z value that at the current x position would
			// be outside the plane
			z_factor = (0.5)*cam.ViewplaneWidth()/cam.ViewDist();  

			// vertex 0

			z_test = z_factor*curr_poly->tvlist[0].z;

			if (curr_poly->tvlist[0].x > z_test)
				vertex_ccodes[0] = CLIP_CODE_GX;
			else if (curr_poly->tvlist[0].x < -z_test)
				vertex_ccodes[0] = CLIP_CODE_LX;
			else
				vertex_ccodes[0] = CLIP_CODE_IX;

			// vertex 1

			z_test = z_factor*curr_poly->tvlist[1].z;         

			if (curr_poly->tvlist[1].x > z_test)
				vertex_ccodes[1] = CLIP_CODE_GX;
			else if (curr_poly->tvlist[1].x < -z_test)
				vertex_ccodes[1] = CLIP_CODE_LX;
			else
				vertex_ccodes[1] = CLIP_CODE_IX;

			// vertex 2

			z_test = z_factor*curr_poly->tvlist[2].z;              

			if (curr_poly->tvlist[2].x > z_test)
				vertex_ccodes[2] = CLIP_CODE_GX;
			else if (curr_poly->tvlist[2].x < -z_test)
				vertex_ccodes[2] = CLIP_CODE_LX;
			else
				vertex_ccodes[2] = CLIP_CODE_IX;

			// test for trivial rejections, polygon completely beyond right or left
			// clipping planes
			if ( ((vertex_ccodes[0] == CLIP_CODE_GX) && 
				(vertex_ccodes[1] == CLIP_CODE_GX) && 
				(vertex_ccodes[2] == CLIP_CODE_GX) ) ||

				((vertex_ccodes[0] == CLIP_CODE_LX) && 
				(vertex_ccodes[1] == CLIP_CODE_LX) && 
				(vertex_ccodes[2] == CLIP_CODE_LX) ) )

			{
				// clip the poly completely out of frustrum
				SET_BIT(curr_poly->state, POLY_STATE_CLIPPED);

				// move on to next polygon
				continue;
			} // end if

		} // end if x planes

		// clip/cull to y-planes       
		if (clip_flags & CLIP_POLY_Y_PLANE)
		{
			// clip/cull only based on y clipping planes
			// for each vertice determine if it's in the clipping region or beyond it and
			// set the appropriate clipping code
			// we do NOT clip the final triangles, we are only trying to trivally reject them 
			// we are going to clip polygons in the rasterizer to the screen rectangle
			// but we do want to clip/cull polys that are totally outside the viewfrustrum

			// since we are clipping to the top/bottom y-planes we need to use the FOV or
			// the plane equations to find the z value that at the current y position would
			// be outside the plane
			z_factor = (0.5)*cam.ViewplaneHeight()/cam.ViewDist();  

			// vertex 0
			z_test = z_factor*curr_poly->tvlist[0].z;

			if (curr_poly->tvlist[0].y > z_test)
				vertex_ccodes[0] = CLIP_CODE_GY;
			else if (curr_poly->tvlist[0].y < -z_test)
				vertex_ccodes[0] = CLIP_CODE_LY;
			else
				vertex_ccodes[0] = CLIP_CODE_IY;

			// vertex 1
			z_test = z_factor*curr_poly->tvlist[1].z;         

			if (curr_poly->tvlist[1].y > z_test)
				vertex_ccodes[1] = CLIP_CODE_GY;
			else if (curr_poly->tvlist[1].y < -z_test)
				vertex_ccodes[1] = CLIP_CODE_LY;
			else
				vertex_ccodes[1] = CLIP_CODE_IY;

			// vertex 2
			z_test = z_factor*curr_poly->tvlist[2].z;              

			if (curr_poly->tvlist[2].y > z_test)
				vertex_ccodes[2] = CLIP_CODE_GY;
			else if (curr_poly->tvlist[2].x < -z_test)
				vertex_ccodes[2] = CLIP_CODE_LY;
			else
				vertex_ccodes[2] = CLIP_CODE_IY;

			// test for trivial rejections, polygon completely beyond top or bottom
			// clipping planes
			if ( ((vertex_ccodes[0] == CLIP_CODE_GY) && 
				(vertex_ccodes[1] == CLIP_CODE_GY) && 
				(vertex_ccodes[2] == CLIP_CODE_GY) ) ||

				((vertex_ccodes[0] == CLIP_CODE_LY) && 
				(vertex_ccodes[1] == CLIP_CODE_LY) && 
				(vertex_ccodes[2] == CLIP_CODE_LY) ) )

			{
				// clip the poly completely out of frustrum
				SET_BIT(curr_poly->state, POLY_STATE_CLIPPED);

				// move on to next polygon
				continue;
			} // end if

		} // end if y planes

		// clip/cull to z planes
		if (clip_flags & CLIP_POLY_Z_PLANE)
		{
			// clip/cull only based on z clipping planes
			// for each vertice determine if it's in the clipping region or beyond it and
			// set the appropriate clipping code
			// then actually clip all polygons to the near clipping plane, this will result
			// in at most 1 additional triangle

			// reset vertex counters, these help in classification
			// of the final triangle 
			num_verts_in = 0;

			// vertex 0
			if (curr_poly->tvlist[0].z > cam.FarClipZ())
			{
				vertex_ccodes[0] = CLIP_CODE_GZ;
			} 
			else if (curr_poly->tvlist[0].z < cam.NearClipZ())
			{
				vertex_ccodes[0] = CLIP_CODE_LZ;
			}
			else
			{
				vertex_ccodes[0] = CLIP_CODE_IZ;
				num_verts_in++;
			} 

			// vertex 1
			if (curr_poly->tvlist[1].z > cam.FarClipZ())
			{
				vertex_ccodes[1] = CLIP_CODE_GZ;
			} 
			else if (curr_poly->tvlist[1].z < cam.NearClipZ())
			{
				vertex_ccodes[1] = CLIP_CODE_LZ;
			}
			else
			{
				vertex_ccodes[1] = CLIP_CODE_IZ;
				num_verts_in++;
			}     

			// vertex 2
			if (curr_poly->tvlist[2].z > cam.FarClipZ())
			{
				vertex_ccodes[2] = CLIP_CODE_GZ;
			} 
			else
			if (curr_poly->tvlist[2].z < cam.NearClipZ())
			{
				vertex_ccodes[2] = CLIP_CODE_LZ;
			}
			else
			{
				vertex_ccodes[2] = CLIP_CODE_IZ;
				num_verts_in++;
			} 

			// test for trivial rejections, polygon completely beyond far or near
			// z clipping planes
			if ( ((vertex_ccodes[0] == CLIP_CODE_GZ) && 
				(vertex_ccodes[1] == CLIP_CODE_GZ) && 
				(vertex_ccodes[2] == CLIP_CODE_GZ) ) ||

				((vertex_ccodes[0] == CLIP_CODE_LZ) && 
				(vertex_ccodes[1] == CLIP_CODE_LZ) && 
				(vertex_ccodes[2] == CLIP_CODE_LZ) ) )

			{
				// clip the poly completely out of frustrum
				SET_BIT(curr_poly->state, POLY_STATE_CLIPPED);

				// move on to next polygon
				continue;
			} // end if

			// test if any vertex has protruded beyond near clipping plane?
			if ( ( (vertex_ccodes[0] | vertex_ccodes[1] | vertex_ccodes[2]) & CLIP_CODE_LZ) )
			{
				// at this point we are ready to clip the polygon to the near 
				// clipping plane no need to clip to the far plane since it can't 
				// possible cause problems. We have two cases: case 1: the triangle 
				// has 1 vertex interior to the near clipping plane and 2 vertices 
				// exterior, OR case 2: the triangle has two vertices interior of 
				// the near clipping plane and 1 exterior

				// step 1: classify the triangle type based on number of vertices
				// inside/outside
				// case 1: easy case :)
				if (num_verts_in == 1)
				{
					// we need to clip the triangle against the near clipping plane
					// the clipping procedure is done to each edge leading away from
					// the interior vertex, to clip we need to compute the intersection
					// with the near z plane, this is done with a parametric equation of 
					// the edge, once the intersection is computed the old vertex position
					// is overwritten along with re-computing the texture coordinates, if
					// there are any, what's nice about this case, is clipping doesn't 
					// introduce any added vertices, so we can overwrite the old poly
					// the other case below results in 2 polys, so at very least one has
					// to be added to the end of the rendering list -- bummer

					// step 1: find vertex index for interior vertex
					if ( vertex_ccodes[0] == CLIP_CODE_IZ) { 
						v0 = 0; v1 = 1; v2 = 2; 
					} else if (vertex_ccodes[1] == CLIP_CODE_IZ) { 
						v0 = 1; v1 = 2; v2 = 0; 
					} else { 
						v0 = 2; v1 = 0; v2 = 1; 
					}

					// step 2: clip each edge
					// basically we are going to generate the parametric line p = v0 + v01*t
					// then solve for t when the z component is equal to near z, then plug that
					// back into to solve for x,y of the 3D line, we could do this with high
					// level code and parametric lines, but to save time, lets do it manually

					// clip edge v0->v1
					v = curr_poly->tvlist[v1].v - curr_poly->tvlist[v0].v;                   

					// the intersection occurs when z = near z, so t = 
					t1 = ( (cam.NearClipZ() - curr_poly->tvlist[v0].z) / v.z );

					// now plug t back in and find x,y intersection with the plane
					xi = curr_poly->tvlist[v0].x + v.x * t1;
					yi = curr_poly->tvlist[v0].y + v.y * t1;

					// now overwrite vertex with new vertex
					curr_poly->tvlist[v1].x = xi;
					curr_poly->tvlist[v1].y = yi;
					curr_poly->tvlist[v1].z = cam.NearClipZ(); 

					// clip edge v0->v2
					v = curr_poly->tvlist[v2].v - curr_poly->tvlist[v0].v;                     

					// the intersection occurs when z = near z, so t = 
					t2 = ( (cam.NearClipZ() - curr_poly->tvlist[v0].z) / v.z );

					// now plug t back in and find x,y intersection with the plane
					xi = curr_poly->tvlist[v0].x + v.x * t2;
					yi = curr_poly->tvlist[v0].y + v.y * t2;

					// now overwrite vertex with new vertex
					curr_poly->tvlist[v2].x = xi;
					curr_poly->tvlist[v2].y = yi;
					curr_poly->tvlist[v2].z = cam.NearClipZ(); 

					// now that we have both t1, t2, check if the poly is textured, if so clip
					// texture coordinates
					if (curr_poly->attr & POLY_ATTR_SHADE_MODE_TEXTURE)
					{
						ui = curr_poly->tvlist[v0].u0 + (curr_poly->tvlist[v1].u0 - curr_poly->tvlist[v0].u0)*t1;
						vi = curr_poly->tvlist[v0].v0 + (curr_poly->tvlist[v1].v0 - curr_poly->tvlist[v0].v0)*t1;
						curr_poly->tvlist[v1].u0 = ui;
						curr_poly->tvlist[v1].v0 = vi;

						ui = curr_poly->tvlist[v0].u0 + (curr_poly->tvlist[v2].u0 - curr_poly->tvlist[v0].u0)*t2;
						vi = curr_poly->tvlist[v0].v0 + (curr_poly->tvlist[v2].v0 - curr_poly->tvlist[v0].v0)*t2;
						curr_poly->tvlist[v2].u0 = ui;
						curr_poly->tvlist[v2].v0 = vi;
					} // end if textured

					// finally, we have obliterated our pre-computed normal length
					// it needs to be recomputed!!!!

					// build u, v
					u = curr_poly->tvlist[v1].v - curr_poly->tvlist[v0].v;
					v = curr_poly->tvlist[v2].v - curr_poly->tvlist[v0].v;

					// compute cross product
					n = u.Cross(v);

					// compute length of normal accurately and store in poly nlength
					// +- epsilon later to fix over/underflows
					curr_poly->nlength = n.LengthFast();

				} // end if
				else if (num_verts_in == 2)
				{   // num_verts = 2

					// must be the case with num_verts_in = 2 
					// we need to clip the triangle against the near clipping plane
					// the clipping procedure is done to each edge leading away from
					// the interior vertex, to clip we need to compute the intersection
					// with the near z plane, this is done with a parametric equation of 
					// the edge, however unlike case 1 above, the triangle will be split
					// into two triangles, thus during the first clip, we will store the 
					// results into a new triangle at the end of the rendering list, and 
					// then on the last clip we will overwrite the triangle being clipped

					// step 0: copy the polygon
					memcpy(&temp_poly, curr_poly, sizeof(PolygonF) );

					// step 1: find vertex index for exterior vertex
					if ( vertex_ccodes[0] == CLIP_CODE_LZ) { 
						v0 = 0; v1 = 1; v2 = 2; 
					} else if (vertex_ccodes[1] == CLIP_CODE_LZ) { 
						v0 = 1; v1 = 2; v2 = 0; 
					} else { 
						v0 = 2; v1 = 0; v2 = 1; 
					}

					// step 2: clip each edge
					// basically we are going to generate the parametric line p = v0 + v01*t
					// then solve for t when the z component is equal to near z, then plug that
					// back into to solve for x,y of the 3D line, we could do this with high
					// level code and parametric lines, but to save time, lets do it manually

					// clip edge v0->v1
					v = curr_poly->tvlist[v1].v - curr_poly->tvlist[v0].v;                        

					// the intersection occurs when z = near z, so t = 
					t1 = ( (cam.NearClipZ() - curr_poly->tvlist[v0].z) / v.z );

					// now plug t back in and find x,y intersection with the plane
					x01i = curr_poly->tvlist[v0].x + v.x * t1;
					y01i = curr_poly->tvlist[v0].y + v.y * t1;

					// clip edge v0->v2
					v = curr_poly->tvlist[v2].v - curr_poly->tvlist[v0].v;            

					// the intersection occurs when z = near z, so t = 
					t2 = ( (cam.NearClipZ() - curr_poly->tvlist[v0].z) / v.z );

					// now plug t back in and find x,y intersection with the plane
					x02i = curr_poly->tvlist[v0].x + v.x * t2;
					y02i = curr_poly->tvlist[v0].y + v.y * t2; 

					// now we have both intersection points, we must overwrite the inplace
					// polygon's vertex 0 with the intersection point, this poly 1 of 2 from
					// the split

					// now overwrite vertex with new vertex
					curr_poly->tvlist[v0].x = x01i;
					curr_poly->tvlist[v0].y = y01i;
					curr_poly->tvlist[v0].z = cam.NearClipZ(); 

					// now comes the hard part, we have to carefully create a new polygon
					// from the 2 intersection points and v2, this polygon will be inserted
					// at the end of the rendering list, but for now, we are building it up
					// in  temp_poly

					// so leave v2 alone, but overwrite v1 with v01, and overwrite v0 with v02
					temp_poly.tvlist[v1].x = x01i;
					temp_poly.tvlist[v1].y = y01i;
					temp_poly.tvlist[v1].z = cam.NearClipZ();              

					temp_poly.tvlist[v0].x = x02i;
					temp_poly.tvlist[v0].y = y02i;
					temp_poly.tvlist[v0].z = cam.NearClipZ();    

					// now that we have both t1, t2, check if the poly is textured, if so clip
					// texture coordinates
					if (curr_poly->attr & POLY_ATTR_SHADE_MODE_TEXTURE)
					{
						// compute poly 1 new texture coordinates from split
						u01i = curr_poly->tvlist[v0].u0 + (curr_poly->tvlist[v1].u0 - curr_poly->tvlist[v0].u0)*t1;
						v01i = curr_poly->tvlist[v0].v0 + (curr_poly->tvlist[v1].v0 - curr_poly->tvlist[v0].v0)*t1;

						// compute poly 2 new texture coordinates from split
						u02i = curr_poly->tvlist[v0].u0 + (curr_poly->tvlist[v2].u0 - curr_poly->tvlist[v0].u0)*t2;
						v02i = curr_poly->tvlist[v0].v0 + (curr_poly->tvlist[v2].v0 - curr_poly->tvlist[v0].v0)*t2;

						// write them all at the same time         
						// poly 1
						curr_poly->tvlist[v0].u0 = u01i;
						curr_poly->tvlist[v0].v0 = v01i;

						// poly 2
						temp_poly.tvlist[v0].u0 = u02i;
						temp_poly.tvlist[v0].v0 = v02i;
						temp_poly.tvlist[v1].u0 = u01i;
						temp_poly.tvlist[v1].v0 = v01i;

					} // end if textured


					// finally, we have obliterated our pre-computed normal lengths
					// they need to be recomputed!!!!

					// poly 1 first, in place

					// build u, v
					u = curr_poly->tvlist[v1].v - curr_poly->tvlist[v0].v;
					v = curr_poly->tvlist[v2].v - curr_poly->tvlist[v0].v;

					// compute cross product
					n = u.Cross(v);

					// compute length of normal accurately and store in poly nlength
					// +- epsilon later to fix over/underflows
					curr_poly->nlength = n.LengthFast();

					// now poly 2, temp_poly
					// build u, v
					u = temp_poly.tvlist[v1].v - temp_poly.tvlist[v0].v;
					v = temp_poly.tvlist[v2].v - temp_poly.tvlist[v0].v;

					// compute cross product
					n = u.Cross(v);

					// compute length of normal accurately and store in poly nlength
					// +- epsilon later to fix over/underflows
					temp_poly.nlength = n.LengthFast();

					// now we are good to go, insert the polygon into list
					// if the poly won't fit, it won't matter, the function will
					// just return 0
					Insert(temp_poly);

				} // end else

			} // end if near_z clipping has occured

		} // end if z planes

	} // end for poly
}

}
