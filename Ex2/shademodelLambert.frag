#version 330 core

layout ( location = 0 ) out vec4 fragColor;

struct LightSource
{
    vec4 position_eyeSpace;
	vec4 position_worldSpace;
    vec3 color;
	float intensity;
};

const int MAX_LIGHTS = 10;
const float PI = 3.14159265359;

uniform LightSource lights[MAX_LIGHTS];
uniform mat4 modelViewTf;
uniform int lightCount;
uniform vec3 ambientColor;
uniform vec3 ambientIntensity;
uniform sampler2D diffuseTex;
uniform sampler2D aoTex;

in vec4 position_eyeSpace;
in vec3 normal_eyeSpace;
in vec3 k_diffuse;
in vec2 uv;

void main()
{
	//E = I * max( dot( N, L ), 0 ) / d^2
	
	vec3 colorDiff = texture(diffuseTex, uv).rgb;
	float ao = texture(aoTex, uv).r;
	
	vec3 R_sum;
	
	for (int i = 0; i < lightCount; i++) {
		float distance = length(position_eyeSpace.xyz - lights[i].position_eyeSpace.xyz);
		vec3 N = normalize(normal_eyeSpace);
		vec3 L = normalize(lights[i].position_eyeSpace.xyz - position_eyeSpace.xyz);
		float dotNL = max( dot(N, L), 0);
		float E = lights[i].intensity * dotNL / (distance * distance);
		vec3 Radiance = E * colorDiff; //* (1 / PI);
		R_sum = R_sum + (Radiance * lights[i].color);
	}
	
	fragColor = vec4( ao * R_sum + (ambientColor * ambientIntensity ), 1); //ambientColor * ambientIntensity
}