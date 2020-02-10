#
# MIT License
#
# Copyright (c) 2019 Alexander Brandt
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


# [common.rb]
# - Alexander Brandt 2019-2020

class Vector3

	attr_accessor :x
	attr_accessor :y
	attr_accessor :z

	def initialize(x = 0.0, y = 0.0, z = 0.0)
		@x = x
		@y = y
		@z = z
	end

	def copy()
		return Vector3.new(@x, @y, @z)
	end

	def add(vector)
		@x += vector.x
		@y += vector.y
		@z += vector.z
		return self
	end

	def subtract(vector)
		@x -= vector.x
		@y -= vector.y
		@z -= vector.z
		return self
	end

	def multiply(vector)
		@x *= vector.x
		@y *= vector.y
		@z *= vector.z
		return self
	end

	def divide(vector)
		@x /= vector.x
		@y /= vector.y
		@z /= vector.z
		return self
	end

	def invert()
		@x = -@x
		@y = -@y
		@z = -@z
		return self
	end

	def scale(magnitude)
		@x *= magnitude
		@y *= magnitude
		@z *= magnitude
		return self
	end
end


class Entity

	def initialize()
		@position = Vector3.new()
		@angle = Vector3.new()
	end

	def think()
	end
end


def max(a, b)
	return (a > b) ? a : b
end


def min(a, b)
	return (a < b) ? a : b
end


def abs(x)
	return (x > 0.0) ? x : -x
end


def clamp(min_val, max_val, x)
	min(max(x, min_val), max_val)
end


def deg_to_rad(x)
	return x * Nara::PI / 180.0
end


def rad_to_deg(x)
	return x * 180.0 / Nara::PI
end


def vector_axes(angle)

	# TODO, implement on the C side
	# x = Pith
	# y = Roll
	# z = Yaw

	cx = Nara.cos(deg_to_rad(angle.x))
	sx = Nara.sin(deg_to_rad(angle.x))
	cy = Nara.cos(deg_to_rad(angle.y)) # TODO: broken... maybe
	sy = Nara.sin(deg_to_rad(angle.y)) # "
	cz = Nara.cos(deg_to_rad(angle.z))
	sz = Nara.sin(deg_to_rad(angle.z))

	forward = Vector3.new(sz * -sx, cz * -sx, -cx)
	left = Vector3.new((sz * sy * sx) + (cy * cz), (cz * sy * sx) + (cy * -sz), sx * sy)
	up = Vector3.new((sz * cy * cx) + -(sy * cz), (cz * cy * cx) + -(sy * -sz), -sx * cy)

	return forward, left, up
end
