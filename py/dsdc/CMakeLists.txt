# Copyright OkCupid 2016

include(${CMAKE_SOURCE_DIR}/CMakeHelpers.cmake)

set(LIBPROT_C_SRC "")
set(LIBPROT_H_SRC "")
set(LIBPROT_XML_C_SRC "")
set(LIBPROT_XML_H_SRC "")

#set(XML_PROT_FILES "")
set(XML_PROT_FILES prot.x)

set(SRC "")
set(HEADERS "")

OkwsPreprocessXmlProtFiles(LIBPROT_XML_C_SRC LIBPROT_XML_H_SRC "${XML_PROT_FILES}")
add_custom_target(libpy_headers DEPENDS ${LIBPROT_H_SRC} ${LIBPROT_XML_H_SRC} ${HEADERS})
