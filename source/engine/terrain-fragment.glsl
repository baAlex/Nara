#version 100

uniform sampler2D texture0;
uniform sampler2D texture1;

uniform lowp vec3 camera_origin;
uniform lowp vec3 highlight;

varying lowp vec2 uv;
varying lowp vec3 position;

const lowp vec4 fog_color = vec4(0.80, 0.82, 0.84, 1.0);


lowp vec4 blend(lowp vec4 a, lowp vec4 b)
{
	// https://en.wikipedia.org/wiki/Blend_modes#Overlay

    return mix((1.0 - 2.0 * (1.0 - a) * (1.0 - b)),
	           (2.0 * a * b),
	           step(a, vec4(0.5)));
}

void main()
{
	lowp float fog_factor = distance(position, camera_origin) / 1024.0;
	fog_factor = pow(fog_factor, 2.0);

	lowp vec4 texture_overlay = blend(texture2D(texture0, uv), texture2D(texture1, uv * vec2(256.0, 256.0)));

	gl_FragColor = mix(texture_overlay + vec4(highlight, 1.0), fog_color, clamp(fog_factor, 0.0, 1.0));

	//gl_FragColor = texture2D(color_texture, uv);
	//gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
