
import sys
import os

if len(sys.argv) > 2:

	simple_name = os.path.basename(sys.argv[1])
	simple_name = os.path.splitext(simple_name)[0]
	simple_name = simple_name.replace("-", "_")
	simple_name = simple_name.lower()

	out_directory = os.path.dirname(sys.argv[2])

	if not os.path.exists(out_directory):
		os.makedirs(out_directory)

	inp = open(sys.argv[1], "rb")
	out = open(sys.argv[2], "w")

	out.write("#include <stdint.h>\n")
	out.write("const uint8_t g_%s[] = {" % simple_name)

	while 1:
		byte = inp.read(1)
		if not byte:
			break

		out.write("0x%02X, " % ord(byte))

	out.write("0x00};\n")
	
	out.close()
	inp.close()
