#version 330 core

layout ( location = 0 ) out vec4 fragColor;

uniform vec3 color = vec3( 1.0 );
uniform float intensity = 1.0;

void main()
{
	fragColor = vec4( intensity * color, 1.0 );
}