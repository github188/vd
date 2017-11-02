set(TOOLCHAIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../ti_tools/linux_devkit/bin")

set(CMAKE_SYSTEM_NAME Linux)
#set(CMAKE_C_COMPILER "${TOOLCHAIN_DIR}/arm-arago-linux-gnueabi-gcc")
#set(CMAKE_CXX_COMPILER "${TOOLCHAIN_DIR}/arm-arago-linux-gnueabi-g++")
set(CMAKE_FIND_ROOT_PATH "...")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

project(vd)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_USE_RELATIVE_PATHS 1)

set(FLAGS "-Wall -funwind-tables -fstack-protector-all")
set(CMAKE_C_FLAGS "${FLAGS} -std=gnu99")
set(CMAKE_CXX_FLAGS "${FLAGS} -std=gnu++0x")
set(CMAKE_MODULE_LINKER_FLAGS "-rdynamic")
set(CMAKE_SHARED_LINKER_FLAGS "-rdynamic")
set(CMAKE_EXE_LINKER_FLAGS "-rdynamic")

add_definitions(-D__STDC_FORMAT_MACROS)
add_definitions(-DPARK_ZEHIN_THIN)

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
	message(STATUS "Debug mode")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g")
else()
	message(STATUS "Release mode")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -s -Os")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -s -Os")
	add_definitions(-DNDEBUG)
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)
set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/../../target/filesys")

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

include_directories(
		${PROJECT_SOURCE_DIR}/
		${PROJECT_SOURCE_DIR}/../../
		${PROJECT_SOURCE_DIR}/../../ipnc_mcfw/
		${PROJECT_SOURCE_DIR}/../../ipnc_mcfw/mcfw/interfaces
		${PROJECT_SOURCE_DIR}/../../ipnc_mcfw/mcfw/interfaces/link_api/
		${PROJECT_SOURCE_DIR}/../../ipnc_mcfw/mcfw/src_bios6/links_c6xdsp/videoAnalysis/
		${PROJECT_SOURCE_DIR}/../interface/inc
		${PROJECT_SOURCE_DIR}/../network/zeroconfig/avahi/avahi-0.6.31/avahi-compat-howl/include
		${PROJECT_SOURCE_DIR}/../../../ti_tools/linuxutils_3_23_00_01/packages/ti/sdo/linuxutils/cmem/include)

include_directories(
		${PROJECT_SOURCE_DIR}/lib/lib_ActiveMQ-CPP/include/activemq-cpp-3.5.0/
		${PROJECT_SOURCE_DIR}/lib/_install_apr/include/apr-1
		${PROJECT_SOURCE_DIR}/lib/_install_apr_util/include/apr-1
		${PROJECT_SOURCE_DIR}/lib/_install_apr_iconv/include/apr-1
		${PROJECT_SOURCE_DIR}/lib/_install_cppunit/include/cppunit
		${PROJECT_SOURCE_DIR}/lib/_install_expat/include
		${PROJECT_SOURCE_DIR}/lib/_install_ssh/include/openssl
		${PROJECT_SOURCE_DIR}/lib/libxml2/include/libxml2
		${PROJECT_SOURCE_DIR}/lib/Json/include/
		${PROJECT_SOURCE_DIR}/lib/sqlite/include/
		${PROJECT_SOURCE_DIR}/../interface/src/iniparser/src
		${PROJECT_SOURCE_DIR}/../interface/src/json-c-0.9
		${PROJECT_SOURCE_DIR}/../interface/bitcom/include
		${PROJECT_SOURCE_DIR}/lib/libmad/include/)

