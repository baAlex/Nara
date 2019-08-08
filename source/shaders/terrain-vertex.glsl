#version 100

attribute vec3 vertex_position;
attribute vec2 vertex_uv;

uniform mat4 projection;
uniform mat4 camera_projection;

varying lowp vec2 uv;
varying lowp vec3 vertex;

void main()
{
	uv = vertex_uv;
	vertex = vertex_position;

	gl_Position = (projection * camera_projection) * vec4(vertex_position, 1.0);
}
