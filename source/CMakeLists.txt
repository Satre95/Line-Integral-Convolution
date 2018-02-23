set(SOURCE_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/VortexSimulationApp.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/Vorton.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/UniformGrid.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/NestedGrid.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/UniformGridMath.cpp
    PARENT_SCOPE
)

set(HEADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/math_helper.hpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/Vorton.hpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/UniformGrid.hpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/NestedGrid.hpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/Particle.hpp    
    ${CMAKE_CURRENT_SOURCE_DIR}/UniformGridMath.hpp
    PARENT_SCOPE
)
    
