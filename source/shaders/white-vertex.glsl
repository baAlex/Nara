#version 100

attribute vec3 vertex_position;

uniform mat4 projection;
uniform mat4 camera_projection;
uniform vec3 camera_components[2]; // 0 = Target, 1 = Origin

void main()
{
	gl_Position = (projection * camera_projection) * vec4(vertex_position, 1.0);
}