include_directories(
		${PROJECT_SOURCE_DIR}/
		${PROJECT_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/toggle
		${PROJECT_SOURCE_DIR}/toggle/dsp
		${PROJECT_SOURCE_DIR}/toggle/eplate
		${PROJECT_SOURCE_DIR}/vmq
		${PROJECT_SOURCE_DIR}/vmq/inc
		${PROJECT_SOURCE_DIR}/test
		${PROJECT_SOURCE_DIR}/mq
		${PROJECT_SOURCE_DIR}/ftp
		${PROJECT_SOURCE_DIR}/storage
		${PROJECT_SOURCE_DIR}/xml
		${PROJECT_SOURCE_DIR}/sqlite3/park/zehin/inc
		${PROJECT_SOURCE_DIR}/sqlite3/park/system/inc
		${PROJECT_SOURCE_DIR}/sqlite3/park/bitcom/inc
		${PROJECT_SOURCE_DIR}/sqlite3/park/songli/inc
		${PROJECT_SOURCE_DIR}/sqlite3/park/http/inc
		${PROJECT_SOURCE_DIR}/tpi/inc
		${PROJECT_SOURCE_DIR}/tpi/park/inc
		${PROJECT_SOURCE_DIR}/tpi/park/http/inc
		${PROJECT_SOURCE_DIR}/tpi/park/songli/inc
		${PROJECT_SOURCE_DIR}/tpi/park/bitcom/inc
		${PROJECT_SOURCE_DIR}/tpi/vp/uniview/inc
		${PROJECT_SOURCE_DIR}/tpi/vp/baokang/inc
		${PROJECT_SOURCE_DIR}/tpi/vp/netposa/inc
		${PROJECT_SOURCE_DIR}/oss
		${PROJECT_SOURCE_DIR}/sqlite3/alleyway/bitcom/inc
		${PROJECT_SOURCE_DIR}/sqlite3/alleyway/common
		${PROJECT_SOURCE_DIR}/oss/include/oss_c_sdk
		${PROJECT_SOURCE_DIR}/oss/include/mxml
		${PROJECT_SOURCE_DIR}/ALPU
		${PROJECT_SOURCE_DIR}/h264
		${PROJECT_SOURCE_DIR}/sysserver
		${PROJECT_SOURCE_DIR}/vdsystem/inc
		${PROJECT_SOURCE_DIR}/park_status/inc)

link_directories(
		${PROJECT_SOURCE_DIR}/lib/libxml2/lib
		${PROJECT_SOURCE_DIR}/lib/lib_ActiveMQ-CPP/lib/
		${PROJECT_SOURCE_DIR}/lib/_install_cppunit/lib
		${PROJECT_SOURCE_DIR}/lib/_install_apr_iconv/lib
		${PROJECT_SOURCE_DIR}/lib/_install_apr_util/lib
		${PROJECT_SOURCE_DIR}/lib/_install_apr/lib
		${PROJECT_SOURCE_DIR}/lib/_install_iconv/lib
		${PROJECT_SOURCE_DIR}/lib/_install_ssh/lib
		${PROJECT_SOURCE_DIR}/lib/_install_expat/lib
		${PROJECT_SOURCE_DIR}/lib/ALPU
		${PROJECT_SOURCE_DIR}/lib/Json/lib
		${PROJECT_SOURCE_DIR}/lib/sqlite/lib
		${PROJECT_SOURCE_DIR}/lib/iniparser
		${PROJECT_SOURCE_DIR}/oss/lib
		${PROJECT_SOURCE_DIR}/../interface/bitcom/build)

link_libraries(
		${PROJECT_SOURCE_DIR}/../interface/lib/sem_util.a
		${PROJECT_SOURCE_DIR}/../interface/lib/i2c_sem_util.a
		${PROJECT_SOURCE_DIR}/../interface/lib/Appro_interface.a
		${PROJECT_SOURCE_DIR}/../interface/lib/file_msg_drv.a
		${PROJECT_SOURCE_DIR}/../interface/lib/log_interface.a
		${PROJECT_SOURCE_DIR}/../interface/lib/serial_ctrl.a
		${PROJECT_SOURCE_DIR}/../interface/lib/sysctrl.a
		${PROJECT_SOURCE_DIR}/../interface/lib/writePidFile.a
		${PROJECT_SOURCE_DIR}/../interface/lib/osa_i2c.a
		${PROJECT_SOURCE_DIR}/../interface/src/iniparser/libiniparser.a
		${PROJECT_SOURCE_DIR}/../interface/lib/libjson.a
		${PROJECT_SOURCE_DIR}/../interface/lib/light_ctl_api.a
		${PROJECT_SOURCE_DIR}/../interface/lib/timer.a
		${PROJECT_SOURCE_DIR}/../../../ti_tools/linuxutils_3_23_00_01/packages/ti/sdo/linuxutils/cmem/lib/cmem.a470MV)

link_libraries(
		curl
        oss_c_sdk_static
        mxml
		ALPU
		Json
		pthread
		sqlite3
		xml2
		apr-1
		apriconv-1
		aprutil-1
		cppunit
		expat
		activemq-cpp
		iconv
		m
		uuid
		asound
		bitcom)

link_libraries(
		toggle
		dsp
		eplate
		vmq
		mq
		storage
		oss
		ftp
		tpi
		sqlite
		h264
		sysserver
		vd_system
		xml
		tpivp
        park_status
        )

link_libraries(
		${PROJECT_SOURCE_DIR}/lib/libmad/lib/libmad.so)

add_subdirectory(${PROJECT_SOURCE_DIR}/ALPU)
add_subdirectory(${PROJECT_SOURCE_DIR}/sys)
add_subdirectory(${PROJECT_SOURCE_DIR}/toggle)
add_subdirectory(${PROJECT_SOURCE_DIR}/h264)
add_subdirectory(${PROJECT_SOURCE_DIR}/mq)
add_subdirectory(${PROJECT_SOURCE_DIR}/oss)
add_subdirectory(${PROJECT_SOURCE_DIR}/storage)
add_subdirectory(${PROJECT_SOURCE_DIR}/sysserver)
add_subdirectory(${PROJECT_SOURCE_DIR}/vmq)
add_subdirectory(${PROJECT_SOURCE_DIR}/xml)
add_subdirectory(${PROJECT_SOURCE_DIR}/ftp)
add_subdirectory(${PROJECT_SOURCE_DIR}/sqlite3)
add_subdirectory(${PROJECT_SOURCE_DIR}/vdsystem)
add_subdirectory(${PROJECT_SOURCE_DIR}/dev)
add_subdirectory(${PROJECT_SOURCE_DIR}/ve)
add_subdirectory(${PROJECT_SOURCE_DIR}/tpi)
add_subdirectory(${PROJECT_SOURCE_DIR}/park_status)

aux_source_directory(${PROJECT_SOURCE_DIR} vd_src)

add_executable(${PROJECT_NAME} ${vd_src})
target_link_libraries(${PROJECT_NAME}
		ve
		dev)

install(TARGETS vd RUNTIME DESTINATION opt/ipnc)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/lib/libmad/lib/libmad.so DESTINATION usr/lib)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/lib/libmad/lib/libmad.so.0 DESTINATION usr/lib)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/lib/libmad/lib/libmad.so.0.2.1 DESTINATION usr/lib)
