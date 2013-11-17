#include "Light.h"

#include "defines.h"

namespace t3d {

//////////////////////////////////////////////////////////////////////////
// class LightsMgr
//////////////////////////////////////////////////////////////////////////

void LightsMgr::Reset()
{
	// this function simply resets all lights in the system
	static int first_time = 1;

	memset(lights, 0, MAX_LIGHTS * sizeof(Light));

	// reset number of lights
	num_lights = 0;

	// reset first time
	first_time = 0;
}

void LightsMgr::Init(int           index,      // index of light to create (0..MAX_LIGHTS-1)
					 int          _state,      // state of light
					 int          _attr,       // type of light, and extra qualifiers
					 Color        _c_ambient,  // ambient light intensity
					 Color        _c_diffuse,  // diffuse light intensity
					 Color        _c_specular, // specular light intensity
					 const vec4*  _pos,        // position of light
					 const vec4*  _dir,        // direction of light
					 float        _kc,         // attenuation factors
					 float        _kl, 
					 float        _kq, 
					 float        _spot_inner, // inner angle for spot light
					 float        _spot_outer, // outer angle for spot light
					 float        _pf)        // power factor/falloff for spot lights
{
	// this function initializes a light based on the flags sent in _attr, values that
	// aren't needed are set to 0 by caller

	// make sure light is in range 
	if (index < 0 || index >= MAX_LIGHTS)
		return;

	// all good, initialize the light (many fields may be dead)
	lights[index].state       = _state;      // state of light
	lights[index].id          = index;       // id of light
	lights[index].attr        = _attr;       // type of light, and extra qualifiers

	lights[index].c_ambient   = _c_ambient;  // ambient light intensity
	lights[index].c_diffuse   = _c_diffuse;  // diffuse light intensity
	lights[index].c_specular  = _c_specular; // specular light intensity

	lights[index].kc          = _kc;         // constant, linear, and quadratic attenuation factors
	lights[index].kl          = _kl;   
	lights[index].kq          = _kq;   

	if (_pos)
		lights[index].pos = *_pos;	// position of light

	if (_dir)
	{
		lights[index].dir = *_dir;	// direction of light
		lights[index].dir.Normalize();
	}

	lights[index].spot_inner  = _spot_inner; // inner angle for spot light
	lights[index].spot_outer  = _spot_outer; // outer angle for spot light
	lights[index].pf          = _pf;         // power factor/falloff for spot lights

	++num_lights;
}

void LightsMgr::Transform(const mat4& mt, int coord_select)
{
	// this function simply transforms all of the lights by the transformation matrix
	// this function is used to place the lights in camera space for example, so that
	// lighting calculations are correct if the lighting function is called AFTER
	// the polygons have already been trasformed to camera coordinates
	// also, later we may decided to optimize this a little by determining
	// the light type and only rotating what is needed, however there are thousands
	// of vertices in a scene and not rotating 10 more points isn't going to make 
	// a difference
	// NOTE: This function MUST be called even if a transform to the lights 
	// is not desired, that is, you are lighting in world coords, in this case
	// the local light positions and orientations MUST still be copied into the 
	// working variables for the lighting engine to use pos->tpos, dir->tdir
	// hence, call this function with TRANSFORM_COPY_LOCAL_TO_TRANS 
	// with a matrix of NULL

	mat4 mr;		// used to build up rotation aspect of matrix only

	// we need to rotate the direction vectors of the lights also, but
	// the translation factors must be zeroed out in the matrix otherwise
	// the results will be incorrect, thus make a copy of the matrix and zero
	// the translation factors

	mr = mt;
	mr.c[3][0] = mr.c[3][1] = mr.c[3][2] = 0;

	// what coordinates should be transformed?
	switch(coord_select)
	{
	case TRANSFORM_COPY_LOCAL_TO_TRANS:
		{
			// loop thru all the lights
			for (size_t curr_light = 0; curr_light < num_lights; curr_light++)
			{  
				lights[curr_light].tpos = lights[curr_light].pos;
				lights[curr_light].tdir = lights[curr_light].dir;
			} // end for

		} 
		break;

	case TRANSFORM_LOCAL_ONLY:
		{
			// loop thru all the lights
			for (size_t curr_light = 0; curr_light < num_lights; curr_light++)
			{   
				// transform the local/world light coordinates in place
				vec4 presult; // hold result of each transformation

				// transform point position of each light
				lights[curr_light].pos = mt * lights[curr_light].pos;

				// transform direction vector 
				lights[curr_light].dir = mr * lights[curr_light].dir;

			} // end for

		} 
		break;

	case TRANSFORM_TRANS_ONLY:
		{
			// loop thru all the lights
			for (size_t curr_light = 0; curr_light < num_lights; curr_light++)
			{ 
				// transform each "transformed" light
				vec4 presult; // hold result of each transformation

				// transform point position of each light
				lights[curr_light].tpos = mt * lights[curr_light].tpos;

				// transform direction vector 
				lights[curr_light].tdir = mr * lights[curr_light].tdir;

			} // end for

		} 
		break;

	case TRANSFORM_LOCAL_TO_TRANS:
		{
			// loop thru all the lights
			for (size_t curr_light = 0; curr_light < num_lights; curr_light++)
			{ 
				// transform each local/world light and place the results into the transformed
				// storage, this is the usual way the function will be called
				vec4 presult; // hold result of each transformation

				// transform point position of each light
				lights[curr_light].tpos = mt * lights[curr_light].pos;

				// transform direction vector 
				lights[curr_light].tdir = mr * lights[curr_light].dir;
			} // end for

		} break;

	default: break;

	} // end switch
}

}

