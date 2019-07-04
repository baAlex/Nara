#version 100

attribute vec3 vertex_position;

uniform mat4 projection;
uniform vec3 camera[2]; // 0 = Target, 1 = Origin


/*-----------------------------

 LookAt()
-----------------------------*/
mat4 LookAt(vec3 target, vec3 origin)
{
	vec3 f = target - origin;
	f = normalize(f);

	vec3 s = cross(f, vec3(0.0, 0.0, 1.0));
	s = normalize(s);

	vec3 u = cross(s, f);

	return mat4(s.x,
	            u.x,
	            -f.x,
	            0.0,

	            s.y,
	            u.y,
	            -f.y,
	            0.0,

	            s.z,
	            u.z,
	            -f.z,
	            0.0,

	            (s.x * (-origin.x) + s.y * (-origin.y) + s.z * (-origin.z)),
	            (u.x * (-origin.x) + u.y * (-origin.y) + u.z * (-origin.z)),
	            ((-f.x) * (-origin.x) + (-f.y) * (-origin.y) + (-f.z) * (-origin.z)),
	            1.0);
}


/*-----------------------------

 main()
-----------------------------*/
void main()
{
	gl_Position = (projection * LookAt(camera[0], camera[1])) * vec4(vertex_position, 1.0);
}
