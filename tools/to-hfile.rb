
class String

	def path_basename()
	return self.slice((self.rindex("/") + 1)..(self.rindex(".") - 1))
	end
end

if ARGV.length > 0 then

	simple_name = ARGV[0].path_basename()
	simple_name = simple_name.gsub("-", "_").gsub(" ", "_")
	simple_name = simple_name.downcase()

	print("#include <stdint.h>\n")
	print("\nconst uint8_t g_#{simple_name}[] = {")

	f = File.open(ARGV[0], "r")

	while (byte = f.getbyte()) != nil do
		printf("0x%02X, ", byte)
	end

	f.close()

	print("0x00};\n\n")
end
