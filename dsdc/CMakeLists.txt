# Copyright OkCupid 2016

include(${CMAKE_SOURCE_DIR}/CMakeHelpers.cmake)

set(SRC main.C
	output.C)

set(TAMED_SRC admin.T
	      aiod2.T
	      master.T
	      proxy.T)

set(HEADERS "")

PreprocessTamedFiles(SRC HEADERS "${TAMED_SRC}")
add_custom_target(dsdc_headers DEPENDS ${HEADERS})

include_directories(${GLOBAL_INCLUDES}
                    ${DSDC_BINARY_DIR}/libdsdc/
                    ${DSDC_BINARY_DIR}/../okws/libpub/
                    ${DSDC_SOURCE_DIR}/../okws/libpub/
                    ${DSDC_SOURCE_DIR}/libdsdc/
		    ${CMAKE_CURRENT_SOURCE_DIR}/
		    ${CMAKE_CURRENT_BINARY_DIR}/)

link_directories(${GLOBAL_LINKS}
                 ${DSDC_BINARY_DIR}/../okws/libpub/
                 /usr/local/lib/sfslite-1.2/shopt/
                 /opt/stmd/lib/)

set(LINK_LIBS libpub libdsdc async sfsmisc sfscrypt tame arpc gmp ssl pcrecpp z crypto bsd icui18n icudata stmd sass expat gmock snappy)

add_executable(bin_dsdc ${CMAKE_CURRENT_BINARY_DIR}/master.cxx ${CMAKE_CURRENT_BINARY_DIR}/proxy.cxx main.C)
target_link_libraries(bin_dsdc PUBLIC ${LINK_LIBS})
add_executable(aiod2 ${CMAKE_CURRENT_BINARY_DIR}/aiod2.cxx)
target_link_libraries(aiod2 PUBLIC ${LINK_LIBS})

add_dependencies(bin_dsdc libpub libdsdc libdsdc_headers)
add_dependencies(aiod2 libpub libdsdc libdsdc_headers)
