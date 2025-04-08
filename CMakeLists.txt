cmake_minimum_required(VERSION 3.16)

project(MLPluginInpx VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_OPENMP "Build with openmp support" OFF)

if(USE_OPENMP)
  find_package(OpenMP REQUIRED)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_OPENMP ${OpenMP_CXX_FLAGS}")
  add_link_options("${OpenMP_CXX_FLAGS}")
endif()

find_package(MLPluginIfc REQUIRED)

find_package(Intl REQUIRED)
find_package(Gettext)

if(GTKMM_VERSION VERSION_LESS "4.10")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DML_GTK_OLD")
endif()

add_library(mlplugininpx SHARED)

set_target_properties(mlplugininpx PROPERTIES POSITION_INDEPENDENT_CODE ON)

set_target_properties(mlplugininpx PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
)

add_subdirectory(src)
add_subdirectory(include)

target_include_directories(mlplugininpx
    PRIVATE include
    PRIVATE MLPluginIfc::mlpluginifc
)

target_link_libraries(mlplugininpx
    PRIVATE MLPluginIfc::mlpluginifc
    PRIVATE atomic
)

include(GNUInstallDirs)

install(TARGETS mlplugininpx EXPORT MLPluginInpxTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(Gettext_FOUND)
   GETTEXT_PROCESS_PO_FILES("ru" ALL
   INSTALL_DESTINATION "${CMAKE_INSTALL_LOCALEDIR}"
   PO_FILES po/MLInpxPlugin.po
)
endif()
