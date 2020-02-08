
class Vector3

	def initialize(x = 1.0, y = 2.0, z = 3.0)
		@x = x
		@y = y
		@z = z
	end
end

class Camera

	def initialize()
		@position = Vector3.new()
		@angle = Vector3.new(4.0, 5.0, 6.0)

		print("~ Hello, Camera here! ~\n")
	end
end
