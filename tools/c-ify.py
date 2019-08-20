
import sys
import os

if len(sys.argv) > 1:
	simple_name = os.path.basename(sys.argv[1])
	simple_name = os.path.splitext(simple_name)[0]
	simple_name = simple_name.replace("-", "_")
	simple_name = simple_name.lower()

	print("#include <stdint.h>")
	print("\nconst uint8_t g_%s[] = {" % simple_name, end='')

	f = open(sys.argv[1], "rb")

	while 1:
		byte = f.read(1)
		if not byte:
			break

		print("0x%02X, " % ord(byte), end="")

	f.close()

	print("0x00};\n")
