/**************************************************************************/
/*  csg_brush_editor_plugin.cpp                                           */
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

#include "csg_brush_editor_plugin.h"

#include "../csg_shape.h"
#include "csg_brush_geometry.h"

#include "core/input/input.h"
#include "core/object/callable_mp.h"
#include "editor/editor_data.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/inspector/editor_resource_picker.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_viewport.h"
#include "editor/themes/editor_scale.h"
#include "scene/3d/camera_3d.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/label.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"

void CSGBrushEditorPlugin::_mode_toggled(bool p_enabled) {
	Node3DEditor *editor = Node3DEditor::get_singleton();
	if (p_enabled) {
		for (int i = 0; i < 4; i++) {
			const Dictionary view = editor->get_editor_viewport(i)->get_state();
			if (view.has("previewing") || bool(view.get("cinematic_preview", false))) {
				mode_button->set_pressed_no_signal(false);
				EditorNode::get_singleton()->show_warning(TTR("Exit camera preview before entering Hammer Mode."));
				return;
			}
		}
	}
	enabled = p_enabled;
	panel->set_visible(enabled);
	_cancel_draft();
	draw_button->set_pressed(false);

	if (enabled) {
		const Dictionary state = editor->get_state();
		previous_layout = Dictionary();
		previous_layout["viewport_mode"] = state["viewport_mode"];
		previous_layout["viewport_splits"] = state["viewport_splits"];
		previous_layout["viewports"] = state["viewports"];

		Dictionary layout;
		layout["viewport_mode"] = 4;
		Dictionary splits;
		splits["main"] = 0;
		splits["first"] = 0;
		splits["second"] = 0;
		layout["viewport_splits"] = splits;
		Array views;
		for (int i = 0; i < 4; i++) {
			Dictionary view;
			view["position"] = Vector3();
			view["distance"] = 20.0;
			view["orthogonal"] = i != 0;
			view["auto_orthogonal_enabled"] = false;
			view["lock_rotation"] = i != 0;
			view["x_rotation"] = i == 1 ? Math::PI / 2.0 : (i == 0 ? 0.5 : 0.0);
			view["y_rotation"] = i == 3 ? -Math::PI / 2.0 : (i == 0 ? -0.5 : 0.0);
			const View3DController::ViewType types[4] = {
				View3DController::VIEW_TYPE_USER, View3DController::VIEW_TYPE_TOP,
				View3DController::VIEW_TYPE_FRONT, View3DController::VIEW_TYPE_RIGHT
			};
			view["view_type"] = types[i];
			view["grid"] = true;
			view["gizmos"] = true;
			view["transform_gizmo"] = true;
			views.push_back(view);
		}
		layout["viewports"] = views;
		editor->set_state(layout);
		_selection_changed();
	} else if (!previous_layout.is_empty()) {
		editor->set_state(previous_layout);
		previous_layout = Dictionary();
	}
}

void CSGBrushEditorPlugin::_draw_toggled(bool p_enabled) {
	_cancel_draft();
}

void CSGBrushEditorPlugin::_cancel_draft() {
	dragging = false;
	has_draft = false;
	drag_camera = ObjectID();
	create_button->set_disabled(true);
	status->set_text(draw_button->is_pressed() ? TTR("Drag a rectangle in a viewport.\nEnter to create, Esc to cancel.") : TTR("Select a brush to move, rotate or resize it using the 3D tools and box handles."));
	update_overlays();
}

void CSGBrushEditorPlugin::_settings_changed(double p_value) {
	if (has_draft) {
		_update_draft();
	}
}

void CSGBrushEditorPlugin::_plane_changed(int p_index) {
	_cancel_draft();
}

void CSGBrushEditorPlugin::_update_draft() {
	const Vector3 a = CSGBrushGeometry::snap_point(start, normal_axis, plane_offset->get_value(), grid_size->get_value());
	const Vector3 b = CSGBrushGeometry::snap_point(end, normal_axis, plane_offset->get_value(), grid_size->get_value());
	draft = CSGBrushGeometry::make_box(a, b, normal_axis, plane_offset->get_value(), depth->get_value());
	create_button->set_disabled(dragging || !CSGBrushGeometry::is_valid_box(draft));
	status->set_text(vformat(TTR("Size: %s × %s × %s\nEnter or Create Brush to confirm."), String::num(draft.size.x, 2), String::num(draft.size.y, 2), String::num(draft.size.z, 2)));
	update_overlays();
}

