#version 330 core

layout ( location = 0 ) in vec3 vertexPosition;
layout ( location = 1 ) in vec3 vertexNormal;
layout ( location = 2 ) in vec2 vertexTexcoord;

uniform mat4 viewTf = mat4( 1.0 );
uniform mat4 projectionTf = mat4( 1.0 );

uniform vec3 position = vec3( 0.0 );
uniform float intensity = 1.0;

void main()
{
	gl_Position = projectionTf * viewTf * vec4( 0.015 * vertexPosition + position, 1.0 );
}