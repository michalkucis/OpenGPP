uniform sampler2D tex;
uniform sampler2D texVignetting;

varying vec2 samplePos;

void main ()  
{
	vec4 color = texture2D(tex, gl_TexCoord[0].st);
	vec4 v = texture2D(texVignetting, samplePos.st);
	vec4 rgba = v*color;
	float grayscale = (rgba.x + rgba.y + rgba.z) / 3;	
	gl_FragColor.x = grayscale;
	gl_FragColor.y = gl_FragColor.z = 0;
}