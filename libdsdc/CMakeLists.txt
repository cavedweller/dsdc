# Copyright OkCupid 2016

include(${DSDC_SOURCE_DIR}/CMakeHelpers.cmake)

set(LIBPROT_C_SRC "")
set(LIBPROT_H_SRC "")
set(LIBPROT_XML_C_SRC "")
set(LIBPROT_XML_H_SRC "")

#set(XML_PROT_FILES "")
set(XML_PROT_FILES dsdc_prot.x)

set(SRC const.C
	dsdc_util.C
	fslru.C
	lock.C
        #match.C
	ring.C
	smartcli_mget.C
	stats1.C
	stats2.C
	stats.C)

set(TAMED_SRC aiod2_client.T
	      fscache.T
	      slave.T
	      smartcli.T
	      state.T
	      thback.T
	      dsdc_tamed.Th
	      fslru.Th)

set(HEADERS "")

PreprocessTamedFiles(SRC HEADERS "${TAMED_SRC}")
DSDCPreprocessXmlProtFiles(LIBPROT_XML_C_SRC LIBPROT_XML_H_SRC "${XML_PROT_FILES}")
message(${LIBPROT_XML_H_SRC})
add_custom_target(libdsdc_headers DEPENDS ${LIBPROT_XML_H_SRC} ${HEADERS})

include_directories(${GLOBAL_INCLUDES}
                    ${DSDC_BINARY_DIR}/../okws/libokxml/
                    ${DSDC_SOURCE_DIR}/../okws/libokxml/
                    ${DSDC_SOURCE_DIR}/../okws/libpub/
                    ${DSDC_BINARY_DIR}/../okws/libpub/
                    ${DSDC_SOURCE_DIR}/../okws/libaok/
                    ${DSDC_BINARY_DIR}/../okws/libaok/
		    ${CMAKE_CURRENT_SOURCE_DIR}/
		    ${CMAKE_CURRENT_BINARY_DIR}/)

add_library(libdsdc SHARED ${LIBPROT_XML_C_SRC} ${LIBPROT_XML_H_SRC} ${SRC})
add_dependencies(libdsdc libpub_headers libdsdc_headers)
