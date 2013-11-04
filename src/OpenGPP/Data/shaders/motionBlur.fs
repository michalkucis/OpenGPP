#version 130

uniform sampler2D tex;
uniform sampler2D depth;

uniform uint numSamples;
uniform vec3 deltaPos;
uniform vec2 deltaView;

void main ()  
{
    float depth = texture(depth, gl_TexCoord[0].st).x;
    vec2 pos = gl_TexCoord[0].st * 2.0 - 1.0;

	vec2 velocity = vec2(0.0, 0.0);
    velocity += deltaPos.st;
    velocity += deltaView.st / depth;
    velocity += pos * (1.0-depth/(depth+deltaPos.z));

    float divider = numSamples;

	vec2 texOffset = velocity / vec2(divider-1);
	vec2 texCoord = gl_TexCoord[0].st;
	vec4 finalColor = vec4(0,0,0,0);
	for (int i = 0; i < int(numSamples); i++)
	{
		vec4 currentColor = texture(tex, texCoord);
		finalColor += currentColor;
        texCoord += texOffset;
	}
	gl_FragColor = finalColor / divider;
}