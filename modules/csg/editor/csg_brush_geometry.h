/**************************************************************************/
/*  csg_brush_geometry.h                                                  */
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

#include "core/math/aabb.h"

// Brush coordinates are world-space, independent of the scene root transform.
namespace CSGBrushGeometry {

inline Vector3 snap_point(const Vector3 &p_point, Vector3::Axis p_normal_axis, real_t p_offset, real_t p_grid) {
	Vector3 point = p_point.snapped(Vector3(p_grid, p_grid, p_grid));
	point[p_normal_axis] = p_offset;
	return point;
}

inline AABB make_box(const Vector3 &p_start, const Vector3 &p_end, Vector3::Axis p_normal_axis, real_t p_offset, real_t p_depth) {
	Vector3 position = p_start.min(p_end);
	Vector3 size = (p_end - p_start).abs();
	position[p_normal_axis] = p_offset;
	size[p_normal_axis] = p_depth;
	return AABB(position, size);
}

inline bool is_valid_box(const AABB &p_box) {
	return p_box.position.is_finite() && p_box.size.is_finite() &&
			p_box.size.x > CMP_EPSILON && p_box.size.y > CMP_EPSILON && p_box.size.z > CMP_EPSILON;
}

} // namespace CSGBrushGeometry
