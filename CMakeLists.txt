project(EMSegment)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)
MARK_AS_ADVANCED(CMAKE_BACKWARDS_COMPATIBILITY)
 
# --------------------------------------------------------------------------
# Find Slicer3 or Slicer4

if(NOT EM_Slicer4_FOUND)

  # we are not in the Slicer4 Source Dir, let's check if we are in Slicer3 Source Dir
  if(NOT Slicer3_SOURCE_DIR)
    # we are not even in the Slicer3 Source Dir, we have to find Slicer3
    find_package(Slicer3 REQUIRED)
    include(${Slicer3_USE_FILE})
    slicer3_set_default_install_prefix_for_external_projects()
  endif(NOT Slicer3_SOURCE_DIR)
  
  set (EM_Slicer3_FOUND ${Slicer3_SOURCE_DIR})

else(NOT EM_Slicer4_FOUND)

  IF(NOT Slicer_SOURCE_DIR)
    FIND_PACKAGE(Slicer REQUIRED)
    INCLUDE(${Slicer_USE_FILE})
  ENDIF(NOT Slicer_SOURCE_DIR)

endif(NOT EM_Slicer4_FOUND)

set(EMSegment_BASE_SRCS 
  # MRML
  ${CMAKE_CURRENT_SOURCE_DIR}/MRML/vtkMRMLEMSNode.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/MRML/vtkMRMLEMSSegmenterNode.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/MRML/vtkMRMLEMSTemplateNode.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/MRML/vtkMRMLEMSTreeNode.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/MRML/vtkMRMLEMSTreeParametersNode.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/MRML/vtkMRMLEMSTreeParametersParentNode.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/MRML/vtkMRMLEMSTreeParametersLeafNode.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/MRML/vtkMRMLEMSGlobalParametersNode.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/MRML/vtkMRMLEMSCollectionNode.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/MRML/vtkMRMLEMSVolumeCollectionNode.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/MRML/vtkMRMLEMSAtlasNode.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/MRML/vtkMRMLEMSTargetNode.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/MRML/vtkMRMLEMSWorkingDataNode.cxx
  # ${CMAKE_CURRENT_SOURCE_DIR}/MRML/vtkMRMLEMSIntensityNormalizationParametersNode.cxx
  # ${CMAKE_CURRENT_SOURCE_DIR}/MRML/vtkMRMLEMSClassInteractionMatrixNode.cxx

  # Algorithm 
  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/EMLocalInterface.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/EMLocalRegistrationCostFunction.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/EMLocalShapeCostFunction.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/vtkDataDef.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/vtkFileOps.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/vtkImageEMGeneral.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/vtkImageEMGenericClass.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/vtkImageEMLocalClass.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/vtkImageEMLocalGenericClass.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/vtkImageEMLocalSegmenter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/vtkImageEMLocalSuperClass.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/vtkTimeDef.cxx

  # ImageFilters
  ${CMAKE_CURRENT_SOURCE_DIR}/ImageFilters/vtkSimonParameterReaderWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/ImageFilters/vtkImageMeanIntensityNormalization.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/ImageFilters/vtkImageLabelPropagation.cxx 
  ${CMAKE_CURRENT_SOURCE_DIR}/ImageFilters/vtkImageIslandFilter.cxx 
  ${CMAKE_CURRENT_SOURCE_DIR}/ImageFilters/vtkImageSumOverVoxels.cxx

  # Registration
  ${CMAKE_CURRENT_SOURCE_DIR}/Registration/vtkRigidRegistrator.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Registration/vtkBSplineRegistrator.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Registration/vtkITKTransformAdapter.cxx

  # not used in current implementation---used in slicer2 modules
  #  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/vtkImageEMAtlasClass.cxx
  #  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/vtkImageEMAtlasSegmenter.cxx
  #  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/vtkImageEMAtlasSuperClass.cxx
  #  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/vtkImageEMMarkov.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Tasks/vtkCTHandBoneClass.cxx

  # module
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkEMSegmentMRMLManager.cxx 
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkEMSegmentLogic.cxx 
)

