cmake_minimum_required(VERSION 3.19)
project(WhisperFlow LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Svg LinguistTools)

qt_standard_project_setup()

qt_add_executable(WhisperFlow
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    theme.h
    theme.cpp
    icons.h
    icons.cpp
    recordbutton.h
    recordbutton.cpp
    transcriptcard.h
    transcriptcard.cpp
)

qt_add_translations(
    TARGETS WhisperFlow
    TS_FILES WhisperFlow_en_AU.ts
)

target_link_libraries(WhisperFlow
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Svg
)

install(TARGETS WhisperFlow
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET WhisperFlow
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
