#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")] // hide console window on Windows in release

use eframe::{egui, glow};

mod vtk_widget;
use vtk_widget::VtkWidget;

fn main() {
    env_logger::init(); // Log to stderr (if you run with `RUST_LOG=debug`).
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default().with_inner_size([350.0, 380.0]),
        multisampling: 4,
        renderer: eframe::Renderer::Glow,
        ..Default::default()
    };
    eframe::run_native(
        "Eframe with VTK Widget",
        options,
        Box::new(|cc| Ok(Box::new(MyApp::new(cc)))),
    )
    .unwrap();
}

struct MyApp {
    vtk_widget: VtkWidget,
}

impl MyApp {
    fn new(cc: &eframe::CreationContext<'_>) -> Self {
        let gl = cc
            .gl
            .as_ref()
            .expect("You need to run eframe with the glow backend");

        let get_proc_address = cc
            .get_proc_address
            .expect("You need to run eframe with the glow backend");

        Self {
            vtk_widget: VtkWidget::new(gl, get_proc_address, &cc.egui_ctx),
        }
    }
}

impl eframe::App for MyApp {
    fn update(&mut self, ctx: &egui::Context, frame: &mut eframe::Frame) {
        use egui::load::SizedTexture;

        self.vtk_widget.paint_if_dirty(frame.gl().unwrap());

        // TODO: think how DPI should affect the actual texture size

        let vtk_img = egui::Image::from_texture(SizedTexture::new(
            self.vtk_widget.texture_id(frame),
            [
                self.vtk_widget.width() as f32,
                self.vtk_widget.height() as f32,
            ],
        ));

        egui::CentralPanel::default().show(ctx, |ui| {
            ui.horizontal(|ui| {
                ui.spacing_mut().item_spacing.x = 0.0;
                ui.label("This cube is being painted using ");
                ui.hyperlink_to("VTK", "https://vtk.org/");
                ui.label(" (in C++).");
            });

            egui::Frame::canvas(ui.style()).show(ui, |ui| {
                self.vtk_widget.show(ui, vtk_img);
            });

            ui.label("Drag to rotate, wheel to zoom!");
        });
    }

    fn on_exit(&mut self, gl: Option<&glow::Context>) {
        if let Some(gl) = gl {
            unsafe { self.vtk_widget.destroy(gl) }
        }
    }
}
