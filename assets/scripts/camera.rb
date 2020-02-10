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


# [camera.rb]
# - Alexander Brandt 2019-2020

class Camera < Entity

	ANALOG_DEAD_ZONE = 0.2
	MOVEMENT_SPEED = 2.5
	LOOK_SPEED = 3.0

	def think()

		forward, left, up = vector_axes(@angle)

		# Forward, backward
		if abs(Nara::Input.la_v) > ANALOG_DEAD_ZONE then
			@position.add(forward.scale(Nara::Input.la_v * Nara.delta * MOVEMENT_SPEED * -1.0))
		end

		# Stride left, right
		if abs(Nara::Input.la_h) > ANALOG_DEAD_ZONE then
			@position.add(left.scale(Nara::Input.la_h * Nara.delta * MOVEMENT_SPEED))
		end

		# Down
		if abs(Nara::Input.la_t) > ANALOG_DEAD_ZONE then
			temp = up.copy()
			@position.add(temp.scale(Nara::Input.la_t * Nara.delta * MOVEMENT_SPEED * -1.0))
		end

		# Up
		if abs(Nara::Input.ra_t) > ANALOG_DEAD_ZONE then
			temp = up.copy()
			@position.add(temp.scale(Nara::Input.ra_t * Nara.delta * MOVEMENT_SPEED))
		end

		# Look up, down
		if abs(Nara::Input.ra_v) > ANALOG_DEAD_ZONE then
			@angle.x += Nara::Input.ra_v * Nara.delta * LOOK_SPEED
			@angle.x = clamp(-180.0, 0.0, @angle.x)
		end

		# Look left, right
		if abs(Nara::Input.ra_h) > ANALOG_DEAD_ZONE then
			@angle.z += Nara::Input.ra_h * Nara.delta * LOOK_SPEED
		end
	end
end
