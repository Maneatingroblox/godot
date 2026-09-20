/**************************************************************************/
/*  test_csg_brush_geometry.h                                             */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#ifdef TOOLS_ENABLED

#include "../editor/csg_brush_geometry.h"

#include "tests/test_macros.h"

#include <limits>

namespace TestCSGBrushGeometry {

TEST_CASE("[CSG][Brush] Grid snapping preserves the plane offset") {
	const Vector3 point(-1.3, 2.7, 3.2);
	CHECK((CSGBrushGeometry::snap_point(point, Vector3::AXIS_Y, 0.125, 0.5) == Vector3(-1.5, 0.125, 3)));
	CHECK((CSGBrushGeometry::snap_point(point, Vector3::AXIS_Z, -2.125, 0.5) == Vector3(-1.5, 2.5, -2.125)));
	CHECK((CSGBrushGeometry::snap_point(point, Vector3::AXIS_X, 1.125, 0.5) == Vector3(1.125, 2.5, 3)));
}

TEST_CASE("[CSG][Brush] Reversed drags produce the same box on every plane") {
	const Vector3 from(4, 6, 8);
	const Vector3 to(-2, 1, 3);
	for (int axis = 0; axis < 3; axis++) {
		const Vector3::Axis normal = Vector3::Axis(axis);
		const AABB box = CSGBrushGeometry::make_box(from, to, normal, -0.25, 2);
		const AABB reversed = CSGBrushGeometry::make_box(to, from, normal, -0.25, 2);
		CHECK((box == reversed));
		CHECK(CSGBrushGeometry::is_valid_box(box));
		CHECK(box.position[axis] == -0.25);
		CHECK(box.size[axis] == 2);
		CHECK(box.get_center()[axis] == 0.75);
		for (int other = 0; other < 3; other++) {
			if (other != axis) {
				CHECK(box.position[other] == to[other]);
				CHECK(box.size[other] == from[other] - to[other]);
			}
		}
	}
}

TEST_CASE("[CSG][Brush] Clicks and flat boxes cannot create brushes") {
	for (int axis = 0; axis < 3; axis++) {
		const Vector3::Axis normal = Vector3::Axis(axis);
		CHECK_FALSE(CSGBrushGeometry::is_valid_box(CSGBrushGeometry::make_box(Vector3(), Vector3(), normal, 0, 2)));
		CHECK_FALSE(CSGBrushGeometry::is_valid_box(CSGBrushGeometry::make_box(Vector3(), Vector3(2, 2, 2), normal, 0, 0)));
		CHECK_FALSE(CSGBrushGeometry::is_valid_box(CSGBrushGeometry::make_box(Vector3(), Vector3(2, 2, 2), normal, 0, -1)));
	}
	CHECK_FALSE(CSGBrushGeometry::is_valid_box(CSGBrushGeometry::make_box(Vector3(), Vector3(2, 0, 0), Vector3::AXIS_Y, 0, 2)));
}

TEST_CASE("[CSG][Brush] Non-finite geometry is rejected") {
	const real_t inf = std::numeric_limits<real_t>::infinity();
	const real_t nan = std::numeric_limits<real_t>::quiet_NaN();
	CHECK_FALSE(CSGBrushGeometry::is_valid_box(AABB(Vector3(inf, 0, 0), Vector3(1, 1, 1))));
	CHECK_FALSE(CSGBrushGeometry::is_valid_box(AABB(Vector3(), Vector3(1, nan, 1))));
	CHECK_FALSE(CSGBrushGeometry::is_valid_box(AABB(Vector3(), Vector3(1, 1, inf))));
}

TEST_CASE("[CSG][Brush] Changing the grid recomputes bounds from unsnapped endpoints") {
	const Vector3 start(0.2, 0, -0.2);
	const Vector3 end(2.6, 0, -3.6);
	for (real_t grid : { 1.0, 0.5, 0.25 }) {
		const Vector3 from = CSGBrushGeometry::snap_point(start, Vector3::AXIS_Y, 0, grid);
		const Vector3 to = CSGBrushGeometry::snap_point(end, Vector3::AXIS_Y, 0, grid);
		const AABB box = CSGBrushGeometry::make_box(from, to, Vector3::AXIS_Y, 0, 2);
		CHECK(CSGBrushGeometry::is_valid_box(box));
		CHECK(Math::is_zero_approx(Math::fmod(box.position.x, grid)));
		CHECK(Math::is_zero_approx(Math::fmod(box.position.z, grid)));
		CHECK(Math::is_zero_approx(Math::fmod(box.size.x, grid)));
		CHECK(Math::is_zero_approx(Math::fmod(box.size.z, grid)));
	}
}

} // namespace TestCSGBrushGeometry

#endif // TOOLS_ENABLED
