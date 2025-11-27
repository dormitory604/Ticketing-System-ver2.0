# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\client-app_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\client-app_autogen.dir\\ParseCache.txt"
  "client-app_autogen"
  )
endif()
