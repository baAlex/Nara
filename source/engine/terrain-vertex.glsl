#version 100

attribute vec3 vertex_position;
attribute vec2 vertex_uv;

uniform lowp vec3 camera_origin;
uniform mat4 camera_projection;
uniform mat4 projection;

varying lowp vec3 position;
varying lowp vec2 uv;

varying lowp float fog_factor;
varying lowp float pink_factor;


lowp float LinearStep(lowp float edge0, lowp float edge1, lowp float x)
{
	return clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
}

void main()
{
	position = vertex_position;
	uv = vertex_uv;

	fog_factor = LinearStep(256.0, 1024.0, distance(vertex_position, camera_origin));
	pink_factor = LinearStep(128.0, 192.0, distance(vertex_position, camera_origin));

	gl_Position = (projection * camera_projection) * vec4(vertex_position, 1.0);
}
