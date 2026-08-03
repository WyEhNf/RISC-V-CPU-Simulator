cmake_minimum_required(VERSION 3.22)
project(sim LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_executable(sim apps/sim.cpp)
target_include_directories(sim PRIVATE include)

include(CTest)
if(BUILD_TESTING)
  set(UNIT_TESTS
      test_m0
      test_reference
      test_predictor
      test_cdb
      test_regfile
      test_regfile_state
      test_rob
      test_rob_state
      test_rs
      test_rs_state
      test_lsq
      test_lsq_state
      test_pipeline
      test_tomasulo)

  foreach(test_name IN LISTS UNIT_TESTS)
    add_executable(${test_name} tests/unit/${test_name}.cpp)
    target_include_directories(${test_name} PRIVATE include)
    add_test(NAME ${test_name} COMMAND ${test_name})
  endforeach()

  add_executable(test_data tests/integration/test_data.cpp)
  target_include_directories(test_data PRIVATE include)
  target_compile_definitions(test_data PRIVATE
      NEWIMPL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
  add_test(NAME test_data COMMAND test_data)
  set_tests_properties(test_data PROPERTIES TIMEOUT 240 LABELS "integration;slow")
endif()
