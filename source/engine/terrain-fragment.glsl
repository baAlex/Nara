#version 100

uniform sampler2D color_texture;
uniform lowp vec3 camera_origin;
uniform lowp vec3 highlight;

varying lowp vec2 uv;
varying lowp vec3 position;

const lowp vec4 fog_color = vec4(0.80, 0.82, 0.84, 1.0);

void main()
{
	lowp float fog_factor = distance(position, camera_origin) / 1024.0;
	fog_factor = pow(fog_factor, 2.0);

	gl_FragColor = mix(texture2D(color_texture, uv) + vec4(highlight, 1.0), fog_color, clamp(fog_factor, 0.0, 1.0));

	//gl_FragColor = texture2D(color_texture, uv);
	//gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
