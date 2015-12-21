#version 330 core

layout ( location = 0 ) out vec4 fragColor;

in vec3 color;
in vec3 normal_eyeSpace;
in vec4 position_eyeSpace;

void main()
{
	float fogDistance_eyeSpace = -5.0;									// hardcoded fog distance in eye space ( beyond this, no fragments should be visible )
	vec3 fogColor = vec3( 1.0 );										// hardcoded fog color

	float depth_eyeSpace = position_eyeSpace.z;							// this value is negative in eye space, just like the fog distance above
	float fogFactor = 1.0 - depth_eyeSpace / fogDistance_eyeSpace;		// factor for linear interpolation

	// diffuse shading is the simplest shading
	vec3 N = normalize( normal_eyeSpace );
	vec3 L = vec3( 0.0, 0.0, 1.0 );										// hardcoded directional ligh source in eye space
	float NoL = max( dot( N, L ), 0.0 );
	vec3 diffuseColor = clamp( ( NoL + 0.2 ) * color.xyz, 0.0, 1.0 );

	fragColor = vec4( mix( fogColor, diffuseColor, fogFactor ), 1.0 );
}