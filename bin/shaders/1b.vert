#version 330 core

layout ( location = 0 ) in vec3 vertexPosition;
layout ( location = 1 ) in vec3 vertexNormal;

mat4 modelTf = mat4( 1.0 );
mat4 viewTf = mat4( 1.0 );
mat4 projectionTf = mat4( 1.0 );

out vec3 normal;

void main()
{
	normal = vertexNormal;
	gl_Position = vec4( vertexPosition, 1.0 ); //projectionTf * viewTf * modelTf * 
}