
class Vector3

	attr_accessor :x
	attr_accessor :y
	attr_accessor :z

	def initialize(x = 1.0, y = 2.0, z = 3.0)
		@x = x
		@y = y
		@z = z
	end
end

class Camera

	def initialize()
		@position = Vector3.new()
		@angle = Vector3.new()
	end

	def think()
		#Nara.print("hello #{@position.x}\n")
	end
end
