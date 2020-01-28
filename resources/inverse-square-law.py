
# Inverse square law
# ------------------

# Alexander Brandt 2020. Under public domain (or the license that you prefer).

# An apology in advance for what follows. This file is my answer to the question:
# 'when the inverse square law reaches zero?' (well, never) and to the problem of
# have sounds with audible ranges in terms of minimum and maximum distances.
# Of course, I have no real knowledge, so the code is terrible and probably I'm
# making things worse instead of fixing them.... but hey!, works!. :D

import numpy as np
import matplotlib.pyplot as plt

max_distance = 500
min_distance = 300

def Linear(distances):
	out = []
	for d in (distances):
		if d <= min_distance: out.append(1)
		elif d >= max_distance: out.append(0)
		else:
			out.append((d - max_distance) / (min_distance - max_distance))

	return out

def InverseSquare(distances):
	out = []
	for d in (distances):
		if d <= min_distance: out.append(1)
		elif d >= max_distance: out.append(0)
		else:
			interval = max_distance - min_distance
			h_displacement = (d - min_distance) / interval
			v_displacement = h_displacement * 15 + 1

			# Inverse square law, feeded with an value of 1..16
			#	1 because: 1 / (1 * 1) = 1
			#	16 because: 1 / (16 * 16) = 0.0039, an epsilon value
			value = 1 / (v_displacement * v_displacement)
			out.append(value)

	return out

def LinearSquare(distances): # At my taste InverseSquare turns off sounds too quickly
	out = []
	for d in (distances):
		if d <= min_distance: out.append(1)
		elif d >= max_distance: out.append(0)
		else:
			out.append(np.power((d - max_distance) / (min_distance - max_distance), 2))

	return out

##

x = np.linspace(0, 600, num = 500)

plt.plot(x, Linear(x))
plt.plot(x, InverseSquare(x))
plt.plot(x, LinearSquare(x))
plt.show()
