#version 330 core

layout ( location = 0 ) out vec4 fragColor;

uniform sampler2D depthTex;
uniform float zNear;
uniform float zFar;

in vec2 uv;

void main()
{
	float z = texture( depthTex, uv ).r;
	float linearZ = ( 2 * zNear ) / ( zFar + zNear - z * ( zFar - zNear ) );

	fragColor = vec4( vec3( linearZ ), 1.0 );
}