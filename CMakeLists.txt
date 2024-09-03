cmake_minimum_required(VERSION 3.0...3.25)
project(
	LearnWebGPU # name of the project, which will also be the name of the visual studio solution if you use it
	VERSION 0.1.0 # any version number
	LANGUAGES CXX C # programming languages used by the project
)


# Add GLFW as a dependency
if (NOT EMSCRIPTEN)
    add_subdirectory(glfw)
else()
    # Emscripten has built-in support for GLFW but requires the `-sUSE_GLFW=3` link option:
    add_library(glfw INTERFACE)
    target_link_options(glfw INTERFACE -sUSE_GLFW=3)
endif()

add_subdirectory(webgpu)

add_subdirectory(glfw3webgpu)

# We specify that we want to create a target of type executable, called "App"
add_executable(App 
	main.cpp
	utils.h
	utils.cpp
	Mesh.cpp
	Mesh.h
	Model.h
	Model.cpp
	SceneObject.cpp
	SceneObject.h
)

target_link_libraries(App PRIVATE glfw webgpu glfw3webgpu)

target_include_directories(App PRIVATE .)

target_copy_webgpu_binaries(App)

target_compile_definitions(App PRIVATE
    RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
)

# Set some recommended properties to configure the version of C++ and treat warnings as errors
set_target_properties(App PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	COMPILE_WARNING_AS_ERROR ON
)

# Also add more warnings, to minimize mistakes
if (MSVC)
	target_compile_options(App PRIVATE /W4)
else()
	target_compile_options(App PRIVATE -Wall -Wextra -pedantic)
endif()

# Generate "schemes" for XCode (macOS) and enable shader debugging (will become handy later on)
if (XCODE)
	set_target_properties(App PROPERTIES
		XCODE_GENERATE_SCHEME ON
		XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
	)
endif()

# Options that are specific to Emscripten
if (EMSCRIPTEN)
    set_target_properties(App PROPERTIES
		SUFFIX ".html"
	)

	# Enable the use of emscripten_sleep()
	target_link_options(App PRIVATE -sASYNCIFY)
endif()

# We add an option to enable different settings when developing the app than
# when distributing it.
option(DEV_MODE "Set up development helper settings" ON)

if(DEV_MODE)
    # In dev mode, we load resources from the source tree, so that when we
    # dynamically edit resources (like shaders), these are correctly
    # versionned.
    target_compile_definitions(App PRIVATE
        RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
    )
else()
    # In release mode, we just load resources relatively to wherever the
    # executable is launched from, so that the binary is portable
    target_compile_definitions(App PRIVATE
        RESOURCE_DIR="./resources"
    )
endif()

target_compile_definitions(App PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_compile_definitions(App PRIVATE GLM_FORCE_LEFT_HANDED)