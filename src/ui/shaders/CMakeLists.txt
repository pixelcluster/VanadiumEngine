cmake_minimum_required (VERSION 3.16)

project ("VanadiumEngine")

vanadium_add_std_vcp_shader("${CMAKE_CURRENT_SOURCE_DIR}/src/ui/shaders/rect.json")
vanadium_add_std_vcp_shader("${CMAKE_CURRENT_SOURCE_DIR}/src/ui/shaders/filledrect.json")
vanadium_add_std_vcp_shader("${CMAKE_CURRENT_SOURCE_DIR}/src/ui/shaders/shadowrect.json")
vanadium_add_std_vcp_shader("${CMAKE_CURRENT_SOURCE_DIR}/src/ui/shaders/text.json")