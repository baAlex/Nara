#version 100

uniform lowp vec3 camera_origin;
uniform lowp vec3 highlight;

uniform sampler2D texture0; // Lightmap
uniform sampler2D texture1;
uniform sampler2D texture2;

varying lowp vec3 position;
varying lowp vec2 uv;

varying lowp float pink_factor;
varying lowp float fog_factor;

const lowp vec4 fog_color = vec4(0.82, 0.85, 0.87, 1.0);


lowp vec4 LazyMultiplication(lowp vec4 a, lowp vec4 b)
{
	return (1.0 + (a - 0.5) * 2.0) * b;
}


void main()
{
	gl_FragColor = mix(texture2D(texture2, uv * vec2(16.0, 16.0)), texture2D(texture1, uv * vec2(16.0, 16.0)), pink_factor);
	gl_FragColor = mix(LazyMultiplication(texture2D(texture0, uv), gl_FragColor), fog_color, fog_factor);
}
