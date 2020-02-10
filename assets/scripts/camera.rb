
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
		@angle = Vector3.new(4.0, 5.0, 6.0)

		Nara.print("~ Hello, Camera here! ~\n")
		Nara.print("#{Nara::Input.a} #{Nara::Input.view}\n")
	end

	def think()

		if Nara.frame > 0 then
			@position.x = 420.0
		end

		Nara.print("~ #{@position.x}! (#{Nara.frame} #{Nara.delta}) #{Nara::VERSION} ~\n")
	end
end
