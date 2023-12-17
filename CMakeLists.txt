cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TARGET "simpleusdview")
add_definitions(-DNOMINMAX)
add_definitions(-DTBB_USE_DEBUG)

project(
    ${TARGET}
    DESCRIPTION "Simple GLFW usdview"
    LANGUAGES CXX C
)

set(MY_CXX_FLAGS "/MD /O2 /W3" )
set(MY_LINK_FLAGS "/MANIFEST:NO" )

set(CMAKE_CXX_FLAGS_DEBUG "${MY_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${MY_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${MY_LINK_FLAGS}" )
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${MY_LINK_FLAGS}" )

set(CMAKE_POSITION_INDEPENDENT_CODE ON) # fPIC

# Discover cmake modules.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake )

find_package(OpenGL REQUIRED)

# Find python libraries.
set(Python_ADDITIONAL_VERSIONS 3.11)
find_package(Python EXACT 3.11 COMPONENTS Interpreter Development)
include_directories( ${Python_INCLUDE_DIRS} )
message(STATUS "Python_FOUND:${Python_FOUND}")
message(STATUS "Python_VERSION:${Python_VERSION}")
message(STATUS "Python_Development_FOUND:${Python_Development_FOUND}")
message(STATUS "Python_LIBRARIES:${Python_LIBRARIES}")

set( USD_ROOT "C:\\dev\\usd-23.11" )
get_filename_component(USD_INCLUDE_DIR ${USD_ROOT}/include ABSOLUTE)
get_filename_component(USD_LIBRARY_DIR ${USD_ROOT}/lib ABSOLUTE)
file( STRINGS ${USD_INCLUDE_DIR}/pxr/pxr.h TMP REGEX "^#define PXR_VERSION .*$" )
string( REGEX MATCHALL "[0-9]+" USD_VERSION ${TMP} )
message(STATUS "USD_ROOT:${USD_ROOT}")
message(STATUS "USD_VERSION:${USD_VERSION}")
message(STATUS "USD_INCLUDE_DIR:${USD_INCLUDE_DIR}")
message(STATUS "USD_LIB_DIR:${USD_LIB_DIR}")
set(USD_LIBRARY_MONOLITHIC FALSE)
set(PXR_LIB_PREFIX "")
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(USD
    REQUIRED_VARS
        USD_INCLUDE_DIR
        USD_LIBRARY_DIR
    VERSION_VAR
        USD_VERSION
)

set(BOOST_FROM_USD 1_78)
set(TBB_ROOT_DIR ${USD_ROOT} )
set(BOOST_ROOT "${USD_ROOT}/include/boost-${BOOST_FROM_USD}" )
find_package(Boost REQUIRED)
set(boost_version_string "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")
message(STATUS "Boost_VERSION:${Boost_VERSION}") 
include_directories( ${BOOST_ROOT} )

set (_usd_libs
    usd_usdImagingGL
    usd_usdImaging
    usd_usdHydra
    usd_hdx
    usd_hdSt
    usd_hd
    usd_glf
    usd_garch
    usd_pxOsd
    usd_usdRi
    usd_usdUI
    usd_usdShade
    usd_usdGeom
    usd_usd
    usd_usdUtils
    usd_pcp
    usd_sdf
    usd_plug
    usd_js
    usd_ar
    usd_work
    usd_tf
    usd_kind
    usd_arch
    usd_vt
    usd_gf
    usd_hf
    usd_cameraUtil
    usd_usdLux
)

include_directories(
    ${USD_INCLUDE_DIR}
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory( "../glfw" "../glfw" )

find_package(TBB REQUIRED)

add_executable( 
    ${TARGET}
    main.cpp
)

target_link_directories(
    ${TARGET}
    PRIVATE
    ${USD_LIBRARY_DIR}
)

target_link_libraries(
    ${TARGET}  
    OpenGL::GL
    glfw
    ${Python_LIBRARIES}
    ${_usd_libs}
)

install(TARGETS ${TARGET} RUNTIME DESTINATION bin)

