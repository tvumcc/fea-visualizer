cmake_minimum_required(VERSION 3.23)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(fea_visualizer LANGUAGES C CXX)

add_subdirectory("lib/glfw")
find_package(OpenGL REQUIRED)
include_directories(
	${OPENGL_INCLUDE_DIRS} 
	${CMAKE_CURRENT_SOURCE_DIR}/include 
	${CMAKE_CURRENT_SOURCE_DIR}/lib 
	${CMAKE_CURRENT_SOURCE_DIR}/lib/glad/include 
	${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw/include
	${CMAKE_CURRENT_SOURCE_DIR}/lib/glm
	${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui
	${CMAKE_CURRENT_SOURCE_DIR}/lib/tinyobjloader
	${CMAKE_CURRENT_SOURCE_DIR}/lib/eigen
	${CMAKE_CURRENT_SOURCE_DIR}/lib/nativefiledialog/src/include
	${CMAKE_CURRENT_SOURCE_DIR}/lib/nativefiledialog/src/
)

add_compile_definitions(NO_TIMER ANSI_DECLARATORS TRILIBRARY)

file(GLOB_RECURSE SRC_FILES src/*.cpp)

# See https://github.com/mlabbe/nativefiledialog?tab=readme-ov-file#compiling-your-programs for more details
if(WIN32)
	set(NFD_OS_FILE lib/nativefiledialog/src/nfd_win.cpp)
elseif(APPLE)
    enable_language(OBJC)
    set(NFD_OS_FILE lib/nativefiledialog/src/nfd_cocoa.m)

	find_library(APPKIT_FRAMEWORK AppKit)
    find_library(FOUNDATION_FRAMEWORK Foundation)
elseif(UNIX)
	set(NFD_OS_FILE lib/nativefiledialog/src/nfd_zenity.c)
else()
	message("Can't determine which OS to compile nativefiledialog for.")
endif()

add_executable(${CMAKE_PROJECT_NAME}
	${SRC_FILES}

	# nativefiledialog
	lib/nativefiledialog/src/nfd_common.c
	${NFD_OS_FILE}

	# GLAD
	lib/glad/src/glad.c
	lib/stb/stb_image.cpp

	# ImGui
	lib/imgui/imgui.cpp
	lib/imgui/imgui_demo.cpp
	lib/imgui/imgui_draw.cpp
	lib/imgui/imgui_tables.cpp
	lib/imgui/imgui_widgets.cpp
	lib/imgui/backends/imgui_impl_glfw.cpp
	lib/imgui/backends/imgui_impl_opengl3.cpp

    # Triangle
    lib/triangle/triangle.cpp

    # Tinyobjloader
    lib/tinyobjloader/tiny_obj_loader.cc
)

if(APPLE)
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "FEA Visualizer"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_IDENTIFIER "com.tvumcc.feavisualizer"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.tvumcc.feavisualizer"
        MACOSX_BUNDLE_EXECUTABLE_NAME "fea_visualizer"
        MACOSX_DEPLOYMENT_TARGET "10.15"
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "@executable_path/../Frameworks"
    )

	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.png")
		add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy 
			${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.png 
			"$<TARGET_BUNDLE_CONTENT_DIR:${CMAKE_PROJECT_NAME}>/Resources/icon.png"
			COMMENT "Copying icon.png to bundle"
		)
		set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES 
			MACOSX_BUNDLE_ICON_FILE icon.png
		)
	endif()

    # Copy resource folders into the app bundle
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets "$<TARGET_BUNDLE_CONTENT_DIR:${CMAKE_PROJECT_NAME}>/Resources/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders "$<TARGET_BUNDLE_CONTENT_DIR:${CMAKE_PROJECT_NAME}>/Resources/shaders"
        COMMENT "Copying resource directories to bundle"
    )
endif()

# For Windows and Linux, copy resources next to the executable
if(WIN32 OR (UNIX AND NOT APPLE))
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/shaders"
        COMMENT "Copying resource directories"
    )
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${OPENGL_LIBRARIES} glfw)
if (APPLE)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${APPKIT_FRAMEWORK} ${FOUNDATION_FRAMEWORK})
endif()