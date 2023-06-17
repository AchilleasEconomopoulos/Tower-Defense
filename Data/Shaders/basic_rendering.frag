#version 330 core
layout(location = 0) out vec4 out_color;

uniform sampler2D uniform_texture;
uniform vec3 uniform_color;

in vec2 f_texcoord;


void main(void) 
{	
	vec4 color;

	if(uniform_color.r == 0.0 && uniform_color.g == 0.0 && uniform_color.b == 0.0)
		color= texture(uniform_texture, f_texcoord);
	else
		color = vec4(uniform_color,0.5);
	
	out_color = vec4(color.rgb, 0.5);
}

