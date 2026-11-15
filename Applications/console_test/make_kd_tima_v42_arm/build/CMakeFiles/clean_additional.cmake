# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "make_kd_tima_v42_arm.bin"
  "make_kd_tima_v42_arm.elf"
  "make_kd_tima_v42_arm.lst"
  "make_kd_tima_v42_armmap"
  )
endif()
