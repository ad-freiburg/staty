set(OSMFIXER_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_BINARY_DIR})

if (COMMAND cpplint_add_subdirectory)
	cpplint_add_subdirectory(osmfixer)
endif()

include_directories(
	${OSMFIXER_INCLUDE_DIR}
)

add_subdirectory(util)
add_subdirectory(staty)
