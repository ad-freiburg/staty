file(GLOB_RECURSE staty_SRC *.cpp)

set(staty_main FixerMain.cpp)

list(REMOVE_ITEM staty_SRC ${staty_main})

include_directories(
	${staty_INCLUDE_DIR}
)

add_executable(staty ${staty_main})
add_library(staty_dep ${staty_SRC})

add_custom_command(
	OUTPUT index.h
	COMMAND cd ${PROJECT_BINARY_DIR}/../web/ && xxd -i index.html > ${CMAKE_CURRENT_BINARY_DIR}/index.h
	DEPENDS "${PROJECT_BINARY_DIR}/../web/index.html"
	VERBATIM
)

add_custom_command(
	OUTPUT build.h
	COMMAND cd ${PROJECT_BINARY_DIR}/../web/ && cat leaflet.js leaflet-heat.js script.js > merged.js && java -jar closurec/compiler.jar -O SIMPLE leaflet.js leaflet-heat.js script.js > build.js && xxd -i build.js > ${CMAKE_CURRENT_BINARY_DIR}/build.h && rm build.js
	DEPENDS "${PROJECT_BINARY_DIR}/../web/script.js"
	DEPENDS "${PROJECT_BINARY_DIR}/../web/leaflet-heat.js"
	VERBATIM
)

add_custom_target(htmlfiles DEPENDS index.h build.h)

add_dependencies(staty_dep htmlfiles)

target_link_libraries(staty staty_dep util ${Boost_LIBRARIES} -lpthread)
