#include <vtk_glad.h>
#include <vtkActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCubeSource.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkGenericRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>

#include "vtk_widget.h"

namespace
{
    const EframeGlLoader *gl_loader = nullptr;
    const EframeRepaintRequester *repaint_requester = nullptr;
    vtkGenericOpenGLRenderWindow *render_window = nullptr;
    vtkGenericRenderWindowInteractor *interactor = nullptr;

    bool gl_loaded = false;
    bool is_dirty = true;
    bool primary_down = false;
    bool secondary_down = false;
    bool middle_down = false;
    int window_width, window_height;

    void IsCurrentCallback(vtkObject *vtkNotUsed(caller), long unsigned int vtkNotUsed(eventId),
                           void *vtkNotUsed(clientData), void *callData)
    {
        // We always make sure to have the context for VTK active before calling render on it
        *(static_cast<bool *>(callData)) = true;
    }

    void FrameCallback(vtkObject *vtkNotUsed(caller), long unsigned int vtkNotUsed(eventId),
                       void *vtkNotUsed(clientData), void *vtkNotUsed(callData))
    {
        assert(repaint_requester != nullptr);
        is_dirty = true;
        eframe_request_repaint(repaint_requester);
    }

    GLADapiproc gl_get_proc_address(const char *name)
    {
        assert(gl_loader != nullptr);
        return static_cast<GLADapiproc>(eframe_gl_loader_get_proc_address(gl_loader, name));
    }
} // end anon namespace

void vtk_load_gl(const EframeGlLoader *loader)
{
    gl_loader = loader;
    gladLoadGL(&gl_get_proc_address);

    // The loader has a limited lifetime so we need to discard our pointer
    gl_loader = nullptr;
    gl_loaded = true;
}

void vtk_new(int width, int height, const EframeRepaintRequester *requester)
{
    vtkLogScopeFunction(INFO);
    vtkLogScopeF(INFO, "do-vtk-new");

    assert(gl_loaded);

    window_width = width;
    window_height = height;
    repaint_requester = requester;

    vtkNew<vtkRenderer> renderer;
    renderer->ResetCamera();
    renderer->SetAutomaticLightCreation(true);

    assert(render_window == nullptr);
    render_window = vtkGenericOpenGLRenderWindow::New();
    render_window->AddRenderer(renderer);
    render_window->SetSize(width, height);

    vtkNew<vtkCallbackCommand> is_current_cb;
    is_current_cb->SetCallback(IsCurrentCallback);
    render_window->AddObserver(vtkCommand::WindowIsCurrentEvent, is_current_cb);

    vtkNew<vtkCallbackCommand> frame_cb;
    frame_cb->SetCallback(FrameCallback);
    render_window->AddObserver(vtkCommand::WindowFrameEvent, frame_cb);

    assert(interactor == nullptr);
    interactor = vtkGenericRenderWindowInteractor::New();
    interactor->SetRenderWindow(render_window);

    vtkNew<vtkInteractorStyleTrackballCamera> style;
    interactor->SetInteractorStyle(style);

    vtkNew<vtkActor> actor;
    renderer->AddActor(actor);
    actor->RotateX(45.0);
    actor->RotateY(45.0);
    actor->GetProperty()->SetColor(0.8, 0.2, 0.2);

    vtkNew<vtkPolyDataMapper> mapper;
    actor->SetMapper(mapper);

    vtkNew<vtkCubeSource> cs;
    mapper->SetInputConnection(cs->GetOutputPort());
}

void vtk_destroy()
{
    assert(interactor != nullptr);
    assert(render_window != nullptr);

    interactor->Delete();
    render_window->Delete();

    interactor = nullptr;
    render_window = nullptr;
    repaint_requester = nullptr;
}

void vtk_paint()
{
    vtkLogScopeFunction(INFO);
    vtkLogScopeF(INFO, "do-vtk-render");

    assert(render_window != nullptr);
    render_window->Render();
    is_dirty = false;
}

bool vtk_is_dirty()
{
    return is_dirty;
}

void vtk_mouse_move(int x, int y)
{
    assert(interactor != nullptr);
    interactor->SetEventPosition(x, y);
    interactor->InvokeEvent(vtkCommand::MouseMoveEvent);
}

void vtk_update_mouse_down(bool primary, bool secondary, bool middle)
{
    assert(interactor != nullptr);

    if (primary_down != primary)
    {
        if (primary)
            interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent);
        else
            interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent);

        primary_down = primary;
    }

    if (secondary_down != secondary)
    {
        if (secondary)
            interactor->InvokeEvent(vtkCommand::RightButtonPressEvent);
        else
            interactor->InvokeEvent(vtkCommand::RightButtonReleaseEvent);

        secondary_down = secondary;
    }

    if (middle_down != middle)
    {
        if (middle)
            interactor->InvokeEvent(vtkCommand::MiddleButtonPressEvent);
        else
            interactor->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent);

        middle_down = middle;
    }
}

void vtk_mouse_wheel(int delta)
{
    assert(interactor != nullptr);

    if (delta > 0)
    {
        interactor->InvokeEvent(vtkCommand::MouseWheelForwardEvent);
    }
    else if (delta < 0)
    {
        interactor->InvokeEvent(vtkCommand::MouseWheelBackwardEvent);
    }
}

void vtk_set_size(int width, int height)
{
    assert(render_window != nullptr);

    window_width = width;
    window_height = height;
    render_window->SetSize(width, height);

    if (interactor)
    {
        interactor->SetSize(width, height);
    }
}