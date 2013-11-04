uniform sampler2D texColor;
uniform sampler2D texDepth;

void main ()  
{
	vec4 color = texture2D(texDepth, gl_TexCoord[0].st);
	gl_FragColor = color;
}