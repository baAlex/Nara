#version 100

uniform sampler2D color_texture;
varying lowp vec2 uv;

void main()
{
	gl_FragColor = texture2D(color_texture, uv);
}
