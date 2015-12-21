#pragma once

#include "../../common/opengl.h"
#include "../../common/glm.h"

#include <vector>
#include <memory>

struct Mesh
{
	GLuint handleToVertexArrayObject = 0;
	GLuint handleToAttribBufferObject = 0;
	GLuint handleToIndexBufferObject = 0;

	unsigned int indexCount = 0;
	unsigned int vertexCount = 0;
};

struct VertexData
{
	glm::vec3 position;
	glm::vec3 normal;
};

struct MeshData
{
	std::vector< VertexData >	attribData;
	std::vector< unsigned >		indexData;
};

inline std::shared_ptr< MeshData > computeUVSphere( const float radius, const unsigned int eSegs, const unsigned int aSegs )
{
	std::shared_ptr< MeshData > result( new MeshData );

	result->attribData.reserve( ( eSegs+1 )*aSegs );
	result->indexData.reserve( 3*2*eSegs*aSegs );

	const float aInc = 2 * glm::pi< float >() / aSegs;
	const float eInc = glm::pi< float >() / eSegs;

	float elevation = 0.f;
	for (unsigned int e = 0; e <= eSegs; ++e)
	{
		float azimuth = 0.f;
		for (unsigned int a = 0; a<aSegs; ++a)
		{
			VertexData vertex;
			vertex.position = radius * glm::vec3( glm::sin( elevation ) * glm::cos( azimuth ), glm::sin( elevation ) * glm::sin( azimuth ), glm::cos( elevation ) );
			vertex.normal = glm::normalize( vertex.position );
			result->attribData.push_back( vertex );

			if ( e == eSegs ) continue;

			unsigned int i0 = e*aSegs + a;
			unsigned int i1 = e*aSegs + (a + 1)%aSegs;
			unsigned int i2 = (e + 1)*aSegs + a;
			unsigned int i3 = (e + 1)*aSegs + (a + 1)%aSegs;

			result->indexData.push_back( i0 ); result->indexData.push_back( i2 ); result->indexData.push_back( i3 );
			result->indexData.push_back( i0 ); result->indexData.push_back( i3 ); result->indexData.push_back( i1 );

			azimuth += aInc;
		}

		elevation += eInc;
	}

	return result;
};

inline std::shared_ptr< Mesh > uploadToGPU( std::shared_ptr< MeshData > data )
{
	std::shared_ptr< Mesh > result( new Mesh );
	result->indexCount = data->indexData.size();
	result->vertexCount = data->attribData.size();

	glGenBuffers( 1, std::addressof( result->handleToIndexBufferObject ) );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, result->handleToIndexBufferObject );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( unsigned ) * data->indexData.size(), reinterpret_cast< GLvoid const* >( data->indexData.data() ), GL_STATIC_DRAW );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	glGenBuffers( 1, std::addressof( result->handleToAttribBufferObject ) );
	glBindBuffer( GL_ARRAY_BUFFER, result->handleToAttribBufferObject );
	glBufferData( GL_ARRAY_BUFFER, sizeof( VertexData ) * data->attribData.size(), reinterpret_cast< GLvoid const* >( data->attribData.data() ), GL_STATIC_DRAW );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	return result;
};

inline std::shared_ptr< Mesh > createSphereMesh()
{
	auto meshdata = computeUVSphere( 1.f, 61, 30 );
	auto mesh = uploadToGPU( meshdata );

	glGenVertexArrays( 1, std::addressof( mesh->handleToVertexArrayObject ) );
	glBindVertexArray( mesh->handleToVertexArrayObject );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh->handleToIndexBufferObject );
	glBindBuffer( GL_ARRAY_BUFFER, mesh->handleToAttribBufferObject );
	glVertexAttribPointer( 0, 3, GL_FLOAT, false, sizeof( VertexData ), reinterpret_cast< GLvoid const* >( offsetof( VertexData, position ) ) );
	glVertexAttribPointer( 1, 3, GL_FLOAT, false, sizeof( VertexData ), reinterpret_cast< GLvoid const* >( offsetof( VertexData, normal ) ) );
	glEnableVertexAttribArray( 0 );
	glEnableVertexAttribArray( 1 );
	glBindVertexArray( 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glDisableVertexAttribArray( 0 );
	glDisableVertexAttribArray( 1 );

	return mesh;
};

inline void cleanUpMesh( std::shared_ptr< Mesh > m )
{
	glDeleteBuffers( 1, std::addressof( m->handleToIndexBufferObject ) );
	glDeleteBuffers( 1, std::addressof( m->handleToAttribBufferObject ) );

	glDeleteVertexArrays( 1, std::addressof( m->handleToVertexArrayObject ) );
};