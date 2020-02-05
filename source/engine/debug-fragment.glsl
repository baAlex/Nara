#version 100

varying lowp float fog_factor;
const lowp vec4 fog_color = vec4(0.82, 0.85, 0.87, 1.0);


void main()
{
	gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
	gl_FragColor = mix(gl_FragColor, fog_color, fog_factor);
}
