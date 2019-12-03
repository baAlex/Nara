#version 100

uniform sampler2D texture0; // Normalmap

uniform sampler2D texture1; // Cliff1
uniform sampler2D texture2; // Cliff2
uniform sampler2D texture3; // Grass
uniform sampler2D texture4; // Dirt

uniform lowp vec3 camera_origin;
uniform lowp vec3 highlight;

varying lowp vec2 uv;
varying lowp vec3 position;

const lowp vec4 fog_color = vec4(0.80, 0.80, 0.84, 1.0);
const lowp vec3 sun_direction = normalize(vec3(0.7, 0.3, 0.1));
const lowp vec3 sun_color = vec3(1.0, 1.0, 1.0);


lowp vec4 Overlay(lowp vec4 a, lowp vec4 b)
{
	// https://en.wikipedia.org/wiki/Blend_modes#Overlay

	return mix((1.0 - 2.0 * (1.0 - a) * (1.0 - b)),
	           (2.0 * a * b),
	           step(a, vec4(0.5)));
}


lowp float BrightnessContrast(lowp float value, lowp float brightness, lowp float contrast)
{
	return (value - 0.5) * contrast + 0.5 + brightness;
}


lowp vec3 BrightnessContrast(lowp vec3 value, lowp float brightness, lowp float contrast)
{
	return (value - 0.5) * contrast + 0.5 + brightness;
}


void main()
{
	// Protip: I have no idea what I did here, well
	// except for trial and error on every single line.

	lowp vec4 combined_map = texture2D(texture0, uv);
	lowp vec3 light = sun_color * dot(vec3(combined_map.x, combined_map.y, 1.0), sun_direction);

	lowp vec4 slope = mix(texture2D(texture1, uv * vec2(16.0, 16.0)), texture2D(texture2, uv * vec2(16.0, 16.0)),
	                  smoothstep(0.0, 150.0, position.z));

	lowp vec4 plain = mix(texture2D(texture3, uv * vec2(16.0, 16.0)), texture2D(texture4, uv * vec2(16.0, 16.0)),
	                  smoothstep(25.0, 100.0, position.z));

	lowp float fog_factor = distance(position, camera_origin) / 1024.0;
	fog_factor = pow(fog_factor, 2.0);
	fog_factor = clamp(fog_factor, 0.0, 1.0);

	gl_FragColor = plain;
	gl_FragColor = mix(gl_FragColor, slope, clamp(BrightnessContrast(combined_map.z, -2.5, 32.0), 0.0, 1.0));
	gl_FragColor = gl_FragColor * vec4(BrightnessContrast(light, -0.1, 2.0), 1.0);
	gl_FragColor = mix(gl_FragColor, fog_color, fog_factor);

	// Developers Developers Developers
	// gl_FragColor = diffuse;
	// gl_FragColor = vec4(light, 1.0);
	// gl_FragColor = vec4(BrightnessContrast(light, -0.7, 2.0), 1.0);
}
