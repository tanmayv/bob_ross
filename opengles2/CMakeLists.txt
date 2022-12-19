LIST(APPEND SOURCES 
  "src/bob_ross.cc")
# find_library(GLESv3_LIBRARY NAMES GLESv3 GLESv2)
add_library(bob_ross_gles3 SHARED ${SOURCES})
target_include_directories(bob_ross_gles3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(bob_ross_gles3 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# target_link_libraries(bob_ross_gles3 bob_ross_interface ${GLESv3_LIBRARY})
target_link_libraries(bob_ross_gles3 bob_ross_interface GLESv3)

