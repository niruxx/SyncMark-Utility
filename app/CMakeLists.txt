cmake_minimum_required(VERSION 3.16)
project(SyncMarkClient VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Core Gui)

file(GLOB_RECURSE SYNCMARK_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE SYNCMARK_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

add_executable(SyncMarkClient WIN32 ${SYNCMARK_SOURCES} ${SYNCMARK_HEADERS})

target_include_directories(SyncMarkClient PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(SyncMarkClient PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Core
    Qt6::Gui
)

if (WIN32)
    set_target_properties(SyncMarkClient PROPERTIES OUTPUT_NAME "SyncMarkClient")

    # Automatically copy the Qt runtime DLLs and plugins next to the exe
    # after every build, so `SyncMarkClient.exe` is runnable directly from
    # the build directory without a manual windeployqt step.
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if (WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET SyncMarkClient POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations $<TARGET_FILE:SyncMarkClient>
            COMMENT "Deploying Qt runtime DLLs next to SyncMarkClient.exe"
        )
    else()
        message(WARNING "windeployqt not found next to Qt6 - run it manually before distributing the exe")
    endif()
endif()

install(TARGETS SyncMarkClient
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)
