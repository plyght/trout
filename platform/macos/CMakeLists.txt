set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_metal.mm
    ${IMGUI_DIR}/backends/imgui_impl_osx.mm
)

set(MACOS_SOURCES
    ${CMAKE_SOURCE_DIR}/app/main.mm
    Permissions.mm
    Accessibility.mm
    EventTap.mm
    Overlay.mm
    MenuBarController.mm
    AppController.mm
    SettingsWindow.mm
)

add_executable(Trout MACOSX_BUNDLE ${MACOS_SOURCES} ${IMGUI_SOURCES})

set_target_properties(Trout PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/resources/Info.plist.in
    MACOSX_BUNDLE_BUNDLE_NAME "Trout"
    MACOSX_BUNDLE_GUI_IDENTIFIER "app.trout.Trout"
)

target_include_directories(Trout PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_compile_options(Trout PRIVATE -fobjc-arc)

target_link_libraries(Trout PRIVATE trout-core
    "-framework Cocoa"
    "-framework Metal"
    "-framework MetalKit"
    "-framework QuartzCore"
    "-framework Carbon"
    "-framework ApplicationServices"
    "-framework CoreGraphics"
    "-framework GameController"
)
