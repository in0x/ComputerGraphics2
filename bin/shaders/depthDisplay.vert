#version 330 core

layout ( location = 0 ) in vec3 vertexPosition;

uniform mat4 mvp;

out vec2 uv;

void main()
{
	uv = 0.5 * vertexPosition.xy + vec2( 0.5 );
	gl_Position = mvp * vec4( vertexPosition, 1.0 );
}