include_directories(
  ${EMSegment_SOURCE_DIR}
  ${EMSegment_BINARY_DIR}
  ${EMSegment_SOURCE_DIR}/MRML
  ${EMSegment_SOURCE_DIR}/Algorithm
  ${EMSegment_SOURCE_DIR}/ImageFilters
  ${EMSegment_SOURCE_DIR}/Registration
  ${EMSegment_SOURCE_DIR}/Tasks
  ${Slicer3_Libs_INCLUDE_DIRS}
  ${Slicer3_Base_INCLUDE_DIRS}
  ${vtkSlicerVolumesModuleLogic_SOURCE_DIR}
  ${vtkSlicerVolumesModuleLogic_BINARY_DIR}
  ${Volumes_SOURCE_DIR}
  ${Volumes_BINARY_DIR}
  )

if(EM_Slicer3_FOUND)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkEMSegmentConfigure.h.in 
  ${CMAKE_CURRENT_BINARY_DIR}/vtkEMSegmentConfigure.h
  )

ADD_CUSTOM_TARGET(copytasks ALL
  ${CMAKE_COMMAND}
  -DSRC=${CMAKE_CURRENT_SOURCE_DIR}/Tasks
  -DDST=${CMAKE_BINARY_DIR}/${Slicer3_INSTALL_MODULES_SHARE_DIR}/${PROJECT_NAME}/
  -P ${CMAKE_CURRENT_SOURCE_DIR}/copytasks.cmake
)

install(
  DIRECTORY Tasks
  DESTINATION ${Slicer3_INSTALL_MODULES_SHARE_DIR}/${PROJECT_NAME}/
  FILES_MATCHING PATTERN "*"
)


file(GLOB TCLFILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "Tcl/*.tcl" )

foreach(file ${TCLFILES})
 configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${file}
    ${CMAKE_BINARY_DIR}/${Slicer3_INSTALL_MODULES_SHARE_DIR}/${PROJECT_NAME}/${file}
    COPYONLY)
endforeach(file)
# MESSAGE(STATUS  "${TCLFILES} ${CMAKE_CURRENT_SOURCE_DIR}")

install(
  FILES ${TCLFILES}
  DESTINATION ${Slicer3_INSTALL_MODULES_SHARE_DIR}/${PROJECT_NAME}/Tcl
)

### this code will be replaced by the cmake command 'file(COPY ....)' below 
file(GLOB TASKFILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "Tasks/*.tcl" "Tasks/*.mrml")
foreach(file ${TASKFILES})
 configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${file}
    ${CMAKE_BINARY_DIR}/${Slicer3_INSTALL_MODULES_SHARE_DIR}/${PROJECT_NAME}/${file}
    COPYONLY)
endforeach(file)
#
#install(
#  FILES ${TASKFILES}
#  DESTINATION ${Slicer3_INSTALL_MODULES_SHARE_DIR}/${PROJECT_NAME}/Tasks
#)
###

install(FILES 
  ${headers} 
  "${CMAKE_CURRENT_BINARY_DIR}/vtkEMSegmentConfigure.h"
  DESTINATION ${Slicer3_INSTALL_MODULES_INCLUDE_DIR}/${PROJECT_NAME} COMPONENT Development
  )



# --------------------------------------------------------------------------
# Testing

  if(BUILD_TESTING)
      ADD_SUBDIRECTORY(Testing)
  endif(BUILD_TESTING)



# --------------------------------------------------------------------------
# Install the testing data

install(DIRECTORY
    Testing/TestData
    DESTINATION ${Slicer3_INSTALL_MODULES_SHARE_DIR}/${PROJECT_NAME}/Testing COMPONENT Development
    PATTERN ".svn" EXCLUDE
    )


# --------------------------------------------------------------------------
# Command line application
  ADD_SUBDIRECTORY(CommandLineApplication)

endif(EM_Slicer3_FOUND)


#
# Slicer3/Slicer4
#
if(EM_Slicer3_FOUND)
  ADD_SUBDIRECTORY(KWWidgets)
endif(EM_Slicer3_FOUND)

if(EM_Slicer4_FOUND)
  ADD_SUBDIRECTORY(Qt)
endif(EM_Slicer4_FOUND)



