/* VanadiumEngine, a Vulkan rendering toolkit
 * Copyright (C) 2022 Friedrich Vock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "AtmosphereParamsCommon.hpp"
#define TEST_TRACE_PASSED
#include <TestUtilCommon.hpp>
#include <iostream>

#include "../../../atmosphere/shaders/functions/common.glsl"

#include "definitions.glsl"
#define IN(x) x
#define OUT(x) x&

#define TRANSMITTANCE_TEXTURE_WIDTH 1
#define TRANSMITTANCE_TEXTURE_HEIGHT 1

/**
 * Copyright (c) 2017 Eric Bruneton
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Precomputed Atmospheric Scattering
 * Copyright (c) 2008 INRIA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*<h2>atmosphere/functions.glsl</h2>
<p>This GLSL file contains the core functions that implement our atmosphere
model. It provides functions to compute the transmittance, the single scattering
and the second and higher orders of scattering, the ground irradiance, as well
as functions to store these in textures and to read them back. It uses physical
types and constants which are provided in two versions: a
<a href="definitions.glsl.html">GLSL version</a> and a
<a href="reference/definitions.h.html">C++ version</a>. This allows this file to
be compiled either with a GLSL compiler or with a C++ compiler (see the
<a href="../index.html">Introduction</a>).
<p>The functions provided in this file are organized as follows:
<ul>
<li><a href="#transmittance">Transmittance</a>
<ul>
<li><a href="#transmittance_computation">Computation</a></li>
<li><a href="#transmittance_precomputation">Precomputation</a></li>
<li><a href="#transmittance_lookup">Lookup</a></li>
</ul>
</li>
<li><a href="#single_scattering">Single scattering</a>
<ul>
<li><a href="#single_scattering_computation">Computation</a></li>
<li><a href="#single_scattering_precomputation">Precomputation</a></li>
<li><a href="#single_scattering_lookup">Lookup</a></li>
</ul>
</li>
<li><a href="#multiple_scattering">Multiple scattering</a>
<ul>
<li><a href="#multiple_scattering_computation">Computation</a></li>
<li><a href="#multiple_scattering_precomputation">Precomputation</a></li>
<li><a href="#multiple_scattering_lookup">Lookup</a></li>
</ul>
</li>
<li><a href="#irradiance">Ground irradiance</a>
<ul>
<li><a href="#irradiance_computation">Computation</a></li>
<li><a href="#irradiance_precomputation">Precomputation</a></li>
<li><a href="#irradiance_lookup">Lookup</a></li>
</ul>
</li>
<li><a href="#rendering">Rendering</a>
<ul>
<li><a href="#rendering_sky">Sky</a></li>
<li><a href="#rendering_aerial_perspective">Aerial perspective</a></li>
<li><a href="#rendering_ground">Ground</a></li>
</ul>
</li>
</ul>
<p>They use the following utility functions to avoid NaNs due to floating point
values slightly outside their theoretical bounds:
*/

Number ClampCosine(Number mu) { return clamp(mu, Number(-1.0), Number(1.0)); }

Length ClampDistance(Length d) { return max(d, 0.0f * m); }

Length ClampRadius(IN(AtmosphereParameters) atmosphere, Length r) {
	return clamp(r, atmosphere.bottom_radius, atmosphere.top_radius);
}

Length SafeSqrt(Area a) { return sqrt(max(a, 0.0f * m2)); }

/*
<h3 id="transmittance">Transmittance</h3>
<p>As the light travels from a point $p$ to a point $q$ in the atmosphere,
it is partially absorbed and scattered out of its initial direction because of
the air molecules and the aerosol particles. Thus, the light arriving at $q$
is only a fraction of the light from $p$, and this fraction, which depends on
wavelength, is called the
<a href="https://en.wikipedia.org/wiki/Transmittance">transmittance</a>. The
following sections describe how we compute it, how we store it in a precomputed
texture, and how we read it back.
<h4 id="transmittance_computation">Computation</h4>
<p>For 3 aligned points $p$, $q$ and $r$ inside the atmosphere, in this
order, the transmittance between $p$ and $r$ is the product of the
transmittance between $p$ and $q$ and between $q$ and $r$. In
particular, the transmittance between $p$ and $q$ is the transmittance
between $p$ and the nearest intersection $i$ of the half-line $[p,q)$
with the top or bottom atmosphere boundary, divided by the transmittance between
$q$ and $i$ (or 0 if the segment $[p,q]$ intersects the ground):
<svg width="340px" height="195px">
  <style type="text/css"><![CDATA[
	circle { fill: #000000; stroke: none; }
	path { fill: none; stroke: #000000; }
	text { font-size: 16px; font-style: normal; font-family: Sans; }
	.vector { font-weight: bold; }
  ]]></style>
  <path d="m 0,26 a 600,600 0 0 1 340,0"/>
  <path d="m 0,110 a 520,520 0 0 1 340,0"/>
  <path d="m 170,190 0,-30"/>
  <path d="m 170,140 0,-130"/>
  <path d="m 170,50 165,-33"/>
  <path d="m 155,150 10,-10 10,10 10,-10"/>
  <path d="m 155,160 10,-10 10,10 10,-10"/>
  <path d="m 95,50 30,0"/>
  <path d="m 95,190 30,0"/>
  <path d="m 105,50 0,140" style="stroke-dasharray:8,2;"/>
  <path d="m 100,55 5,-5 5,5"/>
  <path d="m 100,185 5,5 5,-5"/>
  <path d="m 170,25 a 25,25 0 0 1 25,20" style="stroke-dasharray:4,2;"/>
  <path d="m 170,190 70,0"/>
  <path d="m 235,185 5,5 -5,5"/>
  <path d="m 165,125 5,-5 5,5"/>
  <circle cx="170" cy="190" r="2.5"/>
  <circle cx="170" cy="50" r="2.5"/>
  <circle cx="320" cy="20" r="2.5"/>
  <circle cx="270" cy="30" r="2.5"/>
  <text x="155" y="45" class="vector">p</text>
  <text x="265" y="45" class="vector">q</text>
  <text x="320" y="15" class="vector">i</text>
  <text x="175" y="185" class="vector">o</text>
  <text x="90" y="125">r</text>
  <text x="185" y="25">μ=cos(θ)</text>
  <text x="240" y="185">x</text>
  <text x="155" y="120">z</text>
</svg>
<p>Also, the transmittance between $p$ and $q$ and between $q$ and $p$
are the same. Thus, to compute the transmittance between arbitrary points, it
is sufficient to know the transmittance between a point $p$ in the atmosphere,
and points $i$ on the top atmosphere boundary. This transmittance depends on
only two parameters, which can be taken as the radius $r=\Vertop\Vert$ and
the cosine of the "view zenith angle",
$\mu=op*/

int main() {}