bool CSGBrushEditorPlugin::_project(Camera3D *p_camera, const Vector2 &p_position, Vector3 &r_point) const {
	Vector3 normal;
	normal[normal_axis] = 1.0;
	const Plane plane(normal, plane_offset->get_value());
	return plane.intersects_ray(p_camera->project_ray_origin(p_position), p_camera->project_ray_normal(p_position), &r_point) && r_point.is_finite();
}

EditorPlugin::AfterGUIInput CSGBrushEditorPlugin::forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) {
	if (!enabled || !draw_button->is_pressed()) {
		return AFTER_GUI_INPUT_PASS;
	}

	const Ref<InputEventKey> key = p_event;
	if (key.is_valid() && key->is_pressed() && !key->is_echo()) {
		if (key->get_keycode() == Key::ESCAPE) {
			if (has_draft) {
				_cancel_draft();
			} else {
				draw_button->set_pressed(false);
			}
			return AFTER_GUI_INPUT_STOP;
		}
		if (key->get_keycode() == Key::ENTER || key->get_keycode() == Key::KP_ENTER) {
			_create_brush();
			return AFTER_GUI_INPUT_STOP;
		}
	}

	const Ref<InputEventMouseButton> button = p_event;
	if (button.is_valid() && dragging && button->is_pressed() &&
			(button->get_button_index() == MouseButton::RIGHT || button->get_button_index() == MouseButton::MIDDLE)) {
		_cancel_draft();
		return AFTER_GUI_INPUT_PASS;
	}
	if (button.is_valid() && button->get_button_index() == MouseButton::LEFT) {
		// Leave Alt-drag and freelook to the normal viewport navigation.
		if (!dragging && (button->is_alt_pressed() || Input::get_singleton()->is_mouse_button_pressed(MouseButton::RIGHT))) {
			return AFTER_GUI_INPUT_PASS;
		}
		if (button->is_pressed()) {
			bool editor_camera = false;
			for (int i = 0; i < 4; i++) {
				if (Node3DEditor::get_singleton()->get_editor_viewport(i)->get_camera_3d() == p_camera) {
					editor_camera = true;
					break;
				}
			}
			if (!editor_camera) {
				status->set_text(TTR("Exit camera preview before drawing brushes."));
				return AFTER_GUI_INPUT_STOP;
			}
			Node *root = EditorNode::get_singleton()->get_edited_scene();
			if (!Object::cast_to<Node3D>(root) || Object::cast_to<CSGShape3D>(root)) {
				status->set_text(TTR("Create a scene with a Node3D root first. Brushes are added under that root."));
				return AFTER_GUI_INPUT_STOP;
			}
			_cancel_draft();
			switch (plane_option->get_selected()) {
				case 1:
					normal_axis = Vector3::AXIS_Y;
					break;
				case 2:
					normal_axis = Vector3::AXIS_Z;
					break;
				case 3:
					normal_axis = Vector3::AXIS_X;
					break;
				default: {
					// Orthographic views draw on their dominant plane; perspective draws on the floor.
					normal_axis = p_camera->get_projection() == Camera3D::PROJECTION_ORTHOGONAL ? p_camera->get_global_basis().get_column(2).abs().max_axis_index() : Vector3::AXIS_Y;
				} break;
			}
			if (!_project(p_camera, button->get_position(), start)) {
				status->set_text(TTR("This view does not intersect the drawing plane. Change the view or plane."));
				return AFTER_GUI_INPUT_STOP;
			}
			end = start;
			dragging = true;
			has_draft = true;
			drag_camera = p_camera->get_instance_id();
			_update_draft();
		} else if (dragging) {
			if (drag_camera == p_camera->get_instance_id()) {
				Vector3 point;
				if (_project(p_camera, button->get_position(), point)) {
					end = point;
				}
			}
			dragging = false;
			_update_draft();
		}
		return AFTER_GUI_INPUT_STOP;
	}

	const Ref<InputEventMouseMotion> motion = p_event;
	if (motion.is_valid() && dragging) {
		if (!motion->get_button_mask().has_flag(MouseButtonMask::LEFT)) {
			_cancel_draft(); // The button may have been released outside the viewport.
		} else if (drag_camera == p_camera->get_instance_id()) {
			Vector3 point;
			if (_project(p_camera, motion->get_position(), point)) {
				end = point;
				_update_draft();
			}
		}
		return AFTER_GUI_INPUT_STOP;
	}
	return AFTER_GUI_INPUT_PASS;
}

