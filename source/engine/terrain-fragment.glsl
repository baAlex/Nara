#version 100

uniform lowp vec3 camera_origin;
uniform lowp vec3 highlight;

uniform sampler2D texture0; // Lightmap
uniform sampler2D texture1; // Masksmap
uniform sampler2D texture2; // Grass
uniform sampler2D texture3; // Dirt
uniform sampler2D texture4; // Cliff1
uniform sampler2D texture5; // Cliff2

varying lowp vec3 position;
varying lowp vec2 uv;
varying lowp float fog_factor;

const lowp vec4 fog_color = vec4(0.82, 0.85, 0.87, 1.0);


lowp vec4 LazyMultiplication(lowp vec4 a, lowp vec4 b)
{
	return (1.0 + (a - 0.5) * 2.0) * b;
}

lowp float HeightBased(lowp float edge0, lowp float edge1, lowp float h1, lowp float h2, lowp float x)
{
	lowp float f = (x - edge0) / (edge1 - edge0);

	f = f - (h1) * (1.0 - f) + (h2) * (f);

	return clamp(f, 0.0, 1.0);
}

void main()
{
	lowp vec4 a;
	lowp vec4 b;
	lowp float factor;

	// Plains (grass + dirt)
	a = texture2D(texture2, uv * vec2(16.0, 16.0));
	b = texture2D(texture3, uv * vec2(16.0, 16.0));

	factor = HeightBased(-50.0, 200.0, a.w, b.w, position.z);
	lowp vec4 plains = mix(vec4(a.rgb, 1.0), vec4(b.rgb, 1.0), factor);

	// Cliffs
	a = texture2D(texture4, uv * vec2(16.0, 16.0));
	b = texture2D(texture5, uv * vec2(16.0, 16.0));

	factor = HeightBased(50.0, 100.0, a.w, b.w, position.z);
	lowp vec4 cliffs = mix(vec4(a.rgb, 1.0), vec4(b.rgb, 1.0), position.z/150.0);

	// Combine everything
	gl_FragColor = mix(plains, cliffs, texture2D(texture1, uv));
	gl_FragColor = LazyMultiplication(texture2D(texture0, uv), gl_FragColor);
	gl_FragColor = mix(gl_FragColor, fog_color, fog_factor);
}
