#-----------------------------------------------------------------------------
# VTK GDCM Reader subdir
# Rebuild gdcm whenever a file starting with vtk* is modified
INCLUDE_REGULAR_EXPRESSION("^vtk.*$")


     if(COMMAND cmake_policy)
       cmake_policy(SET CMP0003 NEW)
     endif(COMMAND cmake_policy)



#-----------------------------------------------------------------------------
# Include the VTK library

# EED 5/Mars/2010
SET(VTK_BUILD_SETTINGS_FILE OFF)

INCLUDE(${VTK_USE_FILE})

# Need gdcm header files
INCLUDE_DIRECTORIES(
  ${GDCM_SOURCE_DIR}/src/
  ${GDCM_BINARY_DIR}/
)

# Depending on VTK version build more stuff:
IF("${VTK_MAJOR_VERSION}" GREATER 4)
  SET(VTKGDCM_LIB_SRCS
    vtkGdcmReader.cxx
    vtkGdcmWriter.cxx
    vtkImageColorViewer.cxx
    vtkImageMapToWindowLevelColors2.cxx
  )
ELSE("${VTK_MAJOR_VERSION}" GREATER 4)
  SET(VTKGDCM_LIB_SRCS
    vtkGdcmReader.cxx
    vtkGdcmWriter.cxx
  )
ENDIF("${VTK_MAJOR_VERSION}" GREATER 4)

#-----------------------------------------------------------------------------
# Create vtkgdcm library
ADD_LIBRARY(vtkgdcm ${VTKGDCM_LIB_SRCS})
TARGET_LINK_LIBRARIES(vtkgdcm
  gdcm
  vtkCommon
  vtkIO
  vtkFiltering
)
IF("${VTK_MAJOR_VERSION}" GREATER 4)
  TARGET_LINK_LIBRARIES(vtkgdcm
    vtkRendering
  )
ENDIF("${VTK_MAJOR_VERSION}" GREATER 4)

#-----------------------------------------------------------------------------
SET(vtkgdcmdemo_SOURCES
  vtkGdcmDemo.cxx
)
ADD_EXECUTABLE(vtkgdcmdemo ${vtkgdcmdemo_SOURCES})
TARGET_LINK_LIBRARIES(vtkgdcmdemo
  vtkgdcm
  vtkGraphics
  vtkRendering
)

#-----------------------------------------------------------------------------
SET(Bmp2Dcm_SOURCES
  Bmp2Dcm.cxx
)
ADD_EXECUTABLE(Bmp2Dcm ${Bmp2Dcm_SOURCES})
TARGET_LINK_LIBRARIES(Bmp2Dcm
  vtkgdcm
  vtkImaging

)

#-----------------------------------------------------------------------------
SET(vtkgdcmViewer_SOURCES
  vtkgdcmViewer.cxx
)

#SET(vtkJPEGViewer_SOURCES
#  vtkJPEGViewer.cxx
#)

#SET(vtkJPEGViewer2_SOURCES
#  vtkJPEGViewer2.cxx
#)

# let's do some APPLE/WIN32 magic here
#IF(WIN32)
#  SET(GUI_EXECUTABLE WIN32)
#ENDIF(WIN32)
IF(APPLE)
  IF(VTK_USE_CARBON)
    SET(GUI_EXECUTABLE MACOSX_BUNDLE)
  ENDIF(VTK_USE_CARBON)
  IF(VTK_USE_COCOA)
    SET(GUI_EXECUTABLE MACOSX_BUNDLE)
  ENDIF(VTK_USE_COCOA)
  # when using X11 on Apple you don't need this...
ENDIF(APPLE)

ADD_EXECUTABLE(vtkgdcmViewer ${GUI_EXECUTABLE} ${vtkgdcmViewer_SOURCES})
TARGET_LINK_LIBRARIES(vtkgdcmViewer
  vtkgdcm
  vtkRendering
)

#ADD_EXECUTABLE(vtkJPEGViewer ${GUI_EXECUTABLE} ${vtkJPEGViewer_SOURCES})
#TARGET_LINK_LIBRARIES( vtkJPEGViewer
#  vtkRendering
#)

#ADD_EXECUTABLE(vtkJPEGViewer2 ${GUI_EXECUTABLE} ${vtkJPEGViewer2_SOURCES})
#TARGET_LINK_LIBRARIES( vtkJPEGViewer2
#  vtkgdcm
#  vtkRendering
#)

ADD_EXECUTABLE(vtkgdcmViewer2 ${GUI_EXECUTABLE} vtkgdcmViewer2.cxx)
TARGET_LINK_LIBRARIES(vtkgdcmViewer2
  vtkgdcm
  vtkRendering
)

ADD_EXECUTABLE(vtkgdcmSerieViewer ${GUI_EXECUTABLE} vtkgdcmSerieViewer.cxx)
TARGET_LINK_LIBRARIES(vtkgdcmSerieViewer
  vtkgdcm
  vtkRendering
)

ADD_EXECUTABLE(vtkgdcmSerieViewer2 ${GUI_EXECUTABLE} vtkgdcmSerieViewer2.cxx)
TARGET_LINK_LIBRARIES(vtkgdcmSerieViewer2
  vtkgdcm
  vtkRendering
)


#-----------------------------------------------------------------------------
SET(vtkWriteDicom_SOURCES
  vtkWriteDicom.cxx
)

SET(vtkWriteDicomExtended_SOURCES
  vtkWriteDicomExtended.cxx  
)

ADD_EXECUTABLE(vtkWriteDicom ${vtkWriteDicom_SOURCES})
TARGET_LINK_LIBRARIES(vtkWriteDicom
  vtkgdcm
  vtkIO
  vtkRendering
)

ADD_EXECUTABLE(vtkWriteDicomExtended ${vtkWriteDicomExtended_SOURCES})
TARGET_LINK_LIBRARIES(vtkWriteDicomExtended
  vtkgdcm
  vtkIO
  vtkRendering
)
#-----------------------------------------------------------------------------
# Install vtk header
IF(NOT GDCM_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${GDCM_INSTALL_INCLUDE_DIR} FILES vtkGdcmReader.h)
  INSTALL_FILES(${GDCM_INSTALL_INCLUDE_DIR} FILES vtkGdcmWriter.h)
ENDIF(NOT GDCM_INSTALL_NO_DEVELOPMENT)

IF(NOT GDCM_INSTALL_NO_LIBRARIES)
  INSTALL_TARGETS(${GDCM_INSTALL_LIB_DIR} vtkgdcm)
  INSTALL_TARGETS(${GDCM_INSTALL_BIN_DIR} vtkgdcmViewer)
  INSTALL_TARGETS(${GDCM_INSTALL_BIN_DIR} vtkgdcmViewer)
  
 # INSTALL_TARGETS(${GDCM_INSTALL_BIN_DIR} vtkJPEGViewer)
 # INSTALL_TARGETS(${GDCM_INSTALL_BIN_DIR} vtkJPEGViewer2)
      
  INSTALL_TARGETS(${GDCM_INSTALL_BIN_DIR} vtkgdcmSerieViewer)
ENDIF(NOT GDCM_INSTALL_NO_LIBRARIES)