void CSGBrushEditorPlugin::forward_3d_force_draw_over_viewport(Control *p_overlay) {
	if (!enabled || !has_draft) {
		return;
	}
	Camera3D *camera = nullptr;
	for (int i = 0; i < 4; i++) {
		Node3DEditorViewport *viewport = Node3DEditor::get_singleton()->get_editor_viewport(i);
		if (viewport->get_surface() == p_overlay) {
			const Dictionary view = viewport->get_state();
			if (view.has("previewing") || bool(view.get("cinematic_preview", false))) {
				return;
			}
			camera = viewport->get_camera_3d();
			break;
		}
	}
	if (!camera) {
		return;
	}
	const Color color(1.0, 0.7, 0.25);
	for (int i = 0; i < 12; i++) {
		Vector3 from;
		Vector3 to;
		draft.get_edge(i, from, to);
		if (!camera->is_position_behind(from) && !camera->is_position_behind(to)) {
			p_overlay->draw_line(camera->unproject_position(from), camera->unproject_position(to), color, 2.0 * EDSCALE, true);
		}
	}
}

Ref<StandardMaterial3D> CSGBrushEditorPlugin::_make_material() const {
	Ref<StandardMaterial3D> material;
	material.instantiate();
	material->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, texture_picker->get_edited_resource());
	material->set_flag(BaseMaterial3D::FLAG_UV1_USE_TRIPLANAR, true);
	material->set_flag(BaseMaterial3D::FLAG_UV1_USE_WORLD_TRIPLANAR, true);
	const real_t scale = 1.0 / texture_size->get_value();
	material->set_uv1_scale(Vector3(scale, scale, scale));
	material->set_roughness(1.0);
	return material;
}

void CSGBrushEditorPlugin::_create_brush() {
	if (!has_draft || dragging || !CSGBrushGeometry::is_valid_box(draft)) {
		return;
	}
	Node3D *root = Object::cast_to<Node3D>(EditorNode::get_singleton()->get_edited_scene());
	if (!root || Object::cast_to<CSGShape3D>(root) || Math::is_zero_approx(root->get_global_basis().determinant())) {
		status->set_text(TTR("Brushes need a non-CSG Node3D scene root with a non-zero scale."));
		return;
	}

	CSGBox3D *brush = memnew(CSGBox3D);
	brush->set_name("Brush");
	brush->set_size(draft.size);
	brush->set_use_collision(collision->is_pressed());
	if (texture_picker->get_edited_resource().is_valid()) {
		brush->set_material(_make_material());
	}
	// Preserve world-space box dimensions even under a transformed scene root.
	brush->set_transform(root->get_global_transform().affine_inverse() * Transform3D(Basis(), draft.get_center()));

	EditorUndoRedoManager *undo_redo = get_undo_redo();
	undo_redo->create_action(TTR("Create Brush"), UndoRedo::MERGE_DISABLE, root);
	undo_redo->add_do_method(root, "add_child", brush, true);
	undo_redo->add_do_method(brush, "set_owner", root);
	undo_redo->add_do_reference(brush);
	undo_redo->add_undo_method(root, "remove_child", brush);
	undo_redo->commit_action();

	EditorSelection *selection = EditorNode::get_singleton()->get_editor_selection();
	selection->clear();
	selection->add_node(brush);
	EditorNode::get_singleton()->push_item(brush);
	draw_button->set_pressed(false);
}

