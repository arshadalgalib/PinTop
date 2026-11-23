cmake_minimum_required(VERSION 3.20)
project(PinTop LANGUAGES CXX)
if(NOT WIN32)
  message(FATAL_ERROR "PinTop is a Windows-only Win32 application.")
endif()

add_library(PinTopHook SHARED PinTopHook.cpp)
target_compile_features(PinTopHook PRIVATE cxx_std_17)
target_compile_definitions(PinTopHook PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)
target_link_libraries(PinTopHook PRIVATE user32)

add_executable(PinTop WIN32 PinTop.cpp)
target_compile_features(PinTop PRIVATE cxx_std_17)
target_compile_definitions(PinTop PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)
target_link_libraries(PinTop PRIVATE user32 shell32 advapi32)
add_dependencies(PinTop PinTopHook)
add_custom_command(TARGET PinTop POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:PinTopHook> $<TARGET_FILE_DIR:PinTop>/PinTopHook.dll)

if(MSVC)
  foreach(target PinTop PinTopHook)
    target_compile_options(${target} PRIVATE /O1 /W4 /MT)
    target_link_options(${target} PRIVATE /OPT:REF /OPT:ICF)
  endforeach()
endif()
