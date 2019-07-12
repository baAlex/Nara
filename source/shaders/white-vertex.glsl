#version 100

attribute vec3 vertex_position;
attribute vec2 vertex_uv;

uniform mat4 projection;
uniform mat4 camera_projection;
uniform vec3 camera_components[2]; // 0 = Target, 1 = Origin

varying lowp vec2 uv ;

void main()
{
	uv = vertex_uv;
	gl_Position = (projection * camera_projection) * vec4(vertex_position, 1.0);
}
