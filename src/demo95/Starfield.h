#pragma once

namespace raider3d {

struct Star
{
	unsigned int color;		// color of point 32-bit
	float x,y,z;			// coordinates of point in 3D
};

class Starfield
{
public:

	void Init();

	void Move();

	void Draw() const;

private:
	static const int NUM_STARS = 256;  // number of stars in sim

private:
	Star stars[NUM_STARS];

}; // Starfield

}