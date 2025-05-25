#ifdef __cplusplus
extern "C"
{
#endif
    // Functions defined in Rust
    struct EframeGlLoader;
    //extern void *eframe_gl_loader_get_proc_address(const struct EframeGlLoader *loader, const char *name);
    struct EframeRepaintRequester;
    //extern void eframe_request_repaint(const struct EframeRepaintRequester *requester);

    // Functions defined in C++
    void vtk_load_gl(const struct EframeGlLoader *loader);
    void vtk_new(int width, int height, const struct EframeRepaintRequester *requester);
    void vtk_destroy();
    void vtk_paint();
    bool vtk_is_dirty();
    void vtk_mouse_move(int x, int y);
    void vtk_update_mouse_down(bool primary, bool secondary, bool middle);
    void vtk_mouse_wheel(int delta);
    void vtk_set_size(int width, int height);
#ifdef __cplusplus
}
#endif