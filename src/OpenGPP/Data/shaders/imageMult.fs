uniform sampler2D tex;
uniform vec3 mult;
void main ()  
{
	vec4 color = texture2D(tex, gl_TexCoord[0].st);
	color.rgb *= mult;
	gl_FragColor = color;
}