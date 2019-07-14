#version 100

attribute vec3 vertex_position;
attribute vec2 vertex_uv;

uniform mat4 projection;
uniform mat4 camera_projection;
uniform vec3 camera_components[2]; // 0 = Target, 1 = Origin

varying lowp vec2 uv;
varying lowp vec3 vertex;
varying lowp vec3 camera;

void main()
{
	uv = vertex_uv;
	vertex = vertex_position;
	camera = camera_components[1];

	gl_Position = (projection * camera_projection) * vec4(vertex_position, 1.0);
}
