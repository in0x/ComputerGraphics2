#version 330 core

out vec4 fragColor;

vec4 color = vec4( 1.0, 0.0, 0.0, 1.0 );

uniform mat4 inverseViewTf;
uniform mat4 viewTf;
uniform vec3 cameraPosition;
uniform float shininess;
uniform vec3 lightColor;
uniform vec3 lightAmbient = vec3( 0.2 );
uniform float apertureAngle;
uniform vec3 lightDirection;
uniform float zNear;
uniform float zFar;
uniform mat4 viewTf_Light;
uniform mat4 projectionTf_Light;
uniform sampler2D depthTex;
uniform float windowDim;
uniform float biasScale;
uniform float maxBias;

in vec3 normalWorld;
in vec3 lightWorld;
in vec4 positionWorld;

int samples = 16;
vec3 offsets[32] = vec3[32](
vec3( -0.0992536, -0.00207359, 0.0120177 ),
vec3( 0.093341, 0.0315738, 0.0216127 ),
vec3( -0.000957316, 0.0951422, 0.0407741 ),
vec3( -0.00479632, -0.106327, 0.0177823 ),
vec3( 0.0700023, 0.073685, 0.0517731 ),
vec3( 0.00485127, 0.0882303, 0.0840786 ),
vec3( 0.0763543, 0.0837204, 0.0670088 ),
vec3( -0.123305, -0.0234142, 0.0686715 ),
vec3( 0.097354, 0.0846403, 0.0881605 ),
vec3( 0.061083, -0.0397367, 0.154908 ),
vec3( -0.00493237, 0.05711, 0.178933 ),
vec3( 0.126304, -0.136723, 0.0890712 ),
vec3( -0.0401913, -0.208017, 0.0802754 ),
vec3( 0.118683, -0.207587, 0.067762 ),
vec3( -0.26656, 0.0280983, 0.0478023 ),
vec3( 0.109515, 0.0579134, 0.270758 ),
vec3( -0.181107, -0.269797, 0.00587253 ),
vec3( 0.224133, -0.12123, 0.245736 ),
vec3( -0.351141, 0.0673086, 0.142176 ),
vec3( -0.185046, -0.253738, 0.274776 ),
vec3( 0.290795, 0.261721, 0.225498 ),
vec3( -0.205179, -0.34272, 0.279635 ),
vec3( -0.232573, 0.115979, 0.456611 ),
vec3( 0.455969, 0.318446, 0.0992143 ),
vec3( -0.56202, 0.140843, 0.178425 ),
vec3( 0.230566, 0.475986, 0.37668 ),
vec3( 0.419862, 0.488046, 0.259534 ),
vec3( 0.298842, -0.170069, 0.656079 ),
vec3( 0.619336, 0.075551, 0.483047 ),
vec3( 0.65447, -0.435033, 0.294286 ),
vec3( 0.406177, -0.526913, 0.592699 ),
vec3( -0.848188, 0.275667, 0.3113 )
);

float PI = 3.1415926535897932384626433832795;

void main()
{
	vec3 color = normalize(normalWorld);
	vec3 cameraPositionWorld = cameraPosition;
	
	vec4 positionScreen = ( projectionTf_Light  * viewTf_Light  * positionWorld);
	vec4 nds = positionScreen / positionScreen.w;
	
	vec2 pixel_pos =  (0.5 * nds.xy + 0.5);
	float depthVal = 0.5 * nds.z + 0.5;
	float texVal = texture(depthTex, pixel_pos).r;
	
	vec3 ambient = color * lightAmbient;
	vec3 diffuse = vec3(0.f,0.f,0.f);
	vec3 specular = vec3(0.f,0.f,0.f);
	
	
	vec3 N = normalize( normalWorld );
	vec3 V = normalize( cameraPositionWorld.xyz - positionWorld.xyz );
	vec3 L = normalize( lightWorld.xyz - positionWorld.xyz );
			
	float bias = max(biasScale*tan(acos(dot(N,L))), maxBias); //wtf
	
	if ( depthVal < texVal + bias) {
		
		if (acos(dot(L,normalize(lightDirection))) < apertureAngle) {
			float dotNL = max( dot( N, L ), 0.0 );
			float phongSpecular = 0.0;
			if ( dotNL > 0.0 )
			{
				vec3 H = normalize( L + V );
				phongSpecular = pow( max( dot( H, N ), 0.0 ), shininess );
			}

			diffuse = dotNL * color * lightColor;
			specular = phongSpecular * color * lightColor;
		}
		
		float hits = 0;
		
		for (int i = 0; i < samples; i++) {
			
			float texValOffset = texture(depthTex, pixel_pos + (offsets[i].xy * 0.04)).r;
			if ( depthVal < texValOffset + bias) hits++;
			
		}
		
		hits /= 32;
		diffuse *= hits;
		specular *= hits;
		
		float rho = dot(L, normalize(lightDirection)); 
		float lightAngleCos = cos(PI/4);
		float aa = clamp((rho - lightAngleCos) / (1 - lightAngleCos),  0.0, 1.0 );
		diffuse *= aa;
		specular *= aa;
	}
	
	fragColor = vec4( clamp( diffuse + specular + ambient, vec3( 0.0 ), vec3( 1.0 ) ), 1.0 );
	
}