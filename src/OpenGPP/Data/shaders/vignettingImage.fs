uniform sampler2D tex;
uniform sampler2D texVignetting;
void main ()  
{
	vec4 color = texture2D(tex, gl_TexCoord[0].st);
	vec4 v = texture2D(texVignetting, gl_TexCoord[0].st);
	gl_FragColor = color*v;
}