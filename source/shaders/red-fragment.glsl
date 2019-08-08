#version 100

uniform sampler2D color_texture;
uniform lowp vec3 camera_origin;

void main()
{
	gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
