cmake_minimum_required(VERSION 3.28)
project(vtk-egui-demo)

include(FetchContent)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG v0.5
)
FetchContent_MakeAvailable(Corrosion)

find_package(VTK
 COMPONENTS
    CommonCore
    RenderingCore
    InteractionStyle
    RenderingOpenGL2
)

add_executable(vtk-egui-demo src/vtk_widget.cpp)

target_include_directories(vtk-egui-demo PRIVATE include)

target_link_libraries(vtk-egui-demo PRIVATE ${VTK_LIBRARIES})

# Windows libraries for Rust
target_link_libraries(vtk-egui-demo PRIVATE uxtheme Imm32 Dwmapi uiautomationcore oleaut32 propsys RuntimeObject Shlwapi)

corrosion_import_crate(MANIFEST_PATH Cargo.toml)
target_link_libraries(vtk-egui-demo PRIVATE rs_lib)

vtk_module_autoinit(
    TARGETS vtk-egui-demo
    MODULES ${VTK_LIBRARIES}
)
