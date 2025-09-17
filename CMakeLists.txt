cmake_minimum_required(VERSION 3.16)

project(MLInpxPlugin VERSION 1.0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(MLPluginIfc REQUIRED)

find_package(Intl REQUIRED)
find_package(Gettext)

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
