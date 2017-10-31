# ------------------------------------------------------------------------------------------------------------------------------
# CMAKE SPECIFIC INFO. Versions/paths/etc go here.
# ------------------------------------------------------------------------------------------------------------------------------

# Version 3 is required to find some of the modules
cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR)

# ------------------------------------------------------------------------------------------------------------------------------
# PROJECT INFO. The project and target name go here.
# ------------------------------------------------------------------------------------------------------------------------------

PROJECT(chart)
SET(PROJECT_FOLDER QtWidgets)

IF(WIN32)
	SET(GUI_TYPE WIN32)
ENDIF(WIN32)

# ------------------------------------------------------------------------------------------------------------------------------
# IMPORTANT VARIABLES. These don't actually need to be initialized, but I put them here so you know that you have to fill them!
# ------------------------------------------------------------------------------------------------------------------------------

SET(SOURCE_CODE)	# all .cpp and .h files containing templates get added here
SET(HEADER_CODE)	# all non-template header files go here
SET(INCLUDE_DIRS)	# all library (local and 3rd party) header directories have to be referenced here
SET(LIBRARIES)		# all libraries have to be added here

# ------------------------------------------------------------------------------------------------------------------------------
# SOURCE FILES: Add source code files to this variable (.cpp). For template classes, add the .h
# ------------------------------------------------------------------------------------------------------------------------------

SET(SOURCE_CODE 
chart.cpp
chartView.cpp
callout.cpp
lineSeries.cpp
)

# ------------------------------------------------------------------------------------------------------------------------------
# HEADER FILES: add project header files to this variable.
# ------------------------------------------------------------------------------------------------------------------------------

SET(HEADER_CODE
chart.h
chartView.h
callout.h
lineSeries.h
)

# For windows, add headers to the source so they show up in the vcproj
if(WIN32)
	set(SOURCE_CODE ${SOURCE_CODE} ${HEADER_CODE})
endif(WIN32)

# ------------------------------------------------------------------------------------------------------------------------------
# COMPILER FLAGS. Set compiler flags here.
# ------------------------------------------------------------------------------------------------------------------------------



# ------------------------------------------------------------------------------------------------------------------------------
# RESOURCE FILES: add project resource files to this variable.
# ------------------------------------------------------------------------------------------------------------------------------

SET(QT_RESOURCE_CODE 

)

#Add in the QT Resources
#QT5_ADD_RESOURCES(RESOURCES ${RESOURCES} ${QT_RESOURCE_CODE})

# Add the windows icon resource file on windows builds
IF(WIN32)
	SET(RESOURCE_CODE ${RESOURCE_CODE} )
ENDIF(WIN32)

# ------------------------------------------------------------------------------------------------------------------------------
# SOURCE GROUPS. Set up VS2010 source code filters here.
# ------------------------------------------------------------------------------------------------------------------------------

IF(WIN32)
	SOURCE_GROUP("Resources" FILES ${RESOURCE_CODE} ${QT_RESOURCE_CODE})
	SOURCE_GROUP("Generated" FILES ${PROTO_CODE} ${RESOURCES})
	SOURCE_GROUP("Generated" REGULAR_EXPRESSION ".*_automoc\\.cpp")
ENDIF(WIN32)

# ------------------------------------------------------------------------------------------------------------------------------
# LIBRARIES. Add libraries that are part of the build here.
# ------------------------------------------------------------------------------------------------------------------------------

LIST(APPEND LIBRARIES
	Qt5::Core
	Qt5::Widgets
	Qt5::Charts
)

# ------------------------------------------------------------------------------------------------------------------------------
# BUILD (GENERATE LIBRARY). This code is generic and shouldn't need to be changed.
# ------------------------------------------------------------------------------------------------------------------------------

# Build target
ADD_TAGGED_LIBRARY(${PROJECT_NAME} STATIC ${SOURCE_CODE} ${RESOURCES} ${RESOURCE_CODE})

# Additional include directories (library headers, top level .h files, etc)
target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIRS} ${INCLUDE_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR})

# Set VS2013 folder
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER ${PROJECT_FOLDER} INSTALL_RPATH_USE_LINK_PATH TRUE)

# we have to specify x64 on windows (due to inclusion of resource file)
IF(WIN32)
	set_target_properties(${PROJECT_NAME} PROPERTIES STATIC_LIBRARY_FLAGS "/machine:x64")
ENDIF(WIN32)

# Link 3rd party libs (needs to be done after ADD_TAGGED_EXECUTABLE)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIBRARIES})