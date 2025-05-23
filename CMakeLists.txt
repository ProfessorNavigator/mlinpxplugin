cmake_minimum_required(VERSION 3.16)

project(MLInpxPlugin VERSION 1.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

try_compile(OMP_TEST "${CMAKE_BINARY_DIR}/omp_test" "${PROJECT_SOURCE_DIR}/omp_test" OmpTest
    CMAKE_FLAGS "-DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}")
if(OMP_TEST)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_OPENMP")
  message(STATUS "${PROJECT_NAME} will be built with OpenMP support")
else()
  message(STATUS "${PROJECT_NAME} will be built without OpenMP support.")
endif()

find_package(MLPluginIfc REQUIRED)

find_package(Intl REQUIRED)
find_package(Gettext)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)

if(GTKMM_VERSION VERSION_LESS "4.10")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DML_GTK_OLD")
endif()

add_library(mlinpxplugin SHARED)

set_target_properties(mlinpxplugin PROPERTIES POSITION_INDEPENDENT_CODE True)

set_target_properties(mlinpxplugin PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
)

add_subdirectory(src)
add_subdirectory(include)

target_include_directories(mlinpxplugin
    PRIVATE include
    PRIVATE MLPluginIfc::mlpluginifc
    PRIVATE MLBookProc::mlbookproc
)

target_link_libraries(mlinpxplugin
    PRIVATE MLPluginIfc::mlpluginifc
    PRIVATE MLBookProc::mlbookproc
)

include(GNUInstallDirs)

install(TARGETS mlinpxplugin EXPORT "${PROJECT_NAME}Targets"
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(Gettext_FOUND)
   GETTEXT_PROCESS_PO_FILES("ru" ALL
   INSTALL_DESTINATION "${CMAKE_INSTALL_LOCALEDIR}"
   PO_FILES "po/${PROJECT_NAME}.po"
)
endif()