void CSGBrushEditorPlugin::_selection_changed() {
	Node *root = EditorNode::get_singleton()->get_edited_scene();
	bool has_brush = false;
	if (!root) {
		apply_button->set_disabled(true);
		return;
	}
	for (Node *node : EditorNode::get_singleton()->get_editor_selection()->get_full_selected_node_list()) {
		if (Object::cast_to<CSGBox3D>(node) && (node == root || node->get_owner() == root)) {
			has_brush = true;
			break;
		}
	}
	apply_button->set_disabled(!has_brush);
}

void CSGBrushEditorPlugin::_apply_texture() {
	Node *root = EditorNode::get_singleton()->get_edited_scene();
	if (!root) {
		return;
	}
	Vector<CSGBox3D *> brushes;
	for (Node *node : EditorNode::get_singleton()->get_editor_selection()->get_full_selected_node_list()) {
		CSGBox3D *brush = Object::cast_to<CSGBox3D>(node);
		if (brush && (node == root || node->get_owner() == root)) {
			brushes.push_back(brush);
		}
	}
	if (brushes.is_empty()) {
		return;
	}
	Ref<StandardMaterial3D> material;
	if (texture_picker->get_edited_resource().is_valid()) {
		material = _make_material();
	}
	EditorUndoRedoManager *undo_redo = get_undo_redo();
	undo_redo->create_action(TTR("Texture Brushes"), UndoRedo::MERGE_DISABLE, root);
	for (CSGBox3D *brush : brushes) {
		undo_redo->add_do_method(brush, "set_material", material);
		undo_redo->add_undo_method(brush, "set_material", brush->get_material());
	}
	undo_redo->commit_action();
}

void CSGBrushEditorPlugin::_scene_changed(Node *p_root) {
	_cancel_draft();
	draw_button->set_pressed(false);
	_selection_changed();
}

Dictionary CSGBrushEditorPlugin::get_state() const {
	Dictionary state;
	state["enabled"] = enabled;
	state["previous_layout"] = previous_layout.duplicate(true);
	return state;
}

void CSGBrushEditorPlugin::set_state(const Dictionary &p_state) {
	_cancel_draft();
	draw_button->set_pressed(false);
	enabled = p_state.get("enabled", false);
	previous_layout = Dictionary(p_state.get("previous_layout", Dictionary())).duplicate(true);
	mode_button->set_pressed_no_signal(enabled);
	panel->set_visible(enabled);
	// The 3D editor restores its own viewport state when changing scene tabs.
}

void CSGBrushEditorPlugin::clear() {
	set_state(Dictionary());
}

