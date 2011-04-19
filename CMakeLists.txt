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
    slicer3_set_default_install_prefix_for_external_projects()
  ENDIF(NOT Slicer_SOURCE_DIR)

endif(NOT EM_Slicer4_FOUND)

# --------------------------------------------------------------------------
# Include dirs

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkEMSegmentConfigure.h.in 
  ${CMAKE_CURRENT_BINARY_DIR}/vtkEMSegmentConfigure.h
  )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/MRML
  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm
  ${CMAKE_CURRENT_SOURCE_DIR}/ImageFilters
  ${CMAKE_CURRENT_SOURCE_DIR}/Registration
  ${CMAKE_CURRENT_SOURCE_DIR}/Tasks
  ${Slicer3_Libs_INCLUDE_DIRS}
  ${Slicer3_Base_INCLUDE_DIRS}
  ${vtkSlicerVolumesModuleLogic_SOURCE_DIR}
  ${vtkSlicerVolumesModuleLogic_BINARY_DIR}
  ${Volumes_SOURCE_DIR}
  ${Volumes_BINARY_DIR}
  )


file(GLOB headers 
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/MRML/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/*.txx"
  "${CMAKE_CURRENT_SOURCE_DIR}/ImageFilters/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/Registration/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/Tasks/*.h"
  )

if (EM_Slicer3_FOUND)
  include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/Graph
  )

  file(GLOB em_slicer3_headers
  "${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/Graph/*.h"
  )
  set (headers ${headers} ${em_slicer3_headers})
endif(EM_Slicer3_FOUND)

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

install(FILES 
  ${headers} 
  "${CMAKE_CURRENT_BINARY_DIR}/vtkEMSegmentConfigure.h"
  DESTINATION ${Slicer3_INSTALL_MODULES_INCLUDE_DIR}/${PROJECT_NAME} COMPONENT Development
  )

# --------------------------------------------------------------------------
# Sources

set(EMSegment_SRCS 
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
  vtkEMSegmentMRMLManager.cxx 
  vtkEMSegmentLogic.cxx 
  )


if(EM_Slicer3_FOUND)
  set(EMSegment_SRCS ${EMSegment_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/vtkEMSegmentKWLogic.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/vtkEMSegmentGUI.cxx 
  # wizard
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/vtkEMSegmentStep.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/vtkEMSegmentParametersSetStep.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/vtkEMSegmentIntensityImagesStep.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/vtkEMSegmentAnatomicalStructureStep.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/vtkEMSegmentSpatialPriorsStep.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/vtkEMSegmentNodeParametersStep.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/vtkEMSegmentIntensityDistributionsStep.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/vtkEMSegmentRegistrationParametersStep.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/vtkEMSegmentRunSegmentationStep.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/vtkEMSegmentInputChannelsStep.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/vtkEMSegmentPreProcessingStep.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/vtkEMSegmentKWDynamicFrame.cxx

  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/Graph/vtkImageGraph.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/Graph/vtkIndirectLookupTable.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/Graph/vtkImageDrawObjects.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgets/Graph/vtkImageCurveRegion.cxx
  )
endif(EM_Slicer3_FOUND)

set_source_files_properties(
  # Algorithm sources (temporary exclusion)
  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/EMLocalInterface.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/EMLocalRegistrationCostFunction.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/EMLocalShapeCostFunction.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/vtkDataDef.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/vtkFileOps.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Algorithm/vtkTimeDef.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/ImageFilters/vtkImageMeanIntensityNormalization.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/ImageFilters/vtkSimonParameterReaderWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Registration/vtkBSplineRegistrator.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Registration/vtkRigidRegistrator.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Registration/vtkITKTransformAdapter.cxx
  WRAP_EXCLUDE
  )


#
# Slicer3
#
IF(EM_Slicer3_FOUND)

  # --------------------------------------------------------------------------
  # Wrapping

  include("${VTK_CMAKE_DIR}/vtkWrapTcl.cmake")
  vtk_wrap_tcl3(EMSegment 
    EMSegment_TCL_SRCS 
    "${EMSegment_SRCS}" "")


  #---------------------------------------------------------------------------
  # Add Loadable Module support

  generatelm(EMSegment_SRCS EMSegment.txt)

  # --------------------------------------------------------------------------
  # Build and install the library

  set(lib_name EMSegment)
  add_library(${lib_name} 
    ${EMSegment_SRCS} 
    ${EMSegment_TCL_SRCS}
    )
  slicer3_set_modules_output_path(${lib_name})

  set (EM_SLICER3_GUI_LIBRARIES ${KWWidgets_LIBRARIES})

  target_link_libraries(${lib_name}
      ${Slicer3_Libs_LIBRARIES}
      ${Slicer3_Base_LIBRARIES}
      ${EM_SLICER3_GUI_LIBRARIES} 
      ${ITK_LIBRARIES}
      vtkSlicerVolumesModuleLogic
      Volumes
      )

  # Apply user-defined properties to the library target.
  IF(Slicer3_LIBRARY_PROPERTIES)
      SET_TARGET_PROPERTIES(${lib_name} PROPERTIES
      ${Slicer3_LIBRARY_PROPERTIES}
      )
  ENDIF(Slicer3_LIBRARY_PROPERTIES)

  slicer3_install_modules(${lib_name})

  # --------------------------------------------------------------------------
  # Testing

  if(BUILD_TESTING)
      ADD_SUBDIRECTORY(Testing)
  endif(BUILD_TESTING)

  # --------------------------------------------------------------------------
  # Command line application

  ADD_SUBDIRECTORY(CommandLineApplication)

  # --------------------------------------------------------------------------
  # Install the testing data

  install(DIRECTORY
      Testing/TestData
      DESTINATION ${Slicer3_INSTALL_MODULES_SHARE_DIR}/${PROJECT_NAME}/Testing COMPONENT Development
      PATTERN ".svn" EXCLUDE
      )

ENDIF(EM_Slicer3_FOUND)


#
# Slicer4
#
IF(EM_Slicer4_FOUND)

  # Add subdirectory Qt/
  ADD_SUBDIRECTORY(Qt)

ENDIF(EM_Slicer4_FOUND)

