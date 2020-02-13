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
	LOOK_SPEED = 3.0

	WALK_SPEED = km_per_hour(8.0)
	FLY_SPEED = km_per_hour(180.0)

	def initialize()
		super
		@keystate_rb = KeyState.new()
		@first_person = false
		@movement_speed = FLY_SPEED
		@distance = 0.0
	end

	def think()

		temp = @angle

		if @first_person == true then
			temp = @angle.copy
			temp.x = -90.0 # Hack
		end

		forward, left, up = vector_axes(temp)

		# Forward, backward
		if abs(Nara::Input.la_v) > ANALOG_DEAD_ZONE then
			@position.add(forward.scale(Nara::Input.la_v * Nara.delta * @movement_speed * -1.0))
			@distance += abs(Nara::Input.la_v * Nara.delta * @movement_speed) # Hack
		end

		# Stride left, right
		if abs(Nara::Input.la_h) > ANALOG_DEAD_ZONE then
			@position.add(left.scale(Nara::Input.la_h * Nara.delta * @movement_speed))
			@distance += abs(Nara::Input.la_h * Nara.delta * @movement_speed) # Hack
		end

		# Down
		if abs(Nara::Input.la_t) > ANALOG_DEAD_ZONE then
			temp = up.copy()
			@position.add(temp.scale(Nara::Input.la_t * Nara.delta * @movement_speed * -1.0))
		end

		# Up
		if abs(Nara::Input.ra_t) > ANALOG_DEAD_ZONE then
			temp = up.copy()
			@position.add(temp.scale(Nara::Input.ra_t * Nara.delta * @movement_speed))
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

		# First person mode (a cheap one)
		if @keystate_rb.update(Nara::Input.rb) == true then
			if @first_person == true then
				@first_person = false
				@movement_speed = FLY_SPEED
			else
				@first_person = true
				@movement_speed = WALK_SPEED
			end

			Nara.print("First person mode: #{@first_person}\n")
		end

		if @first_person == true then
			@position.z = Nara.terrain_elevation(@position.x, @position.y) + 1.67

			if @distance > 0.8 then

				# https://knowyourmeme.com/memes/half-life-sound-effects
				cheap_random = (@distance * Nara.frame).round % 4
				@distance = 0.0

				if cheap_random == 0 then
					Nara.play2d(0.3, "../../.local/share/Steam/steamapps/common/Half-Life/valve/sound/player/pl_dirt1.wav")
				elsif cheap_random == 1 then
					Nara.play2d(0.3, "../../.local/share/Steam/steamapps/common/Half-Life/valve/sound/player/pl_dirt2.wav")
				elsif cheap_random == 2 then
					Nara.play2d(0.3, "../../.local/share/Steam/steamapps/common/Half-Life/valve/sound/player/pl_dirt3.wav")
				else
					Nara.play2d(0.3, "../../.local/share/Steam/steamapps/common/Half-Life/valve/sound/player/pl_dirt4.wav")
				end
			end
		end
	end
end
