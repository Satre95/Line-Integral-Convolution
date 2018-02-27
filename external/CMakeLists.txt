add_subdirectory(of)
set(OF_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/of)
set(OF_INCLUDE_DIRS
    "${OF_ROOT}/src/openframeworks"
    "${OF_ROOT}/src/openframeworks/3d"
    "${OF_ROOT}/src/openframeworks/app"
    "${OF_ROOT}/src/openframeworks/communication"
    "${OF_ROOT}/src/openframeworks/events"
    "${OF_ROOT}/src/openframeworks/gl"
    "${OF_ROOT}/src/openframeworks/graphics"
    "${OF_ROOT}/src/openframeworks/math"
    "${OF_ROOT}/src/openframeworks/sound"
    "${OF_ROOT}/src/openframeworks/types"
    "${OF_ROOT}/src/openframeworks/utils"
    "${OF_ROOT}/src/openframeworks/video"
	PARENT_SCOPE
)
