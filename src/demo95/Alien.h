#pragma once

#include "Vector.h"
#include "Matrix.h"

namespace t3d { class RenderObject; class Camera; class RenderList; }

namespace raider3d {

// states for aliens
#define ALIEN_STATE_DEAD      0  // alien is dead
#define ALIEN_STATE_ALIVE     1  // alien is alive
#define ALIEN_STATE_DYING     2  // alien is in process of dying

class Camera;
class RenderList;

struct Alien
{
	int  state;  // state of this ALIEN fighter
	int  count;  // generic counter
	int  aux;    // generic auxialiary var
	vec3 pos;    // position of ALIEN fighter
	vec3 vel;    // velocity of ALIEN fighter
	vec3 rot;    // rotational velocities
	vec3 ang;    // current orientation
};

class RenderObject;

class AlienList
{
public:
	AlienList();
	~AlienList();

	void Init();

	int Start();

	void DrawAliens(const t3d::Camera& cam, t3d::RenderList& rlist);

	void DrawExplosions(t3d::RenderList& rlist);

	void Update(const t3d::Camera& cam);

private:
	int StartMeshExplosion(const mat4& mrot,	// initial orientation of object
						   int detail,          // the detail level,1 highest detail
						   float rate,          // velocity of explosion shrapnel
						   int lifetime);

private:
	// alien defines
	static const int NUM_ALIENS = 16; // total number of ALIEN fighters 

	// explosion stuff
	static const int NUM_EXPLOSIONS = 16;

private:
	// alien globals
	Alien aliens[NUM_ALIENS];		// the ALIEN fighters
	t3d::RenderObject* obj_alien;		// the master object

	// explosions
	t3d::RenderObject* explobj[NUM_EXPLOSIONS]; // the explosions

	int explosion_sound_id;

}; // AlienList

}