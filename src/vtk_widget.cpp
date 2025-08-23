#include <vtkActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCubeSource.h>
#include <vtkFileOutputWindow.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkGenericRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkOutputWindow.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtk_glad.h>

#include "vtk_widget.h"

struct VtkWidgetCpp {
    const EframeRepaintRequester *m_repaint_requester;
    int m_width, m_height;

    vtkNew<vtkRenderer> m_renderer;
    vtkNew<vtkGenericOpenGLRenderWindow> m_render_window;
    vtkNew<vtkGenericRenderWindowInteractor> m_interactor;

    bool m_needs_repaint = true;
    bool m_primary_down = false;
    bool m_secondary_down = false;
    bool m_middle_down = false;

    VtkWidgetCpp(int width, int height, const EframeRepaintRequester *requester)
        : m_width{width}, m_height{height}, m_repaint_requester{requester} {}
};

namespace {
// TODO: we probably want to do this GL stuff in a different "class"
const EframeGlLoader *gl_loader = nullptr;
bool gl_loaded = false;

void IsCurrentCallback(vtkObject *vtkNotUsed(caller),
                       long unsigned int vtkNotUsed(eventId),
                       void *vtkNotUsed(clientData), void *callData) {
    // We always make sure to have the context for VTK active before calling
    // render on it
    *(static_cast<bool *>(callData)) = true;
}

void FrameCallback(vtkObject *vtkNotUsed(caller),
                   long unsigned int vtkNotUsed(eventId), void *clientData,
                   void *vtkNotUsed(callData)) {
    VtkWidgetCpp *widget = static_cast<VtkWidgetCpp *>(clientData);

    assert(widget->m_repaint_requester != nullptr);
    widget->m_needs_repaint = true;
    eframe_request_repaint(widget->m_repaint_requester);
}

GLADapiproc gl_get_proc_address(const char *name) {
    assert(gl_loader != nullptr);
    return reinterpret_cast<GLADapiproc>(
        eframe_gl_loader_get_proc_address(gl_loader, name));
}
} // namespace

void vtk_eframe_load_gl(const EframeGlLoader *loader) {
    if (gl_loaded) {
        std::cout << "OpenGL alread loaded. Skipping.\n";
        return;
    }

    gl_loader = loader;
    gladLoadGL(&gl_get_proc_address);

    // The loader has a limited lifetime so we need to discard our pointer
    gl_loader = nullptr;
    gl_loaded = true;

    // Or redirect to your own logging system
    vtkNew<vtkFileOutputWindow> fileOutputWindow;
    fileOutputWindow->SetFileName("vtk_output.log");
    vtkOutputWindow::SetInstance(fileOutputWindow);
}

VtkWidgetCpp *vtk_widget_new(int width, int height,
                             const EframeRepaintRequester *requester) {
    vtkLogScopeFunction(INFO);
    vtkLogScopeF(INFO, "do-vtk-new");

    assert(gl_loaded);

    auto self = new VtkWidgetCpp(width, height, requester);

    self->m_renderer->ResetCamera();
    self->m_renderer->SetAutomaticLightCreation(true);

    self->m_render_window->AddRenderer(self->m_renderer);
    self->m_render_window->SetSize(width, height);

    vtkNew<vtkCallbackCommand> is_current_cb;
    is_current_cb->SetCallback(IsCurrentCallback);
    self->m_render_window->AddObserver(vtkCommand::WindowIsCurrentEvent,
                                       is_current_cb);

    vtkNew<vtkCallbackCommand> frame_cb;
    frame_cb->SetCallback(FrameCallback);
    frame_cb->SetClientData(self);
    self->m_render_window->AddObserver(vtkCommand::WindowFrameEvent, frame_cb);

    self->m_interactor->SetRenderWindow(self->m_render_window);

    vtkNew<vtkInteractorStyleTrackballCamera> style;
    self->m_interactor->SetInteractorStyle(style);

    vtkNew<vtkActor> actor;
    self->m_renderer->AddActor(actor);
    actor->RotateX(45.0);
    actor->RotateY(45.0);
    actor->GetProperty()->SetColor(0.8, 0.2, 0.2);

    vtkNew<vtkPolyDataMapper> mapper;
    actor->SetMapper(mapper);

    vtkNew<vtkCubeSource> cs;
    mapper->SetInputConnection(cs->GetOutputPort());

    return self;
}

void vtk_widget_delete(VtkWidgetCpp *self) {
    assert(self != nullptr);
    delete self;
}

vtkRenderer *vtk_widget_renderer(VtkWidgetCpp *self) {
    return self->m_renderer;
}

void vtk_widget_paint(VtkWidgetCpp *self) {
    vtkLogScopeFunction(INFO);
    vtkLogScopeF(INFO, "do-vtk-render");

    self->m_render_window->Render();
    self->m_needs_repaint = false;
}

void vtk_widget_read_state(VtkWidgetCpp *self, bool *needs_repaint_out,
                           int *width_out, int *height_out) {
    *needs_repaint_out = self->m_needs_repaint;
    *width_out = self->m_width;
    *height_out = self->m_height;
}

void vtk_widget_mouse_move(VtkWidgetCpp *self, int x, int y) {
    self->m_interactor->SetEventPosition(x, y);
    self->m_interactor->InvokeEvent(vtkCommand::MouseMoveEvent);
}

void vtk_widget_mouse_down(VtkWidgetCpp *self, bool primary, bool secondary,
                           bool middle) {
    if (self->m_primary_down != primary) {
        if (primary)
            self->m_interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent);
        else
            self->m_interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent);

        self->m_primary_down = primary;
    }

    if (self->m_secondary_down != secondary) {
        if (secondary)
            self->m_interactor->InvokeEvent(vtkCommand::RightButtonPressEvent);
        else
            self->m_interactor->InvokeEvent(
                vtkCommand::RightButtonReleaseEvent);

        self->m_secondary_down = secondary;
    }

    if (self->m_middle_down != middle) {
        if (middle)
            self->m_interactor->InvokeEvent(vtkCommand::MiddleButtonPressEvent);
        else
            self->m_interactor->InvokeEvent(
                vtkCommand::MiddleButtonReleaseEvent);

        self->m_middle_down = middle;
    }
}

void vtk_widget_mouse_wheel(VtkWidgetCpp *self, int delta) {
    if (delta > 0) {
        self->m_interactor->InvokeEvent(vtkCommand::MouseWheelForwardEvent);
    } else if (delta < 0) {
        self->m_interactor->InvokeEvent(vtkCommand::MouseWheelBackwardEvent);
    }
}

void vtk_widget_set_size(VtkWidgetCpp *self, int width, int height) {
    if (self->m_width == width && self->m_height == height) {
        return;
    }

    self->m_render_window->SetSize(width, height);

    self->m_width = width;
    self->m_height = height;
    self->m_needs_repaint = true;
}

void vtk_widget_reset_camera(VtkWidgetCpp *self) {
    auto camera = self->m_renderer->GetActiveCamera();

    // Reset camera to default position and orientation
    camera->SetPosition(0, 0, 1);
    camera->SetFocalPoint(0, 0, 0);
    camera->SetViewUp(0, 1, 0);

    // Reset the camera to fit the scene
    self->m_renderer->ResetCamera();

    vtk_widget_repaint(self);
}

void vtk_widget_repaint(VtkWidgetCpp *self) {
    self->m_needs_repaint = true;
    eframe_request_repaint(self->m_repaint_requester);
}
