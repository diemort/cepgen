file(GLOB sources *.cpp)

include_directories(${PROJECT_SOURCE_DIR})

if(HEPMC_LIB)
  include_directories(${HEPMC_INCLUDE})
  add_library(CepGenExporter SHARED ${sources})
  target_link_libraries(CepGenExporter ${HEPMC_LIB})
  add_definitions(-DHEPMC_LINKED)

  install(TARGETS CepGenExporter DESTINATION lib)
endif()

