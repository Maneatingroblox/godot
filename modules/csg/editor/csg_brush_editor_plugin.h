/**************************************************************************/
/*  csg_brush_editor_plugin.h                                             */
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
#include "editor/plugins/editor_plugin.h"
#include "scene/resources/material.h"

class Button;
class CheckBox;
class EditorResourcePicker;
class Label;
class OptionButton;
class PanelContainer;
class SpinBox;

class CSGBrushEditorPlugin : public EditorPlugin {
	GDCLASS(CSGBrushEditorPlugin, EditorPlugin);

	Button *mode_button = nullptr;
	PanelContainer *panel = nullptr;
	Button *draw_button = nullptr;
	Button *create_button = nullptr;
	Button *apply_button = nullptr;
	OptionButton *plane_option = nullptr;
	SpinBox *grid_size = nullptr;
	SpinBox *depth = nullptr;
	SpinBox *plane_offset = nullptr;
	SpinBox *texture_size = nullptr;
	CheckBox *collision = nullptr;
	EditorResourcePicker *texture_picker = nullptr;
	Label *status = nullptr;

	Dictionary previous_layout;
	bool enabled = false;
	bool dragging = false;
	bool has_draft = false;
	ObjectID drag_camera;
	Vector3 start;
	Vector3 end;
	Vector3::Axis normal_axis = Vector3::AXIS_Y;
	AABB draft;

	void _mode_toggled(bool p_enabled);
	void _draw_toggled(bool p_enabled);
	void _cancel_draft();
	void _settings_changed(double p_value);
	void _plane_changed(int p_index);
	void _update_draft();
	void _selection_changed();
	bool _project(Camera3D *p_camera, const Vector2 &p_position, Vector3 &r_point) const;
	void _create_brush();
	Ref<StandardMaterial3D> _make_material() const;
	void _apply_texture();
	void _scene_changed(Node *p_root);

public:
	String get_plugin_name() const override { return "CSGBrushEditor"; }
	AfterGUIInput forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) override;
	void forward_3d_force_draw_over_viewport(Control *p_overlay) override;
	Dictionary get_state() const override;
	void set_state(const Dictionary &p_state) override;
	void clear() override;

	CSGBrushEditorPlugin();
};
