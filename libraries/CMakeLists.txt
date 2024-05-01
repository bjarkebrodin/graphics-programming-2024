# obtain the list of subdirectories
SUBDIRLIST(SUBDIRS ${CMAKE_CURRENT_LIST_DIR})

FOREACH(subdir ${SUBDIRS})
    if(NOT ${subdir} MATCHES "CMakeLists")
        add_subdirectory(${subdir})
    endif()
ENDFOREACH()
