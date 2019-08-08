#version 100

uniform sampler2D color_texture;
uniform lowp vec3 camera_origin;

varying lowp vec2 uv;
varying lowp vec3 vertex;

const lowp vec4 fog_color = vec4(0.80, 0.82, 0.84, 1.0);

void main()
{
	lowp float fog_factor = distance(vertex, camera_origin) / 1024.0;
	fog_factor = pow(fog_factor, 4.0);

	gl_FragColor = mix(texture2D(color_texture, uv), fog_color, clamp(fog_factor, 0.0, 1.0));
	//gl_FragColor = texture2D(color_texture, uv);
	//gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
