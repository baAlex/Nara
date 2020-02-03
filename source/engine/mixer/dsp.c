/*-----------------------------

MIT License

Copyright (c) 2019 Alexander Brandt

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

-------------------------------

 [dsp.c]
 - Alexander Brandt 2019-2020

__
References:

Dimitrios Giannolis, et al. (2012), «Digital Dynamic Range Compressor Design—
A Tutorial and Analysis», J. Audio Eng. Soc., Vol. 60, No. 6, Queen Mary
University of London, London, UK.

Hämäläinen Perttu (2002), «Smoothing Of The Control Signal Without Clipped
Output In Digital Peak Limiters», Proc. of the 5th Int. Conference on
Digital Audio Effects (DAFx-02), Hamburg, Germany.

Roelandts (2016), «Low-Pass Single-Pole IIR Filter»:
https://tomroelandts.com/articles/low-pass-single-pole-iir-filter

Cavaliere (2017), «How to calculate a first order IIR filter in 5 minutes»:
https://www.monocilindro.com/2017/04/08/how-to-implement-a-1st-order-iir-filter-in-5-minutes/

-----------------------------*/

#include "private.h"


static inline float sIIRLowPass(float k, float input, float previous_lp_sample)
{
	return (k * input + (1.0f - k) * previous_lp_sample);
}


inline int DspLimiterInit(const struct jaConfiguration* config, struct DspLimiter* limiter, struct jaStatus* st)
{
	memset(limiter, 0, sizeof(struct DspLimiter));

	if (jaCvarValue(jaCvarGet(config, "limiter.threshold"), &limiter->threshold, st) != 0 ||
	    jaCvarValue(jaCvarGet(config, "limiter.attack"), &limiter->attack, st) != 0 ||
	    jaCvarValue(jaCvarGet(config, "limiter.release"), &limiter->release, st) != 0)
		return 1;

	return 0;
}


inline float DspLimiterGain(struct DspLimiter* limiter, size_t ch, float input)
{
	// TODO, hardcoded K values
	float env = 0;

	// Attack/release
	if (fabsf(input) > limiter->last_envelope_sample[ch])
		env = sIIRLowPass(0.9f, fabsf(input), fabsf(limiter->last_envelope_sample[ch]));
	else
		env = sIIRLowPass(0.001f, fabsf(input), fabsf(limiter->last_envelope_sample[ch]));

	limiter->last_envelope_sample[ch] = env;

	// Gain
	return (env > limiter->threshold) ? env / limiter->threshold : 1.0f;
}
