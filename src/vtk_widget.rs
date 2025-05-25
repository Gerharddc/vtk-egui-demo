use eframe::{egui, glow};
use std::ffi::CStr;
use std::{ffi::c_void, os::raw::c_char};

#[allow(improper_ctypes)]
#[link(name = "vtk-egui-demo")]
unsafe extern "C" {
    fn vtk_load_gl(loader: *const EframeGlLoader<'_>);
    fn vtk_new(width: i32, height: i32, requester: *const EframeRepaintRequester);
    fn vtk_destroy();
    fn vtk_paint();
    fn vtk_is_dirty() -> bool;
    fn vtk_mouse_move(x: i32, y: i32);
    fn vtk_update_mouse_down(primary: bool, secondary: bool, middle: bool);
    fn vtk_mouse_wheel(delta: i32);
    fn vtk_set_size(width: i32, height: i32);
}

pub struct EframeRepaintRequester(egui::Context);

#[unsafe(no_mangle)]
pub extern "C" fn eframe_request_repaint(requester: *const EframeRepaintRequester) {
    let requester = unsafe { &*requester };
    requester.0.request_repaint();
}

pub struct VtkWidget {
    fbo: glow::NativeFramebuffer,
    texture: glow::NativeTexture,
    width: i32,
    height: i32,
    egui_texture_id: Option<egui::TextureId>,
    _repaint_requester: Box<EframeRepaintRequester>,
}

pub struct EframeGlLoader<'s> {
    get_proc_address: &'s dyn Fn(&std::ffi::CStr) -> *const std::ffi::c_void,
}

#[unsafe(no_mangle)]
pub extern "C" fn eframe_gl_loader_get_proc_address(
    loader: *const EframeGlLoader<'_>,
    name: *const c_char,
) -> *const c_void {
    let loader = unsafe { &*loader };
    let name = unsafe { CStr::from_ptr(name) };
    (loader.get_proc_address)(name)
}

impl VtkWidget {
    pub fn new(
        gl: &glow::Context,
        get_proc_address: &dyn Fn(&std::ffi::CStr) -> *const std::ffi::c_void,
        ctx: &egui::Context,
    ) -> Self {
        use glow::HasContext as _;

        let width = 300;
        let height = 300;

        let gl_loader = EframeGlLoader { get_proc_address };
        let repaint_requester = Box::new(EframeRepaintRequester(ctx.clone()));

        unsafe {
            vtk_load_gl(&gl_loader);
            vtk_new(width, height, &*repaint_requester);

            let fbo = gl.create_framebuffer().unwrap();
            gl.bind_framebuffer(glow::FRAMEBUFFER, Some(fbo));

            let texture = gl.create_texture().unwrap();
            gl.bind_texture(glow::TEXTURE_2D, Some(texture));
            gl.tex_image_2d(
                glow::TEXTURE_2D,
                0,
                glow::RGBA as i32,
                width,
                height,
                0,
                glow::RGBA,
                glow::UNSIGNED_BYTE,
                glow::PixelUnpackData::Slice(None),
            );
            gl.tex_parameter_i32(
                glow::TEXTURE_2D,
                glow::TEXTURE_MIN_FILTER,
                glow::LINEAR as i32,
            );
            gl.tex_parameter_i32(
                glow::TEXTURE_2D,
                glow::TEXTURE_MAG_FILTER,
                glow::LINEAR as i32,
            );
            gl.framebuffer_texture_2d(
                glow::FRAMEBUFFER,
                glow::COLOR_ATTACHMENT0,
                glow::TEXTURE_2D,
                Some(texture),
                0,
            );

            assert_eq!(
                gl.check_framebuffer_status(glow::FRAMEBUFFER),
                glow::FRAMEBUFFER_COMPLETE
            );

            gl.bind_framebuffer(glow::FRAMEBUFFER, None);

            VtkWidget {
                fbo,
                texture,
                width,
                height,
                egui_texture_id: None,
                _repaint_requester: repaint_requester,
            }
        }
    }

    // It is only safe to call this when eframe exists since it likely owns the texture
    pub unsafe fn destroy(&self, gl: &glow::Context) {
        use glow::HasContext as _;

        unsafe {
            vtk_destroy();
            gl.delete_framebuffer(self.fbo);
            gl.delete_texture(self.texture);
        }
    }

    pub fn paint_if_dirty(&self, gl: &glow::Context) {
        use glow::HasContext as _;

        unsafe {
            if vtk_is_dirty() {
                gl.bind_framebuffer(glow::FRAMEBUFFER, Some(self.fbo));
                vtk_paint();
                gl.bind_framebuffer(glow::FRAMEBUFFER, None);
            }
        }
    }

    pub fn width(&self) -> i32 {
        self.width
    }

    pub fn height(&self) -> i32 {
        self.height
    }

    pub fn texture_id(&mut self, frame: &mut eframe::Frame) -> egui::TextureId {
        // We are handing over ownership of the texture so should take care not to delete it
        *self
            .egui_texture_id
            .get_or_insert(frame.register_native_glow_texture(self.texture))
    }

    pub fn show(&mut self, ui: &mut egui::Ui, vtk_img: egui::Image) {
        let response = ui.add(vtk_img.sense(egui::Sense::all()));

        let current_size = response.rect.size();
        let width = current_size.x as i32;
        let height = current_size.y as i32;

        if width != self.width || height != self.height {
            println!("Updating size: {:#?}", current_size);

            unsafe {
                vtk_set_size(width, height);
            }
            self.width = width;
            self.height = height;
        }

        if response.hovered() {
            if let Some(pos) = ui.input(|i| i.pointer.latest_pos()) {
                let image_rect = response.rect;
                let relative_pos = pos - image_rect.min;
                let x = relative_pos.x as i32;
                let y = relative_pos.y as i32;

                unsafe {
                    vtk_mouse_move(x, y);
                }
            }

            let scroll_delta = ui.input(|i| i.raw_scroll_delta.y as i32);
            if scroll_delta != 0 {
                unsafe {
                    vtk_mouse_wheel(scroll_delta);
                }
            }

            let (primary, secondary, middle) = ui.input(|i| {
                (
                    i.pointer.primary_down(),
                    i.pointer.secondary_down(),
                    i.pointer.middle_down(),
                )
            });

            unsafe { vtk_update_mouse_down(primary, secondary, middle) };
        }
    }
}
