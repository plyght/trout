add_executable(TroutIM MACOSX_BUNDLE
    main.mm
    TroutInputController.mm
)

set_target_properties(TroutIM PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    MACOSX_BUNDLE_BUNDLE_NAME "TroutIM"
    MACOSX_BUNDLE_GUI_IDENTIFIER "app.trout.inputmethod.TroutIM"
)

target_include_directories(TroutIM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(TroutIM PRIVATE -fobjc-arc)

target_link_libraries(TroutIM PRIVATE
    "-framework Cocoa"
    "-framework InputMethodKit"
    "-framework Carbon"
)
