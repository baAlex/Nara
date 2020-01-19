#version 100

uniform sampler2D texture0; // Lightmap
uniform sampler2D texture1; // Masksmap

uniform sampler2D texture2; // Grass
uniform sampler2D texture3; // Dirt
uniform sampler2D texture4; // Cliff1
uniform sampler2D texture5; // Cliff2

uniform lowp vec3 camera_origin;
uniform lowp vec3 highlight;

varying lowp vec2 uv;
varying lowp vec3 position;

const lowp vec4 fog_color = vec4(0.82, 0.85, 0.87, 1.0);


/*lowp vec4 Overlay(lowp vec4 a, lowp vec4 b)
{
	// https://en.wikipedia.org/wiki/Blend_modes#Overlay
	// http://www.simplefilter.de/en/basics/mixmods.html

	return mix((1.0 - 2.0 * (1.0 - a) * (1.0 - b)), (2.0 * a * b),
	           step(a, vec4(0.5)));
}

lowp vec4 Hardlight(lowp vec4 a, lowp vec4 b)
{
	return mix((1.0 - 2.0 * (1.0 - a) * (1.0 - b)), (2.0 * a * b),
	           step(b, vec4(0.5)));
}*/

lowp vec4 LazyMultiplication(lowp vec4 a, lowp vec4 b)
{
	return (1.0 + (a - 0.5) * 2.0) * b;
}

/*lowp float BrightnessContrast(lowp float value, lowp float brightness, lowp float contrast)
{
	return (value - 0.5) * contrast + 0.5 + brightness;
}

lowp vec3 BrightnessContrast(lowp vec3 value, lowp float brightness, lowp float contrast)
{
	return (value - 0.5) * contrast + 0.5 + brightness;
}*/


void main()
{
	// Lightmap
	lowp vec4 lightmap = texture2D(texture0, uv);

	// Plains (grass + dirt)
	lowp vec4 plains = mix(texture2D(texture2, uv * vec2(16.0, 16.0)), texture2D(texture3, uv * vec2(16.0, 16.0)),
	                       smoothstep(25.0, 100.0, position.z));

	// Cliffs
	lowp vec4 cliffs = mix(texture2D(texture4, uv * vec2(16.0, 16.0)), texture2D(texture5, uv * vec2(16.0, 16.0)),
	                       smoothstep(25.0, 100.0, position.z));

	// Fog
	lowp float fog_factor = distance(position, camera_origin) / 1024.0;
	fog_factor = pow(fog_factor, 2.0);
	fog_factor = clamp(fog_factor, 0.0, 1.0);

	// Combine everything
	gl_FragColor = mix(plains, cliffs, texture2D(texture1, uv));
	gl_FragColor = LazyMultiplication(lightmap, gl_FragColor);
	gl_FragColor = mix(gl_FragColor, fog_color, fog_factor);
}
