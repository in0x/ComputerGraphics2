#pragma once

#include "../../common/glm.h"

inline float randFloat( float minval = 0.f, float maxval = 1.f )
{
	float r = rand() / static_cast< float >( RAND_MAX );	// [0, 1]
	return minval + r*(maxval-minval);								// remap range [0, 1] to [min, max]
}

inline glm::vec3 randVec3( glm::vec3 min = glm::vec3( -1.f ), glm::vec3 max = glm::vec3( 1.f ) )
{
	glm::vec3 p;
	p.x = randFloat( min.x, max.x );
	p.y = randFloat( min.y, max.y );
	p.z = randFloat( min.z, max.z );
	return p;
}

inline glm::vec3 randRGBColor( float s, float v )
{
	float h = rand() / static_cast< float >( RAND_MAX );
	glm::vec4 K( 1.f, 2.f / 3.f, 1.f / 3.f, 3.f );
	glm::vec3 p = glm::abs( glm::fract( glm::vec3( h, h, h ) + glm::vec3( 1.f, 2.f / 3.f, 1.f / 3.f ) ) * 6.f - glm::vec3( 3.f ) );
	return v * glm::mix( glm::vec3( 1.f ), glm::clamp( p - glm::vec3( 1.f ), 0.f, 1.f ), s );
	//return glm::vec3( 1.f, 0.f, 0.f );
}