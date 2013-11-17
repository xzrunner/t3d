#pragma once

#include "Vector.h"

namespace t3d {

class Plane
{
public:

	void Assign(const vec3& p0, const vec3& n) {
		this->p0 = p0;
		this->n = n.Normalized();
	}

private:
	vec3 p0; // point on the plane
	vec3 n; // normal to the plane (not necessarily a unit vector)

}; // Plane

}