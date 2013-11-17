#pragma once

#include "Vector.h"
#include "Color.h"
#include "Matrix.h"

namespace t3d {

// defines for light types
#define LIGHT_ATTR_AMBIENT      0x0001    // basic ambient light
#define LIGHT_ATTR_INFINITE     0x0002    // infinite light source
#define LIGHT_ATTR_DIRECTIONAL  0x0002    // infinite light source (alias)
#define LIGHT_ATTR_POINT        0x0004    // point light source
#define LIGHT_ATTR_SPOTLIGHT1   0x0008    // spotlight type 1 (simple)
#define LIGHT_ATTR_SPOTLIGHT2   0x0010    // spotlight type 2 (complex)

#define LIGHT_STATE_ON          1         // light on
#define LIGHT_STATE_OFF         0         // light off

class Light
{
public:
	Light() : ptr(0) {}

public:
	int state; // state of light
	int id;    // id of light
	int attr;  // type of light, and extra qualifiers

	Color c_ambient;   // ambient light intensity
	Color c_diffuse;   // diffuse light intensity
	Color c_specular;  // specular light intensity

	vec4  pos;       // position of light
	vec4  tpos;
	vec4  dir;       // direction of light
	vec4  tdir;
	float kc, kl, kq;   // attenuation factors
	float spot_inner;   // inner angle for spot light
	float spot_outer;   // outer angle for spot light
	float pf;           // power factor/falloff for spot lights

	int   iaux1, iaux2; // auxiliary vars for future expansion
	float faux1, faux2;
	void  *ptr;

}; // Light

class LightsMgr
{
public:
	LightsMgr() : num_lights(0) {}

	Light& operator [] (int index) {
		return index < MAX_LIGHTS ? lights[index] : lights[MAX_LIGHTS-1];
	}
	int Size() const { return num_lights; }

	void Reset();

	void Init(int           index,      // index of light to create (0..MAX_LIGHTS-1)
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
			  float        _pf);        // power factor/falloff for spot lights

	void Transform(const mat4& mt,		// transformation matrix
				   int coord_select);   // selects coords to transform);

private:
	static const int MAX_LIGHTS = 8;         // good luck with 1!

private:
	Light lights[MAX_LIGHTS];    // lights in system
	int num_lights;              // current number of lights

}; // LightsMgr

}