cmake_minimum_required(VERSION 3.25)

project(fd-gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Concurrent Test)

qt_standard_project_setup()
qt_policy(SET QTP0001 NEW)
enable_testing()

qt_add_executable(fd-gui
    src/main.cpp
    src/dmstheme.cpp
    src/fdsearch.cpp
    src/launcher.cpp
    src/previewprovider.cpp
    src/resultmodel.cpp
    src/settingsstore.cpp
    src/statworker.cpp
)

qt_add_qml_module(fd-gui
    URI FdGui
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/PreviewPane.qml
        qml/ResultDelegate.qml
    SOURCES
        src/dmstheme.h
        src/fdsearch.h
        src/launcher.h
        src/previewprovider.h
        src/resultmodel.h
        src/settingsstore.h
        src/statworker.h
)

target_include_directories(fd-gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(fd-gui PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Concurrent
)

set_target_properties(fd-gui PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

include(GNUInstallDirs)

install(TARGETS fd-gui
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES fd-gui.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

qt_add_executable(backend-tests
    tests/backendtest.cpp
    src/resultmodel.cpp
    src/resultmodel.h
    src/settingsstore.cpp
    src/settingsstore.h
)

target_include_directories(backend-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(backend-tests PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME backend-tests COMMAND backend-tests)
