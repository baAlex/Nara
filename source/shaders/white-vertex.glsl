#version 100

attribute vec3 vertex_position;
attribute vec4 vertex_color;

uniform mat4 projection;
uniform mat4 camera_projection;
uniform vec3 camera_components[2]; // 0 = Target, 1 = Origin

varying lowp vec4 color ;

void main()
{
	color = vertex_color;
	gl_Position = (projection * camera_projection) * vec4(vertex_position, 1.0);
}
