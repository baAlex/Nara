#version 100

attribute vec3 vertex_position;
attribute vec2 vertex_uv;

uniform mat4 projection;
uniform mat4 camera_projection;
uniform vec3 highlight;

varying lowp vec2 uv;
varying lowp vec3 position;

void main()
{
	uv = vertex_uv;
	position = vertex_position;

	gl_Position = (projection * camera_projection) * vec4(vertex_position, 1.0);
}
