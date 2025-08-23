#pragma once

#include <vtkRenderer.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
// Functions defined in Rust
struct EframeGlLoader;
extern void *eframe_gl_loader_get_proc_address(const EframeGlLoader *loader,
                                               const char *name);
struct EframeRepaintRequester;
extern void eframe_request_repaint(const EframeRepaintRequester *requester);

// Functions defined in C++
struct VtkWidgetCpp;
void vtk_eframe_load_gl(const struct EframeGlLoader *loader);
VtkWidgetCpp *vtk_widget_new(int width, int height,
                             const EframeRepaintRequester *requester);
void vtk_widget_delete(VtkWidgetCpp *self);
vtkRenderer *vtk_widget_renderer(VtkWidgetCpp *self);
void vtk_widget_paint(VtkWidgetCpp *self);
void vtk_widget_read_state(VtkWidgetCpp *self, bool *needs_repaint, int *width,
                           int *height);
void vtk_widget_mouse_move(VtkWidgetCpp *self, int x, int y);
void vtk_widget_mouse_down(VtkWidgetCpp *self, bool primary, bool secondary,
                           bool middle);
void vtk_widget_mouse_wheel(VtkWidgetCpp *self, int delta);
void vtk_widget_set_size(VtkWidgetCpp *self, int width, int height);
void vtk_widget_reset_camera(VtkWidgetCpp *self);
void vtk_widget_repaint(VtkWidgetCpp *self);

#ifdef __cplusplus
}
#endif // __cplusplus
