#version 100

uniform sampler2D color_texture;

varying lowp vec2 uv;
varying lowp vec3 vertex;
varying lowp vec3 camera;

const lowp vec4 fog_color = vec4(0.80, 0.82, 0.84, 1.0);

void main()
{
	lowp float fog_factor = distance(vertex, camera) / 20000.0; // 20 km
	gl_FragColor = mix(texture2D(color_texture, uv), fog_color, fog_factor);

	//gl_FragColor = texture2D(color_texture, uv);
	//gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
