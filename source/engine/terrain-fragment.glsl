#version 100

uniform lowp vec3 highlight;

uniform sampler2D texture0; // Lightmap
uniform sampler2D texture1; // Low fidelity (?)
uniform sampler2D texture2; // Window
uniform sampler2D texture3; // Masks
uniform sampler2D texture4; // Details

varying lowp vec2 uv;
varying lowp vec2 uv_detail;
varying lowp vec2 uv_window;
varying lowp float window_factor;
varying lowp float fog_factor;

const lowp vec4 fog_color = vec4(0.82, 0.85, 0.87, 1.0);


lowp vec4 LazyMultiplication(lowp vec4 a, lowp vec4 b)
{
	return (1.0 + (a - 0.5) * 2.0) * b;
}

lowp vec4 Overlay(lowp vec4 a, lowp vec4 b)
{
	return mix((1.0 - 2.0 * (1.0 - a) * (1.0 - b)), (2.0 * a * b), step(a, vec4(0.5)));
}

void main()
{
	lowp vec4 mask = texture2D(texture3, uv);
	lowp vec4 detail = texture2D(texture4, uv_detail);

	lowp float final_mask = mix(0.5, detail.r, mask.r);
	final_mask = mix(final_mask, detail.g, mask.g);
	final_mask = mix(final_mask, detail.b, mask.b);
	final_mask = mix(final_mask, detail.a, 1.0 - mask.a);

	gl_FragColor = mix(texture2D(texture2, uv_window),
	                   texture2D(texture1, uv * vec2(4.0, 4.0)), // Just (uv * 1.0) in the completed implementation
	                   window_factor);

	gl_FragColor = Overlay(gl_FragColor, vec4(final_mask));
	gl_FragColor = mix(LazyMultiplication(texture2D(texture0, uv), gl_FragColor), fog_color, fog_factor);
}
