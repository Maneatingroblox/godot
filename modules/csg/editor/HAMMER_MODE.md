# Hammer-style box-brush mode

This editor-only tool adds **Hammer Mode** to the 3D toolbar when the CSG module
is enabled. It is a box-brush authoring workflow, not a VMF importer or a complete
replacement for Hammer. Brushes remain ordinary `CSGBox3D` nodes; no new runtime
node, scene format, or project plugin is required.

## Quick start

1. Build and launch this checkout's Godot editor. Open or create a scene with a
   non-CSG `Node3D` root. Exit any camera previews first.
2. Click **Hammer Mode** in the 3D toolbar. The editor opens perspective, top,
   front, and right views, with a brush panel on the right.
3. Click **Draw Box**, then left-drag a rectangle in a viewport. A gold outline
   previews the box in all four views. Nothing is added to the scene yet.
4. Adjust **Depth**, **Plane offset**, and **Brush grid** as needed, then press
   **Enter** while the viewport is focused, or click **Create Brush**.
5. The new brush is selected and drawing switches off. Use Godot's normal move,
   rotate, and scale tools, the CSG box's face handles, or Inspector properties.
   Duplicate and delete work through the existing editor commands.
6. Drop a `Texture2D` into **Brush texture** (or load one through its resource
   menu), choose **Tile size (m)**, and click **Apply to Selected Brushes**.
   The chosen texture is also used for subsequent new brushes.
7. Click **Hammer Mode** again to restore your previous viewport layout and
   cameras. The brushes remain in the scene.

## Drawing controls

- **Auto plane:** orthographic views use their dominant axis; perspective uses
  the XZ floor plane. You can explicitly choose floor (XZ), front (XY), or side
  (YZ). A view parallel to the chosen plane cannot draw on it.
- **Plane offset:** the plane's world Y coordinate for floor, Z for front, or
  X for side. Use it to place stacked floors, walls, or elevated platforms.
- **Depth:** extrusion along the plane's positive normal axis, in meters.
- **Brush grid:** snaps the rectangle's endpoints in world space. Drawing in
  either direction works. Clicks and zero-area rectangles cannot create a box.
  This setting is separate from the 3D toolbar's transform snapping; use the
  toolbar's snap settings when moving or resizing existing brushes.
- **Create collision:** enables native CSG collision on each new brush.
- **Escape / Cancel Preview:** discard the draft without changing the scene.
  Press Escape again to leave Draw Box. Toggling Draw Box, changing planes,
  switching scenes, or leaving Hammer Mode also discards the draft.
- Viewport navigation remains available through the existing middle/right mouse
  and Alt navigation controls. Enter/Escape affect the draft only when a 3D
  viewport has keyboard focus, not while typing in a panel field.

## Materials, persistence, and scope

Textures use a `StandardMaterial3D` with world-space triplanar mapping. This
provides a consistent texture density on all six sides when a brush is resized.
It intentionally aligns textures to the world, rather than making them follow
object movement. Applying an empty texture removes the selected brushes'
materials. Material replacement and creation are undoable. Applying a texture
only affects selected boxes owned by the current scene (or its root), not
non-box nodes or children belonging to other packed scenes.

Brushes are added directly under the edited scene root with the correct owner,
so normal scene saving includes their geometry, materials, and collision
settings. Placement compensates for the root's world transform; a root with
zero scale cannot be used. Mode/layout state is stored through Godot's existing
per-scene editor state mechanism, while unconfirmed drafts are never saved.

This first version does **not** add arbitrary convex brushes, clipping, vertex
editing, hollowing, per-face materials/UV tools, or VMF/MAP import/export. Existing
CSG booleans and mesh/collision baking remain available through Godot's tools.

## Validation

`modules/csg/tests/test_csg_brush_geometry.h` adds five C++ test cases (58
assertions) for all three planes, reversed drags, fractional and negative grid
coordinates, grid changes, and rejection of degenerate/non-finite boxes. These
are included automatically in editor builds made with `tests=yes`:

```sh
scons platform=linuxbsd target=editor tests=yes
bin/godot.linuxbsd.editor.x86_64 --test --test-case='[CSG][Brush]*'
```

Adjust the executable suffix for your build options and platform.

### Interactive regression checklist

- Toggle mode from a custom one/two/four-view arrangement. Check all view
  orientations, split positions, and restoration on exit.
- Draw in top/front/right and perspective, in both drag directions, including
  negative coordinates. Change depth, offset, and grid before confirming.
- Cancel a draft; try a single click, a line, and a parallel drawing plane.
  None should add nodes or undo entries. Navigate without creating brushes.
- Create, undo, and redo. Move, resize by face handle, scale, rotate, duplicate,
  and delete the result using the standard editor tools.
- Apply a texture to one and several selected brushes; change its tile size;
  undo/redo replacement and removal. Confirm unrelated selections are untouched.
- Save/reopen a scene, run it with collision enabled, and repeat with a translated,
  rotated, or scaled scene root.
- Switch scene tabs with an unfinished draft. Confirm no draft leaks into the
  next scene and that each tab keeps its own layout/mode state.
- Check the scrollable panel at small window sizes and editor UI scales.
