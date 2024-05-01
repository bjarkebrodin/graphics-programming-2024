# obtain the list of subdirectories
SUBDIRLIST(SUBDIRS ${CMAKE_CURRENT_LIST_DIR})

FOREACH(subdir ${SUBDIRS})
    if (NOT ${subdir} MATCHES "CMakeFiles")
        add_subdirectory(${subdir})
    ENDIF()
ENDFOREACH()
