cmake_minimum_required(VERSION 3.10)
project(MeshViewer)

set(CMAKE_CXX_STANDARD 17)

set(OpenGL_GL_PREFERENCE LEGACY)

find_package(GLUT REQUIRED)
find_package(glm REQUIRED)

find_package(OpenGL REQUIRED)
if(OPENGL_FOUND)
    include_directories(${OpenGL_INCLUDE_DIRS})
    link_directories(${OpenGL_LIBRARY_DIRS})
    add_definitions(${OpenGL_DEFINITIONS})
else()
    message(ERROR " OPENGL not found!")
endif()

find_package(GLUT REQUIRED)
if(GLUT_FOUND)
    include_directories(${GLUT_INCLUDE_DIRS})
else()
    message(ERROR " GLUT not found!")
endif()

FIND_PACKAGE(GLEW REQUIRED)
if(GLEW_FOUND)
    include_directories( ${GLEW_INCLUDE_PATH})
else()
    MESSAGE("GLEW not found!")
endif()

set(SOURCE_FILES main.cpp
                 myHalfedge.cpp
                 myVector3D.cpp
                 myPoint3D.cpp
                 myFace.cpp
                 myMesh.cpp
                 myVertex.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${GLUT_LIBRARIES} glm::glm)

# Copy shaders to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders)

