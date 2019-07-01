#version 100

attribute vec3 vertex_position;

uniform mat4 projection;
uniform mat4 camera;

void main()
{
	gl_Position = (projection * camera) * vec4(vertex_position, 1.0);
}
