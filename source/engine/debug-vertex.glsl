#version 100

attribute vec3 vertex_position;

uniform lowp vec3 camera_origin;
uniform mat4 camera_projection;
uniform mat4 projection;

varying lowp float fog_factor;


lowp float LinearStep(lowp float edge0, lowp float edge1, lowp float x)
{
	return clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
}

void main()
{
	fog_factor = LinearStep(256.0, 1024.0, distance(vertex_position, camera_origin));
	gl_Position = (projection * camera_projection) * vec4(vertex_position, 1.0);
}
