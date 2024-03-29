CMAKE_MINIMUM_REQUIRED( VERSION 2.6.2 )

#Nom du projet
PROJECT(LidarFormat)

SET(LidarFormat_version "000102")
# Useful for Eclipse code STL completion
if(UNIX)
	exec_program( gcc ARGS -dumpversion OUTPUT_VARIABLE GCC_VERSION )
	string( SUBSTRING ${GCC_VERSION} 0 3 GOOD_GCC_VERSION )
    include_directories( /usr/include/c++/${GOOD_GCC_VERSION} )
endif()

####
#### On recherche tous les fichiers ".h", ".hpp", ".hxx" et "*.inl"
####
FILE( GLOB ALL_LIDAR_FORMAT_HEADER_FILES src/LidarFormat/*.h )
FILE( GLOB ALL_FILE_FORMATS_HEADER_FILES src/LidarFormat/file_formats/*.h )
FILE( GLOB ALL_GEOMETRY_HEADER_FILES src/LidarFormat/geometry/*.h )
FILE( GLOB ALL_TOOLS_HEADER_FILES src/LidarFormat/tools/*.h src/LidarFormat/tools/*.hpp)
FILE( GLOB ALL_MODELS_HEADER_FILES src/LidarFormat/models/*.h src/LidarFormat/models/*.hpp)
FILE( GLOB ALL_EXTERN_HEADER_FILES src/LidarFormat/extern/matis/*.h)

#Pour utiliser le find xerces et xsd:
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
FIND_PACKAGE(XercesC)
IF( XERCESC_FOUND )
	INCLUDE_DIRECTORIES( ${XERCESC_INCLUDE_DIR} )
	SET(LidarFormat_LIBRAIRIES ${LidarFormat_LIBRAIRIES} ${XERCESC_LIBRARIES})
ELSE()
	MESSAGE( FATAL_ERROR "XercesC not found ! Please set XercesC path ..." )
ENDIF()

FIND_PACKAGE(Xsd)
IF( XSD_FOUND )
	INCLUDE_DIRECTORIES( ${XSD_INCLUDE_DIR} )
	#MESSAGE("XSD Include Path : " ${XSD_INCLUDE_DIR} )
ELSE()
	MESSAGE( FATAL_ERROR "XSD not found ! Please set XSD path ..." )
ENDIF()

#Génération du code de parse des fichiers xml (gestion du format lidar)
ADD_CUSTOM_COMMAND(
    OUTPUT  ${CMAKE_SOURCE_DIR}/src/LidarFormat/models/format_me.cxx 
			${CMAKE_SOURCE_DIR}/src/LidarFormat/models/format_me.hxx 
			${CMAKE_SOURCE_DIR}/src/LidarFormat/models/format_me-fwd.hxx
    COMMAND ${XSD_EXECUTABLE} cxx-tree 	--function-naming lcc 
							--type-naming ucc 
							--generate-polymorphic 
							--generate-comparison 
							--generate-doxygen 
							--generate-serialization 
							--generate-forward 
							--output-dir ${CMAKE_SOURCE_DIR}/src/LidarFormat/models ${CMAKE_SOURCE_DIR}/src/LidarFormat/models/xsd/format_me.xsd
	DEPENDS ${CMAKE_SOURCE_DIR}/src/LidarFormat/models/xsd/format_me.xsd
)

SET( ALL_MODELS_HEADER_FILES 
		${ALL_MODELS_HEADER_FILES} 
		${CMAKE_SOURCE_DIR}/src/LidarFormat/models/format_me.cxx 
		${CMAKE_SOURCE_DIR}/src/LidarFormat/models/format_me.hxx 
		${CMAKE_SOURCE_DIR}/src/LidarFormat/models/format_me-fwd.hxx
)



#### Definition des dossiers contenant les sources
SET(SRC_DIR src/)
INCLUDE_DIRECTORIES(${SRC_DIR})

AUX_SOURCE_DIRECTORY(${SRC_DIR}/LidarFormat SRC_LIDAR_FORMAT)
AUX_SOURCE_DIRECTORY(${SRC_DIR}/LidarFormat/geometry  SRC_GEOMETRY)
AUX_SOURCE_DIRECTORY(${SRC_DIR}/LidarFormat/tools  SRC_TOOLS)
AUX_SOURCE_DIRECTORY(${SRC_DIR}/LidarFormat/file_formats  SRC_FILE_FORMATS)
AUX_SOURCE_DIRECTORY(${SRC_DIR}/LidarFormat/file_formats/standard  SRC_FILE_FORMATS_STANDARD)
AUX_SOURCE_DIRECTORY(${SRC_DIR}/LidarFormat/extern/matis  SRC_EXTERN_MATIS)

SET(SRC_MODELS_DIR ${SRC_DIR}/LidarFormat/models)
SET(SRC_MODELS  ${SRC_MODELS_DIR}/format_me.cxx)

####
#### Python bindings
####
OPTION( BUILD_PYTHON_BINDINGS "Build Python bindings" OFF )
if(BUILD_PYTHON_BINDINGS)
    INCLUDE( cmake/LidarFormat_python_bindings.cmake )
endif(BUILD_PYTHON_BINDINGS)


# Une variable regroupant toutes les sources ...
SET( ALL_SOURCES 
        ${SRC_LIDAR_FORMAT} 
        ${SRC_MODELS} 
        ${SRC_GEOMETRY} 
        ${SRC_TOOLS} 
        ${SRC_FILE_FORMATS}
        ${SRC_FILE_FORMATS_STANDARD}
        ${SRC_EXTERN_MATIS} 
        
        ${SRC_PYTHON}
   )

SET( ALL_HEADERS ${ALL_LIDAR_FORMAT_HEADER_FILES} ${ALL_MODELS_HEADER_FILES} ${ALL_GEOMETRY_HEADER_FILES})

INCLUDE( cmake/LidarFormat_plugin_io.cmake )

####
#### Recherche des libs
####	


# Find BOOST
# CMake does not include boost version 1.39
SET(Boost_ADDITIONAL_VERSIONS "1.39.0" "1.39" "1.38.0" "1.38" "1.37.0" "1.37" "1.40" "1.41" "1.42" "1.43" "1.44")
FIND_PACKAGE( Boost 1.36 COMPONENTS filesystem system unit_test_framework REQUIRED)
if( Boost_FOUND )
	INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )
	LINK_DIRECTORIES( ${Boost_LIBRARY_DIRS} )
	# Autolink under Windows platforms
	if( NOT WIN32 )
		SET(LidarFormat_LIBRAIRIES ${LidarFormat_LIBRAIRIES} ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY})
	endif()
else()
	message( FATAL_ERROR "Boost not found ! Please set Boost path ..." )
endif()


####
#### Construction de la librarie
####
# Options pour construire une librarie (et la choisir en .a ou .so)
SET( LIBRARY_TYPE STATIC )
# Verification du type de bibliotheque a construire
OPTION( BUILD_LIBRARY_SHARED "Build LidarFormat as a SHARED library." ON )
IF( BUILD_LIBRARY_SHARED )
	SET( LIBRARY_TYPE SHARED )
ELSE()
	SET( LIBRARY_TYPE STATIC )
ENDIF()

if(UNIX)
	ADD_DEFINITIONS(-Wall)
else()
	ADD_DEFINITIONS( -W3 )
	# Visual warnings on checked iterators
	ADD_DEFINITIONS( -D_SCL_SECURE_NO_WARNINGS )
	ADD_DEFINITIONS( -D_CRT_SECURE_NO_WARNINGS )
endif()

ADD_LIBRARY( LidarFormat  ${LIBRARY_TYPE} ${ALL_SOURCES} ${ALL_HEADERS}  )
TARGET_LINK_LIBRARIES( LidarFormat ${LidarFormat_LIBRAIRIES} )


####
#### Choice for install mode.
####     - None : no install target
####     - Install : create a install target
####     - Package : create a packake 
SET(INSTALL_MODE "Install" CACHE STRING "Choose install mode between : None, Install, Package")
IF(${INSTALL_MODE} STREQUAL "Install")
	INCLUDE(cmake/LidarFormat_install.cmake )
	MESSAGE( STATUS " Install file include " )
ENDIF(${INSTALL_MODE} STREQUAL "Install")
IF(${INSTALL_MODE} STREQUAL "Package")
	INCLUDE(cmake/LidarFormat_package.cmake )
	MESSAGE( STATUS " Install file package" )
ENDIF(${INSTALL_MODE} STREQUAL "Package")

####
#### Tests
####
OPTION( BUILD_TESTS "Build tests" ON )
if(BUILD_TESTS)
    INCLUDE( cmake/LidarFormat_tests.cmake )
endif(BUILD_TESTS)

####
#### Documentation ( create a doc traget for generating doxygen pages)
####
#OPTION( BUILD_DOC "Build doc " OFF )
#if(BUILD_DOC)
#    INCLUDE( cmake/LidarFormat_doc.cmake )
#endif(BUILD_DOC)



