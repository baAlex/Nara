#version 100

attribute vec3 vertex_position;
attribute vec2 vertex_uv;

uniform mat4 projection;
uniform mat4 camera_projection;
uniform lowp vec2 highlight;

varying lowp vec2 uv;
varying lowp vec3 vertex;
varying lowp vec4 is_border;

void main()
{
	uv = vertex_uv;
	vertex = vertex_position;

	// Because there are no off/on state except for the coordinates themself,
	// vertices on positions x:0 and y:0 always falls as being part of a border
	if(vertex_position.x == highlight.x ||vertex_position.y == highlight.y)
		is_border = vec4(1.0, 0.0, 0.0, 1.0);
	else
		is_border = vec4(0.0, 0.0, 0.0, 0.0);

	gl_Position = (projection * camera_projection) * vec4(vertex_position, 1.0);
}
