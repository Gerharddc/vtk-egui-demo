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

add_library(vtk-egui-demo SHARED src/vtk_widget.cpp)

target_include_directories(vtk-egui-demo PRIVATE include)

target_link_libraries(vtk-egui-demo PRIVATE ${VTK_LIBRARIES})

corrosion_import_crate(MANIFEST_PATH Cargo.toml)
corrosion_link_libraries(rs_lib vtk-egui-demo)

vtk_module_autoinit(
    TARGETS vtk-egui-demo
    MODULES ${VTK_LIBRARIES}
)
