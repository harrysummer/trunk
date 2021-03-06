cmake_minimum_required(VERSION 2.8)

# to compile CCLib only! (CMake implicitly imposes to declare a project before anything...)
project( CC_DUMMY_PROJECT )

# CC_CORE_LIB only relies on 'triangle' library...
add_subdirectory (triangle)
# ... and QT!
set( DESIRED_QT_VERSION 4 )
find_package( Qt4 ${QT_VERSION} COMPONENTS QtCore REQUIRED )
if( NOT QT_FOUND )
    message( SEND_ERROR "Qt required, but not found with 'find_package()'" )
else()
    include( ${QT_USE_FILE} )
endif()

project( CC_CORE_LIB )

set( VERSION_MAJOR 1 )
set( VERSION_MINOR 0 )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include )
if( MSVC )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include/msvc )
endif()
include_directories( ${triangle_SOURCE_DIR} )

file( GLOB header_list include/*.h)
file( GLOB source_list src/*.cpp src/*.h)

add_library( ${PROJECT_NAME} SHARED ${header_list} ${source_list} )

target_link_libraries( ${PROJECT_NAME} triangle )
target_link_libraries( ${PROJECT_NAME} ${QT_LIBRARIES} )

# Load advanced scripts
include( ../CMakeInclude.cmake )

# Default preprocessors
set_default_cc_preproc( ${PROJECT_NAME} )

if (WIN32)
	set_property( TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_DEFINITIONS CC_USE_AS_DLL )
endif()

# Install (shared) library to specified destinations
if( UNIX )
	install_shared( CC_CORE_LIB lib lib ) #default destination: /usr/lib
else()
	foreach( dest ${INSTALL_DESTINATIONS} )
		install_shared( CC_CORE_LIB ${dest} ${dest}_debug )
	endforeach()
endif()