CSGBrushEditorPlugin::CSGBrushEditorPlugin() {
	mode_button = memnew(Button);
	mode_button->set_text(TTR("Hammer Mode"));
	mode_button->set_toggle_mode(true);
	mode_button->set_tooltip_text(TTR("Switch to a four-view box-brush workspace. Uses native CSGBox3D nodes."));
	mode_button->connect(SceneStringName(toggled), callable_mp(this, &CSGBrushEditorPlugin::_mode_toggled));
	add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, mode_button);

	panel = memnew(PanelContainer);
	panel->set_custom_minimum_size(Size2(230, 0) * EDSCALE);
	ScrollContainer *scroll = memnew(ScrollContainer);
	scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	panel->add_child(scroll);
	VBoxContainer *content = memnew(VBoxContainer);
	content->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	scroll->add_child(content);
	Label *title = memnew(Label);
	title->set_text(TTR("BOX BRUSHES"));
	content->add_child(title);

	draw_button = memnew(Button);
	draw_button->set_text(TTR("Draw Box"));
	draw_button->set_toggle_mode(true);
	draw_button->set_tooltip_text(TTR("Turn off to select and transform existing brushes."));
	draw_button->connect(SceneStringName(toggled), callable_mp(this, &CSGBrushEditorPlugin::_draw_toggled));
	content->add_child(draw_button);

	plane_option = memnew(OptionButton);
	plane_option->add_item(TTR("Plane: Auto (from view)"));
	plane_option->add_item(TTR("Plane: Floor (XZ)"));
	plane_option->add_item(TTR("Plane: Front (XY)"));
	plane_option->add_item(TTR("Plane: Side (YZ)"));
	plane_option->connect(SceneStringName(item_selected), callable_mp(this, &CSGBrushEditorPlugin::_plane_changed));
	content->add_child(plane_option);

	auto add_spin = [content](const String &p_label, double p_min, double p_max, double p_step, double p_default) {
		HBoxContainer *row = memnew(HBoxContainer);
		content->add_child(row);
		Label *label = memnew(Label);
		label->set_text(p_label);
		label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		row->add_child(label);
		SpinBox *spin = memnew(SpinBox);
		spin->set_min(p_min);
		spin->set_max(p_max);
		spin->set_step(p_step);
		spin->set_value(p_default);
		row->add_child(spin);
		return spin;
	};
	grid_size = add_spin(TTR("Brush grid"), 0.01, 1024, 0.01, 1);
	grid_size->set_tooltip_text(TTR("Grid spacing for drawing new brushes. For moving and resizing, use the 3D toolbar snap settings."));
	depth = add_spin(TTR("Depth"), 0.01, 16384, 0.01, 2);
	plane_offset = add_spin(TTR("Plane offset"), -16384, 16384, 0.01, 0);
	plane_offset->set_tooltip_text(TTR("World coordinate along the plane normal: Y for floor, Z for front, X for side. Depth extends in the positive direction."));
	for (SpinBox *spin : { grid_size, depth, plane_offset }) {
		spin->connect(SceneStringName(value_changed), callable_mp(this, &CSGBrushEditorPlugin::_settings_changed));
	}
	collision = memnew(CheckBox);
	collision->set_text(TTR("Create collision"));
	collision->set_pressed(true);
	content->add_child(collision);

	create_button = memnew(Button);
	create_button->set_text(TTR("Create Brush"));
	create_button->set_disabled(true);
	create_button->connect(SceneStringName(pressed), callable_mp(this, &CSGBrushEditorPlugin::_create_brush));
	content->add_child(create_button);
	Button *cancel = memnew(Button);
	cancel->set_text(TTR("Cancel Preview"));
	cancel->connect(SceneStringName(pressed), callable_mp(this, &CSGBrushEditorPlugin::_cancel_draft));
	content->add_child(cancel);
	content->add_child(memnew(HSeparator));

	Label *texture_label = memnew(Label);
	texture_label->set_text(TTR("Brush texture"));
	content->add_child(texture_label);
	texture_picker = memnew(EditorResourcePicker);
	texture_picker->set_base_type("Texture2D");
	texture_picker->set_tooltip_text(TTR("Drop a texture here or load one from the resource menu. An empty texture clears the material when applied."));
	content->add_child(texture_picker);
	texture_size = add_spin(TTR("Tile size (m)"), 0.01, 1024, 0.01, 2);
	texture_size->set_tooltip_text(TTR("World-space triplanar mapping keeps texture density consistent when brushes are resized."));
	apply_button = memnew(Button);
	apply_button->set_text(TTR("Apply to Selected Brushes"));
	apply_button->set_disabled(true);
	apply_button->connect(SceneStringName(pressed), callable_mp(this, &CSGBrushEditorPlugin::_apply_texture));
	content->add_child(apply_button);

	status = memnew(Label);
	status->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	status->set_text(TTR("Draw a box, set its depth, then create it. Use the normal 3D tools to edit brushes."));
	content->add_child(status);
	add_control_to_container(CONTAINER_SPATIAL_EDITOR_SIDE_RIGHT, panel);
	panel->hide();

	set_input_event_forwarding_always_enabled();
	set_force_draw_over_forwarding_enabled();
	EditorNode::get_singleton()->get_editor_selection()->connect("selection_changed", callable_mp(this, &CSGBrushEditorPlugin::_selection_changed));
	connect("scene_changed", callable_mp(this, &CSGBrushEditorPlugin::_scene_changed));
}
