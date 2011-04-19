#include <algorithm>

#include "vtkEMSegmentLogic.h"
#include "vtkObjectFactory.h"
#include "vtkDirectory.h"
#include "vtkMRMLEMSWorkingDataNode.h"
#include "vtkGridTransform.h"
#include "vtkImageEMLocalSegmenter.h"
#include "vtkImageEMLocalSuperClass.h"
#include "vtkSlicerVolumesLogic.h"
#include "vtkTransformToGrid.h"
#include "vtkIdentityTransform.h"
#include "vtkMRMLEMSAtlasNode.h"
#include "vtkMRMLEMSGlobalParametersNode.h"
#include "vtkMRMLLabelMapVolumeDisplayNode.h"
#include "vtkMRMLEMSTemplateNode.h"

// A helper class to compare two maps
template <class T>
class MapCompare
{
public:
  static bool 
  map_value_comparer(typename std::map<T, unsigned int>::value_type &i1, 
                     typename std::map<T, unsigned int>::value_type &i2)
  {
  return i1.second<i2.second;
  }
};

//----------------------------------------------------------------------------
vtkEMSegmentLogic* vtkEMSegmentLogic::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = 
    vtkObjectFactory::CreateInstance("vtkEMSegmentLogic");
  if(ret)
    {
    return (vtkEMSegmentLogic*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkEMSegmentLogic;
}


//----------------------------------------------------------------------------
vtkEMSegmentLogic::vtkEMSegmentLogic()
{
  this->ModuleName = NULL;

  this->ProgressCurrentAction = NULL;
  this->ProgressGlobalFractionCompleted = 0.0;
  this->ProgressCurrentFractionCompleted = 0.0;

  //this->DebugOn();

  this->MRMLManager = NULL; // NB: must be set before SetMRMLManager is called
  vtkEMSegmentMRMLManager* manager = vtkEMSegmentMRMLManager::New();
  this->SetMRMLManager(manager);
  manager->Delete();
}

//----------------------------------------------------------------------------
vtkEMSegmentLogic::~vtkEMSegmentLogic()
{
  this->SetMRMLManager(NULL);
  this->SetProgressCurrentAction(NULL);
  this->SetModuleName(NULL);
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* 
vtkEMSegmentLogic::AddArchetypeScalarVolume (const char* filename, const char* volname, vtkSlicerApplicationLogic* appLogic,  vtkMRMLScene* mrmlScene)
{
  vtkSlicerVolumesLogic* volLogic  = vtkSlicerVolumesLogic::New();
  volLogic->SetMRMLScene(mrmlScene);
  volLogic->SetApplicationLogic(appLogic);
  vtkMRMLScalarVolumeNode* volNode = volLogic->AddArchetypeScalarVolume(filename, volname,2);
  volLogic->Delete();
  return  volNode;
}


//----------------------------------------------------------------------------
bool
vtkEMSegmentLogic::
StartPreprocessingInitializeInputData()
{
  this->MRMLManager->GetWorkingDataNode()->SetInputTargetNodeIsValid(1);
  this->MRMLManager->GetWorkingDataNode()->SetInputAtlasNodeIsValid(1);
  this->MRMLManager->GetWorkingDataNode()->SetAlignedTargetNodeIsValid(0);
  this->MRMLManager->GetWorkingDataNode()->SetAlignedAtlasNodeIsValid(0);

  return true;
}

//----------------------------------------------------------------------------
void
vtkEMSegmentLogic::
PrintImageInfo(vtkMRMLVolumeNode* volumeNode)
{
  if (volumeNode == NULL || volumeNode->GetImageData() == NULL)
    {
    std::cout << "Volume node or image data is null" << std::endl;
    return;
    }

  // extent
  int extent[6];
  volumeNode->GetImageData()->GetExtent(extent);
  std::cout << "Extent: " << std::endl;
  std::cout  << extent[0] << " " << extent[1] << " " << extent[2] << " " << extent[3] << " " << extent[4] << " " << extent[5] << std::endl;

  // ijkToRAS
  vtkMatrix4x4* matrix = vtkMatrix4x4::New();
  volumeNode->GetIJKToRASMatrix(matrix);
  std::cout << "IJKtoRAS Matrix: " << std::endl;
  for (unsigned int r = 0; r < 4; ++r)
    {
    std::cout << "   ";
    for (unsigned int c = 0; c < 4; ++c)
      {
      std::cout 
        << matrix->GetElement(r,c)
        << "   ";
      }
    std::cout << std::endl;
    }  
  matrix->Delete();
}

// a utility to print out a vtk image origin, spacing, and extent
//----------------------------------------------------------------------------
void
vtkEMSegmentLogic::
PrintImageInfo(vtkImageData* image)
{
  double spacing[3];
  double origin[3];
  int extent[6];

  image->GetSpacing(spacing);
  image->GetOrigin(origin);
  image->GetExtent(extent);

  std::cout << "Spacing: " << spacing[0] << " " << spacing[1] << " " << spacing[2] << std::endl;
  std::cout << "Origin: " << origin[0] << " " << origin[1] << " " << origin[2] << std::endl;
  std::cout << "Extent: " << extent[0] << " " << extent[1] << " " << extent[2] << " " << extent[3] << " " << extent[4] << " " << extent[5] << std::endl;
}

//----------------------------------------------------------------------------
bool 
vtkEMSegmentLogic::
IsVolumeGeometryEqual(vtkMRMLVolumeNode* lhs,
                      vtkMRMLVolumeNode* rhs)
{
  if (lhs == NULL || rhs == NULL ||
      lhs->GetImageData() == NULL || rhs->GetImageData() == NULL)
    {
    return false;
    }

  // check extent
  int extentLHS[6];
  lhs->GetImageData()->GetExtent(extentLHS);
  int extentRHS[6];
  rhs->GetImageData()->GetExtent(extentRHS);
  bool equalExent = std::equal(extentLHS, extentLHS+6, extentRHS);
  
  // check ijkToRAS
  vtkMatrix4x4* matrixLHS = vtkMatrix4x4::New();
  lhs->GetIJKToRASMatrix(matrixLHS);
  vtkMatrix4x4* matrixRHS = vtkMatrix4x4::New();
  rhs->GetIJKToRASMatrix(matrixRHS);  
  bool equalMatrix = true;
  for (int r = 0; r < 4; ++r)
    {
    for (int c = 0; c < 4; ++c)
      {
        // Otherwise small errors will cause that they are not equal but should be ignored !
    if (double(int((*matrixLHS)[r][c]*100000)/100000.0) != double(int((*matrixRHS)[r][c]*100000)/100000.0))
        {
        equalMatrix = false;
    break;
        }
      }
    }

  matrixLHS->Delete();
  matrixRHS->Delete();
  return equalExent && equalMatrix;
}

// loops through the faces of the image bounding box and counts all the different image values and stores them in a map
// T represents the image data type
template <class T>
T
vtkEMSegmentLogic::
GuessRegistrationBackgroundLevel(vtkImageData* imageData)
{
  int borderWidth = 5;
  T inLevel;
  typedef std::map<T, unsigned int> MapType;
  MapType m;
  long totalVoxelsCounted = 0;

  T* inData = static_cast<T*>(imageData->GetScalarPointer());
  int dim[3];
  imageData->GetDimensions(dim);

  vtkIdType inc[3];
  vtkIdType iInc, jInc, kInc;
  imageData->GetIncrements(inc);

   // k first slice
  for (int k = 0; k < borderWidth; ++k)
    {
    kInc = k*inc[2];
    for (int j = 0; j < dim[1]; ++j)
      {
      jInc = j*inc[1];
      for (int i = 0; i < dim[0]; ++i)
        {
        iInc = i*inc[0];
        inLevel = inData[iInc+jInc+kInc];
        if (m.count(inLevel))
          {
          ++m[inLevel];
          }
        else
          {
          m[inLevel] = 1;
          }
        ++totalVoxelsCounted;
        }
      }
    }

  // k last slice
  for (int k = dim[2]-borderWidth; k < dim[2]; ++k)
    {
    kInc = k*inc[2];
    for (int j = 0; j < dim[1]; ++j)
      {
      jInc = j*inc[1];
      for (int i = 0; i < dim[0]; ++i)
        {
        iInc = i*inc[0];
        inLevel = inData[iInc+jInc+kInc];
        if (m.count(inLevel))
          {
          ++m[inLevel];
          }
        else
          {
          m[inLevel] = 1;
          }
        ++totalVoxelsCounted;
        }
      }
    }

  // j first slice
  for (int j = 0; j < borderWidth; ++j)
    {
    jInc = j*inc[1];
    for (int k = 0; k < dim[2]; ++k)
      {
      kInc = k*inc[2];
      for (int i = 0; i < dim[0]; ++i)
        {
        iInc = i*inc[0];
        inLevel = inData[iInc+jInc+kInc];
        if (m.count(inLevel))
          {
          ++m[inLevel];
          }
        else
          {
          m[inLevel] = 1;
          }
        ++totalVoxelsCounted;
        }
      }
    }

  // j last slice
  for (int j = dim[1]-borderWidth; j < dim[1]; ++j)
    {
    jInc = j*inc[1];
    for (int k = 0; k < dim[2]; ++k)
      {
      kInc = k*inc[2];
      for (int i = 0; i < dim[0]; ++i)
        {
        iInc = i*inc[0];
        inLevel = inData[iInc+jInc+kInc];
        if (m.count(inLevel))
          {
          ++m[inLevel];
          }
        else
          {
          m[inLevel] = 1;
          }
        ++totalVoxelsCounted;
        }
      }
    }

  // i first slice
  for (int i = 0; i < borderWidth; ++i)
    {
    iInc = i*inc[0];
    for (int k = 0; k < dim[2]; ++k)
      {
      kInc = k*inc[2];
      for (int j = 0; j < dim[1]; ++j)
        {
        jInc = j*inc[1];
        inLevel = inData[iInc+jInc+kInc];
        if (m.count(inLevel))
          {
          ++m[inLevel];
          }
        else
          {
          m[inLevel] = 1;
          }
        ++totalVoxelsCounted;
        }
      }
    }

  // i last slice
  for (int i = dim[0]-borderWidth; i < dim[0]; ++i)
    {
    iInc = i*inc[0];
    for (int k = 0; k < dim[2]; ++k)
      {
      kInc = k*inc[2];
      for (int j = 0; j < dim[1]; ++j)
        {
        jInc = j*inc[1];
        inLevel = inData[iInc+jInc+kInc];
        if (m.count(inLevel))
          {
          ++m[inLevel];
          }
        else
          {
          m[inLevel] = 1;
          }
        ++totalVoxelsCounted;
        }
      }
    }

  // all the information is stored in map m :  std::map<T, unsigned int>

  if (m.empty())
    {
    // no image data provided?
    return 0;
    }
  else if (m.size() == 1)
    {
      // Homogeneous background
      return m.begin()->first;
   }
  else
    {
    // search for the largest element
    typename MapType::iterator itor = 
      std::max_element(m.begin(), m.end(),
                       MapCompare<T>::map_value_comparer);

    // the iterator is pointing to the element with the largest value in the range [m.begin(), m.end()]
    T backgroundLevel = itor->first;

    // how many counts?
    double percentageOfVoxels = 
      100.0 * static_cast<double>(itor->second)/totalVoxelsCounted;

    std::cout << "   Background level guess : "<< std::endl
              << "   first place: "
              << static_cast<int>(backgroundLevel) << " (" << percentageOfVoxels << "%) "
              << std::endl;


    // erase largest element
    m.erase(itor);


    // again, search for the largest element (second place)
    typename MapType::iterator itor2 = 
      std::max_element(m.begin(), m.end(),
                       MapCompare<T>::map_value_comparer);

    T backgroundLevel_second_place = itor2->first;

    double percentageOfVoxels_secondplace =
      100.0 * static_cast<double>(itor2->second)/totalVoxelsCounted;

    std::cout << "   second place: "
              << static_cast<int>(backgroundLevel_second_place) << " (" << percentageOfVoxels_secondplace << "%)"
              << std::endl;

    return backgroundLevel;
    }
}

//
// A Slicer3 wrapper around vtkImageReslice.  Reslice the image data
// from inputVolumeNode into outputVolumeNode with the output image
// geometry specified by outputVolumeGeometryNode.  Optionally specify
// a transform.  The reslice transform will be:
//
// outputIJK->outputRAS->(outputRASToInputRASTransform)->inputRAS->inputIJK
//
//----------------------------------------------------------------------------
void
vtkEMSegmentLogic::
SlicerImageReslice(vtkMRMLVolumeNode* inputVolumeNode,
                   vtkMRMLVolumeNode* outputVolumeNode,
                   vtkMRMLVolumeNode* outputVolumeGeometryNode,
                   vtkTransform* outputRASToInputRASTransform,
                   int interpolationType,
                   double backgroundLevel)
{
  vtkImageData* inputImageData  = inputVolumeNode->GetImageData();
  vtkImageData* outputImageData = outputVolumeNode->GetImageData();
  vtkImageData* outputGeometryData = NULL;
  if (outputVolumeGeometryNode != NULL)
    {
    outputGeometryData = outputVolumeGeometryNode->GetImageData();
    }

  vtkImageReslice* resliceFilter = vtkImageReslice::New();

  //
  // set inputs
  resliceFilter->SetInput(inputImageData);

  //
  // set geometry
  if (outputGeometryData != NULL)
    {
    resliceFilter->SetInformationInput(outputGeometryData);
    outputVolumeNode->CopyOrientation(outputVolumeGeometryNode);
    }

  //
  // setup total transform
  // ijk of output -> RAS -> XFORM -> RAS -> ijk of input
  vtkTransform* totalTransform = vtkTransform::New();
  if (outputRASToInputRASTransform != NULL)
    {
    totalTransform->DeepCopy(outputRASToInputRASTransform);
    }

  vtkMatrix4x4* outputIJKToRAS  = vtkMatrix4x4::New();
  outputVolumeNode->GetIJKToRASMatrix(outputIJKToRAS);
  vtkMatrix4x4* inputRASToIJK = vtkMatrix4x4::New();
  inputVolumeNode->GetRASToIJKMatrix(inputRASToIJK);

  totalTransform->PreMultiply();
  totalTransform->Concatenate(outputIJKToRAS);
  totalTransform->PostMultiply();
  totalTransform->Concatenate(inputRASToIJK);
  resliceFilter->SetResliceTransform(totalTransform);

  //
  // resample the image
  resliceFilter->SetBackgroundLevel(backgroundLevel);
  resliceFilter->OptimizationOn();

  switch (interpolationType)
    {
    case vtkEMSegmentMRMLManager::InterpolationNearestNeighbor:
      resliceFilter->SetInterpolationModeToNearestNeighbor();
      break;
    case vtkEMSegmentMRMLManager::InterpolationCubic:
      resliceFilter->SetInterpolationModeToCubic();
      break;
    case vtkEMSegmentMRMLManager::InterpolationLinear:
    default:
      resliceFilter->SetInterpolationModeToLinear();
    }

  resliceFilter->Update();
  outputImageData->ShallowCopy(resliceFilter->GetOutput());

  //
  // clean up
  outputIJKToRAS->Delete();
  inputRASToIJK->Delete();
  resliceFilter->Delete();
  totalTransform->Delete();
}

// Assume geometry is already specified, create
// outGrid(p) = postMultiply \circ inGrid \circ preMultiply (p)
//
// right now simplicity over speed.  Optimize later?
//----------------------------------------------------------------------------
void
vtkEMSegmentLogic::
ComposeGridTransform(vtkGridTransform* inGrid,
                     vtkMatrix4x4*     preMultiply,
                     vtkMatrix4x4*     postMultiply,
                     vtkGridTransform* outGrid)
{
  // iterate over output grid
  double inPt[4] = {0, 0, 0, 1};
  double pt[4]   = {0, 0, 0, 1};
  double* outDataPtr = 
    static_cast<double*>(outGrid->GetDisplacementGrid()->GetScalarPointer());  
  vtkIdType numOutputVoxels = outGrid->GetDisplacementGrid()->
    GetNumberOfPoints();

  for (vtkIdType i = 0; i < numOutputVoxels; ++i)
    {
    outGrid->GetDisplacementGrid()->GetPoint(i, inPt);
    preMultiply->MultiplyPoint(inPt, pt);
    inGrid->TransformPoint(pt, pt);
    postMultiply->MultiplyPoint(pt, pt);
    
    *outDataPtr++ = pt[0] - inPt[0];
    *outDataPtr++ = pt[1] - inPt[1];
    *outDataPtr++ = pt[2] - inPt[2];
    }
}

//
// A Slicer3 wrapper around vtkImageReslice.  Reslice the image data
// from inputVolumeNode into outputVolumeNode with the output image
// geometry specified by outputVolumeGeometryNode.  Optionally specify
// a transform.  The reslice transorm will be:
//
// outputIJK->outputRAS->(outputRASToInputRASTransform)->inputRAS->inputIJK
//
//----------------------------------------------------------------------------
void
vtkEMSegmentLogic::
SlicerImageResliceWithGrid(vtkMRMLVolumeNode* inputVolumeNode,
                           vtkMRMLVolumeNode* outputVolumeNode,
                           vtkMRMLVolumeNode* outputVolumeGeometryNode,
                           vtkGridTransform* outputRASToInputRASTransform,
                           int interpolationType,
                           double backgroundLevel)
{
  vtkImageData* inputImageData  = inputVolumeNode->GetImageData();
  vtkImageData* outputImageData = outputVolumeNode->GetImageData();
  vtkImageData* outputGeometryData = NULL;
  if (outputVolumeGeometryNode != NULL)
    {
    outputGeometryData = outputVolumeGeometryNode->GetImageData();
    }

  vtkImageReslice* resliceFilter = vtkImageReslice::New();

  //
  // set inputs
  resliceFilter->SetInput(inputImageData);

  //
  // create total transform
  vtkTransformToGrid* gridSource = vtkTransformToGrid::New();
  vtkIdentityTransform* idTransform = vtkIdentityTransform::New();
  gridSource->SetInput(idTransform);
  //gridSource->SetGridScalarType(VTK_FLOAT);
  idTransform->Delete();

  //
  // set geometry
  if (outputGeometryData != NULL)
    {
    resliceFilter->SetInformationInput(outputGeometryData);
    outputVolumeNode->CopyOrientation(outputVolumeGeometryNode);

    gridSource->SetGridExtent(outputGeometryData->GetExtent());
    gridSource->SetGridSpacing(outputGeometryData->GetSpacing());
    gridSource->SetGridOrigin(outputGeometryData->GetOrigin());
    }
  else
    {
    gridSource->SetGridExtent(outputImageData->GetExtent());
    gridSource->SetGridSpacing(outputImageData->GetSpacing());
    gridSource->SetGridOrigin(outputImageData->GetOrigin());
    }
  gridSource->Update();
  vtkGridTransform* totalTransform = vtkGridTransform::New();
  totalTransform->SetDisplacementGrid(gridSource->GetOutput());
//  totalTransform->SetInterpolationModeToCubic();
  gridSource->Delete();
  
  //
  // fill in total transform
  // ijk of output -> RAS -> XFORM -> RAS -> ijk of input
  vtkMatrix4x4* outputIJKToRAS  = vtkMatrix4x4::New();
  outputVolumeNode->GetIJKToRASMatrix(outputIJKToRAS);
  vtkMatrix4x4* inputRASToIJK = vtkMatrix4x4::New();
  inputVolumeNode->GetRASToIJKMatrix(inputRASToIJK);
  vtkEMSegmentLogic::ComposeGridTransform(outputRASToInputRASTransform,
                                          outputIJKToRAS,
                                          inputRASToIJK,
                                          totalTransform);
  resliceFilter->SetResliceTransform(totalTransform);

  //
  // resample the image
  resliceFilter->SetBackgroundLevel(backgroundLevel);
  resliceFilter->OptimizationOn();

  switch (interpolationType)
    {
    case vtkEMSegmentMRMLManager::InterpolationNearestNeighbor:
      resliceFilter->SetInterpolationModeToNearestNeighbor();
      break;
    case vtkEMSegmentMRMLManager::InterpolationCubic:
      resliceFilter->SetInterpolationModeToCubic();
      break;
    case vtkEMSegmentMRMLManager::InterpolationLinear:
    default:
      resliceFilter->SetInterpolationModeToLinear();
    }

  resliceFilter->Update();
  outputImageData->ShallowCopy(resliceFilter->GetOutput());

  //
  // clean up
  outputIJKToRAS->Delete();
  inputRASToIJK->Delete();
  resliceFilter->Delete();
  totalTransform->Delete();
}

//----------------------------------------------------------------------------
void vtkEMSegmentLogic::StartPreprocessingResampleAndCastToTarget(vtkMRMLVolumeNode* movingVolumeNode, vtkMRMLVolumeNode* fixedVolumeNode, vtkMRMLVolumeNode* outputVolumeNode)
{
  if (!vtkEMSegmentLogic::IsVolumeGeometryEqual(fixedVolumeNode, outputVolumeNode))
    {

      std::cout << "Warning: Target-to-target registration skipped but "
                << "target images have differenent geometries. "
                << std::endl
                << "Suggestion: If you are not positive that your images are "
                << "aligned, you should enable target-to-target registration."
                << std::endl;

      std::cout << "Fixed Volume Node: " << std::endl;
      PrintImageInfo(fixedVolumeNode);
      std::cout << "Output Volume Node: " << std::endl;
      PrintImageInfo(outputVolumeNode);

      // std::cout << "Resampling target image " << i << "...";
      double backgroundLevel = 0;
      switch (movingVolumeNode->GetImageData()->GetScalarType())
        {  
          vtkTemplateMacro(backgroundLevel = (GuessRegistrationBackgroundLevel<VTK_TT>(movingVolumeNode->GetImageData())););
        }
      std::cout << "   Guessed background level: " << backgroundLevel << std::endl;

      vtkEMSegmentLogic::SlicerImageReslice(movingVolumeNode, 
                                            outputVolumeNode, 
                                            fixedVolumeNode,
                                            NULL,
                                            vtkEMSegmentMRMLManager::InterpolationLinear,
                                            backgroundLevel);
    }

  if (fixedVolumeNode->GetImageData()->GetScalarType() != movingVolumeNode->GetImageData()->GetScalarType())
    {
      //cast
      vtkImageCast* cast = vtkImageCast::New();
      cast->SetInput(outputVolumeNode->GetImageData());
      cast->SetOutputScalarType(fixedVolumeNode->GetImageData()->GetScalarType());
      cast->Update();
      outputVolumeNode->GetImageData()->DeepCopy(cast->GetOutput());
      cast->Delete();
    }
  std::cout << "Resampling and casting output volume \"" << outputVolumeNode->GetName() << "\" to reference target \"" << fixedVolumeNode->GetName() <<  "\" DONE" << std::endl;
}

//----------------------------------------------------------------------------
double vtkEMSegmentLogic::GuessRegistrationBackgroundLevel(vtkMRMLVolumeNode* volumeNode)
{
  if (!volumeNode ||  !volumeNode->GetImageData())  
    {
      std::cerr << "double vtkEMSegmentLogic::GuessRegistrationBackgroundLevel(vtkMRMLVolumeNode* volumeNode) : volumeNode or volumeNode->GetImageData is null" << std::endl;
      return -1;
    }

  // guess background level    
  double backgroundLevel = 0;
  switch (volumeNode->GetImageData()->GetScalarType())
      {  
        vtkTemplateMacro(backgroundLevel = (GuessRegistrationBackgroundLevel<VTK_TT>(volumeNode->GetImageData())););
      }
  std::cout << "   Guessed background level: " << backgroundLevel << std::endl;
  return backgroundLevel;
}

//-----------------------------------------------------------------------------
vtkIntArray*
vtkEMSegmentLogic::
NewObservableEvents()
{
  vtkIntArray *events = vtkIntArray::New();
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);

  return events;
}

//-----------------------------------------------------------------------------
void
vtkEMSegmentLogic::
CopyDataToSegmenter(vtkImageEMLocalSegmenter* segmenter)
{
  //
  // copy atlas related parameters to algorithm
  //
  vtkstd::cout << "atlas data...";
  this->CopyAtlasDataToSegmenter(segmenter);

  //
  // copy target related parameters to algorithm
  //
  vtkstd::cout << "target data...";
  this->CopyTargetDataToSegmenter(segmenter);

  //
  // copy global parameters to algorithm 
  //
  vtkstd::cout << "global data...";
  this->CopyGlobalDataToSegmenter(segmenter);

  //
  // copy tree base parameters to algorithm
  //
  vtkstd::cout << "tree data...";
  vtkImageEMLocalSuperClass* rootNode = vtkImageEMLocalSuperClass::New();
  this->CopyTreeDataToSegmenter(rootNode, 
                                this->MRMLManager->GetTreeRootNodeID());
  segmenter->SetHeadClass(rootNode);
  rootNode->Delete();
}

//-----------------------------------------------------------------------------
void
vtkEMSegmentLogic::
CopyAtlasDataToSegmenter(vtkImageEMLocalSegmenter* segmenter)
{
  segmenter->
    SetNumberOfTrainingSamples(this->MRMLManager->
                               GetAtlasNumberOfTrainingSamples());
}

//-----------------------------------------------------------------------------
void
vtkEMSegmentLogic::
CopyTargetDataToSegmenter(vtkImageEMLocalSegmenter* segmenter)
{
  // !!! todo: TESTING HERE!!!
  vtkMRMLEMSVolumeCollectionNode* workingTarget = 
    this->MRMLManager->GetWorkingDataNode()->GetAlignedTargetNode();
  unsigned int numTargetImages = workingTarget->GetNumberOfVolumes();
  std::cout << "Setting number of target images: " << numTargetImages 
            << std::endl;
  segmenter->SetNumInputImages(numTargetImages);

  for (unsigned int i = 0; i < numTargetImages; ++i)
    {
    std::string mrmlID = workingTarget->GetNthVolumeNodeID(i);
    vtkDebugMacro("Setting target image " << i << " mrmlID=" 
                  << mrmlID.c_str());

    vtkImageData* imageData = 
      workingTarget->GetNthVolumeNode(i)->GetImageData();

    std::cout << "AddingTargetImage..." << std::endl;
    this->PrintImageInfo(imageData);
    imageData->Update();
    this->PrintImageInfo(imageData);

    segmenter->SetImageInput(i, imageData);
    }
}

//-----------------------------------------------------------------------------
void
vtkEMSegmentLogic::
CopyGlobalDataToSegmenter(vtkImageEMLocalSegmenter* segmenter)
{
  if (this->MRMLManager->GetEnableMultithreading())
    {
    segmenter->
      SetDisableMultiThreading(0);
    }
  else
    {
    segmenter->
      SetDisableMultiThreading(1);
    }
  segmenter->SetPrintDir(this->MRMLManager->GetSaveWorkingDirectory());
  
  //
  // NB: In the algorithm code smoothing widht and sigma parameters
  // are defined globally.  In this logic, they are defined for each
  // parent node.  For now copy parameters from the root tree
  // node. !!!todo!!!
  //
  vtkIdType rootNodeID = this->MRMLManager->GetTreeRootNodeID();
  segmenter->
    SetSmoothingWidth(this->MRMLManager->
                      GetTreeNodeSmoothingKernelWidth(rootNodeID));

  // type mismatch between logic and algorithm !!!todo!!!
  int intSigma = 
    vtkMath::Round(this->MRMLManager->
                   GetTreeNodeSmoothingKernelSigma(rootNodeID));
  segmenter->SetSmoothingSigma(intSigma);

  //
  // registration parameters
  //
  int algType = this->ConvertGUIEnumToAlgorithmEnumInterpolationType
    (this->MRMLManager->GetRegistrationInterpolationType());
  segmenter->SetRegistrationInterpolationType(algType);
}

//-----------------------------------------------------------------------------
void
vtkEMSegmentLogic::
CopyTreeDataToSegmenter(vtkImageEMLocalSuperClass* node, vtkIdType nodeID)
{
  // need this here because the vtkImageEM* classes don't use
  // virtual functions and so failed initializations lead to
  // memory errors
  node->SetNumInputImages(this->MRMLManager->
                          GetTargetNumberOfSelectedVolumes());

  // copy generic tree node data to segmenter
  this->CopyTreeGenericDataToSegmenter(node, nodeID);
  
  // copy parent specific tree node data to segmenter
  this->CopyTreeParentDataToSegmenter(node, nodeID);

  // add children
  unsigned int numChildren = 
    this->MRMLManager->GetTreeNodeNumberOfChildren(nodeID);
  double totalProbability = 0.0;
  for (unsigned int i = 0; i < numChildren; ++i)
    {
    vtkIdType childID = this->MRMLManager->GetTreeNodeChildNodeID(nodeID, i);
    bool isLeaf = this->MRMLManager->GetTreeNodeIsLeaf(childID);

    if (isLeaf)
      {
      vtkImageEMLocalClass* childNode = vtkImageEMLocalClass::New();
      // need this here because the vtkImageEM* classes don't use
      // virtual functions and so failed initializations lead to
      // memory errors
      childNode->SetNumInputImages(this->MRMLManager->
                                   GetTargetNumberOfSelectedVolumes());
      this->CopyTreeGenericDataToSegmenter(childNode, childID);
      this->CopyTreeLeafDataToSegmenter(childNode, childID);
      node->AddSubClass(childNode, i);
      childNode->Delete();
      }
    else
      {
      vtkImageEMLocalSuperClass* childNode = vtkImageEMLocalSuperClass::New();
      this->CopyTreeDataToSegmenter(childNode, childID);
      node->AddSubClass(childNode, i);
      childNode->Delete();
      }

    totalProbability += 
      this->MRMLManager->GetTreeNodeClassProbability(childID);
    }

  if (totalProbability != 1.0)
    {
    vtkWarningMacro("Warning: child probabilities don't sum to unity for node "
                    << this->MRMLManager->GetTreeNodeName(nodeID)
                    << " they sum to " << totalProbability);
    }

  // Set Markov matrices
  const unsigned int numDirections = 6;
  for (unsigned int d = 0; d < numDirections; ++d)
    {
    for (unsigned int r = 0; r < numChildren; ++r)
      {
      for (unsigned int c = 0; c < numChildren; ++c)
        {
          double val = (r == c ? 1.0 : 0.0);
          node->SetMarkovMatrix(val, d, c, r);
        }
      }
    }
  node->Update();
}

//-----------------------------------------------------------------------------
void vtkEMSegmentLogic::DefineValidSegmentationBoundary() 
{
 //
  // Setup ROI.  If if looks bogus then use the default (entire image)
  bool useDefaultBoundary = false;
  int boundMin[3];
  int boundMax[3];

  // get dimensions of target image
  int targetImageDimensions[3];
  this->MRMLManager->GetTargetInputNode()->GetNthVolumeNode(0)->
    GetImageData()->GetDimensions(targetImageDimensions);

  this->MRMLManager->GetSegmentationBoundaryMin(boundMin);
  this->MRMLManager->GetSegmentationBoundaryMax(boundMax);
  // Specify boundary in 1-based, NOT 0-based as you might expect
  for (unsigned int i = 0; i < 3; ++i)
    {
    if (boundMin[i] <  1 || 
        boundMin[i] >  targetImageDimensions[i]   ||
        boundMax[i] <  1                   ||
        boundMax[i] >  targetImageDimensions[i]   ||
        boundMax[i] <  boundMin[i])
      {
      useDefaultBoundary = true;
      break;
      }
    }
  if (useDefaultBoundary)
    {
    std::cout 
      << std::endl
      << "====================================================================" << std::endl
      << "Warning: the segmentation ROI was bogus, setting ROI to entire image"  << std::endl
      << "Axis 0 -  Image Min: 1 <= RoiMin(" << boundMin[0] << ") <= ROIMax(" << boundMax[0] <<") <=  Image Max:" << targetImageDimensions[0] <<  std::endl
      << "Axis 1 -  Image Min: 1 <= RoiMin(" << boundMin[1] << ") <= ROIMax(" << boundMax[1] << ") <=  Image Max:" << targetImageDimensions[1] <<  std::endl
      << "Axis 2 -  Image Min: 1 <= RoiMin(" << boundMin[2] << ") <= ROIMax(" << boundMax[2] << ") <=  Image Max:" << targetImageDimensions[2] <<  std::endl
      << "NOTE: The above warning about ROI should not lead to poor segmentation results;  the entire image should be segmented.  It only indicates an error if you intended to segment a subregion of the image."
      << std::endl
      << "Define Boundary as: ";
      for (unsigned int i = 0; i < 3; ++i)
        {
          boundMin[i] = 1;
          boundMax[i] = targetImageDimensions[i];
          std::cout << boundMin[i] << ", " << boundMax[i] << ",   ";
        }
      std::cout << std::endl << "====================================================================" << std::endl;

      this->MRMLManager->SetSegmentationBoundaryMin(boundMin);
      this->MRMLManager->SetSegmentationBoundaryMax(boundMax); 
    }
}

void
vtkEMSegmentLogic::
CopyTreeGenericDataToSegmenter(vtkImageEMLocalGenericClass* node, 
                               vtkIdType nodeID)
{
  unsigned int numTargetImages = 
  this->MRMLManager->GetTargetNumberOfSelectedVolumes();

 
  this->DefineValidSegmentationBoundary();
  int boundMin[3];
  int boundMax[3];
  this->MRMLManager->GetSegmentationBoundaryMin(boundMin);
  this->MRMLManager->GetSegmentationBoundaryMax(boundMax);
  node->SetSegmentationBoundaryMin(boundMin[0], boundMin[1], boundMin[2]);
  node->SetSegmentationBoundaryMax(boundMax[0], boundMax[1], boundMax[2]);

  node->SetProbDataWeight(this->MRMLManager->
                          GetTreeNodeSpatialPriorWeight(nodeID));

  node->SetTissueProbability(this->MRMLManager->
                             GetTreeNodeClassProbability(nodeID));

  node->SetPrintWeights(this->MRMLManager->GetTreeNodePrintWeight(nodeID));

  // set target input channel weights
  for (unsigned int i = 0; i < numTargetImages; ++i)
    {
    node->SetInputChannelWeights(this->MRMLManager->
                                 GetTreeNodeInputChannelWeight(nodeID, 
                                                               i), i);
    }

  //
  // registration related data
  //
  //!!!bcd!!!

  //
  // set probability data
  //

  // get working atlas
  // !!! error checking!
  vtkMRMLVolumeNode*  atlasNode = this->MRMLManager->GetAlignedSpatialPriorFromTreeNodeID(nodeID);
  if (atlasNode)
    {
    vtkDebugMacro("Setting spatial prior: node=" 
                  << this->MRMLManager->GetTreeNodeName(nodeID));
    vtkImageData* imageData = atlasNode->GetImageData();
    node->SetProbDataPtr(imageData);
    }

  int exclude =  this->MRMLManager->GetTreeNodeExcludeFromIncompleteEStep(nodeID);
  node->SetExcludeFromIncompleteEStepFlag(exclude);
}


//-----------------------------------------------------------------------------
void
vtkEMSegmentLogic::
CopyTreeParentDataToSegmenter(vtkImageEMLocalSuperClass* node, 
                              vtkIdType nodeID)
{
  node->SetPrintFrequency (this->MRMLManager->
                           GetTreeNodePrintFrequency(nodeID));
  node->SetPrintBias      (this->MRMLManager->
                           GetTreeNodePrintBias(nodeID));
  node->SetPrintLabelMap  (this->MRMLManager->
                           GetTreeNodePrintLabelMap(nodeID));

  node->SetPrintEMLabelMapConvergence
    (this->MRMLManager->GetTreeNodePrintEMLabelMapConvergence(nodeID));
  node->SetPrintEMWeightsConvergence
    (this->MRMLManager->GetTreeNodePrintEMWeightsConvergence(nodeID));
  node->SetStopEMType(this->ConvertGUIEnumToAlgorithmEnumStoppingConditionType
                      (this->MRMLManager->
                      GetTreeNodeStoppingConditionEMType(nodeID)));
  node->SetStopEMValue(this->MRMLManager->
                       GetTreeNodeStoppingConditionEMValue(nodeID));
  node->SetStopEMMaxIter
    (this->MRMLManager->GetTreeNodeStoppingConditionEMIterations(nodeID));

  node->SetPrintMFALabelMapConvergence
    (this->MRMLManager->GetTreeNodePrintMFALabelMapConvergence(nodeID));
  node->SetPrintMFAWeightsConvergence
    (this->MRMLManager->GetTreeNodePrintMFAWeightsConvergence(nodeID));
  node->SetStopMFAType(this->ConvertGUIEnumToAlgorithmEnumStoppingConditionType
                       (this->MRMLManager->
                       GetTreeNodeStoppingConditionMFAType(nodeID)));
  node->SetStopMFAValue(this->MRMLManager->
                        GetTreeNodeStoppingConditionMFAValue(nodeID));
  node->SetStopMFAMaxIter
    (this->MRMLManager->GetTreeNodeStoppingConditionMFAIterations(nodeID));

  node->SetStopBiasCalculation
    (this->MRMLManager->GetTreeNodeBiasCalculationMaxIterations(nodeID));

  node->SetPrintShapeSimularityMeasure(0);         // !!!bcd!!!

  node->SetPCAShapeModelType(0);                   // !!!bcd!!!

  node->SetRegistrationIndependentSubClassFlag(0); // !!!bcd!!!
  node->SetRegistrationType(0);                    // !!!bcd!!!

  node->SetGenerateBackgroundProbability
    (this->MRMLManager->GetTreeNodeGenerateBackgroundProbability(nodeID));

  // New in 3.6. : Alpha now reflects user interface and is now correctly set for each parent node
  // cout << "Alpha setting for " << this->MRMLManager->GetTreeNodeName(nodeID) << " " << this->MRMLManager->GetTreeNodeAlpha(nodeID) << endl;
  node->SetAlpha(this->MRMLManager->GetTreeNodeAlpha(nodeID)); 
                      
}

//-----------------------------------------------------------------------------
void
vtkEMSegmentLogic::
CopyTreeLeafDataToSegmenter(vtkImageEMLocalClass* node, 
                            vtkIdType nodeID)
{
  unsigned int numTargetImages = 
    this->MRMLManager->GetTargetNumberOfSelectedVolumes();

  // this label describes the output intensity value for this class in
  // the segmentation result
  node->SetLabel(this->MRMLManager->GetTreeNodeIntensityLabel(nodeID));

  // set log mean and log covariance
  for (unsigned int r = 0; r < numTargetImages; ++r)
    {
    node->SetLogMu(this->MRMLManager->
                   GetTreeNodeDistributionLogMeanWithCorrection(nodeID, r), r);

    for (unsigned int c = 0; c < numTargetImages; ++c)
      {
      node->SetLogCovariance(this->MRMLManager->
                             GetTreeNodeDistributionLogCovarianceWithCorrection(nodeID,
                                                                  r, c), 
                             r, c);
      }
    }

  node->SetPrintQuality(this->MRMLManager->GetTreeNodePrintQuality(nodeID));
}

//-----------------------------------------------------------------------------
int
vtkEMSegmentLogic::
ConvertGUIEnumToAlgorithmEnumStoppingConditionType(int guiEnumValue)
{
  switch (guiEnumValue)
    {
    case (vtkEMSegmentMRMLManager::StoppingConditionIterations):
      return EMSEGMENT_STOP_FIXED;
    case (vtkEMSegmentMRMLManager::StoppingConditionLabelMapMeasure):
      return EMSEGMENT_STOP_LABELMAP;
    case (vtkEMSegmentMRMLManager::StoppingConditionWeightsMeasure):
      return EMSEGMENT_STOP_WEIGHTS;
    default:
      vtkErrorMacro("Unknown stopping condition type: " << guiEnumValue);
      return -1;
    }
}

//-----------------------------------------------------------------------------
int
vtkEMSegmentLogic::
ConvertGUIEnumToAlgorithmEnumInterpolationType(int guiEnumValue)
{
  switch (guiEnumValue)
    {
    case (vtkEMSegmentMRMLManager::InterpolationLinear):
      return EMSEGMENT_REGISTRATION_INTERPOLATION_LINEAR;
    case (vtkEMSegmentMRMLManager::InterpolationNearestNeighbor):
      return EMSEGMENT_REGISTRATION_INTERPOLATION_NEIGHBOUR;
    case (vtkEMSegmentMRMLManager::InterpolationCubic):
      // !!! not implemented
      vtkErrorMacro("Cubic interpolation not implemented: " << guiEnumValue);
      return -1;
    default:
      vtkErrorMacro("Unknown interpolation type: " << guiEnumValue);
      return -1;
    }
}

//----------------------------------------------------------------------------
vtkstd::string  vtkEMSegmentLogic::GetTclGeneralDirectory()
{
  // Later do automatically
  vtkstd::string file_path = this->GetModuleShareDirectory() +  vtkstd::string("/Tcl");
  return vtksys::SystemTools::ConvertToOutputPath(file_path.c_str());
}

//----------------------------------------------------------------------------
void vtkEMSegmentLogic::TransferIJKToRAS(vtkMRMLVolumeNode* volumeNode, int ijk[3], double ras[3])
{
  vtkMatrix4x4* matrix = vtkMatrix4x4::New();
  volumeNode->GetIJKToRASMatrix(matrix);
  float input[4] = {ijk[0],ijk[1],ijk[2],1};
  float output[4];
  matrix->MultiplyPoint(input, output);
  ras[0]= output[0];
  ras[1]= output[1];
  ras[2]= output[2];
}

//----------------------------------------------------------------------------
void vtkEMSegmentLogic::TransferRASToIJK(vtkMRMLVolumeNode* volumeNode, double ras[3], int ijk[3])
{
  vtkMatrix4x4* matrix = vtkMatrix4x4::New();
  volumeNode->GetRASToIJKMatrix(matrix);
  double input[4] = {ras[0],ras[1],ras[2],1};
  double output[4];
  matrix->MultiplyPoint(input, output);
  ijk[0]= int(output[0]);
  ijk[1]= int(output[1]);
  ijk[2]= int(output[2]);
}

//----------------------------------------------------------------------------
// works for running stuff in TCL so that you do not need to look in two windows 
void vtkEMSegmentLogic::PrintText(char *TEXT) {
  cout << TEXT << endl;
} 

//----------------------------------------------------------------------------
void vtkEMSegmentLogic::PrintTextNoNewLine(char *TEXT) {
  cout << TEXT;
  cout.flush();
} 

//----------------------------------------------------------------------------
void  vtkEMSegmentLogic::AutoCorrectSpatialPriorWeight(vtkIdType nodeID)
{ 
   unsigned int numChildren = this->MRMLManager->GetTreeNodeNumberOfChildren(nodeID);
   for (unsigned int i = 0; i < numChildren; ++i)
    {
    vtkIdType childID = this->MRMLManager->GetTreeNodeChildNodeID(nodeID, i);
    bool isLeaf = this->MRMLManager->GetTreeNodeIsLeaf(childID);
    if (isLeaf)
      {
    if ((this->MRMLManager->GetTreeNodeSpatialPriorWeight(childID) > 0.0) && (!this->MRMLManager->GetAlignedSpatialPriorFromTreeNodeID(childID)))
      {
        vtkWarningMacro("Class with ID " <<  childID << " is set to 0 bc no atlas assigned to class!" );
        this->MRMLManager->SetTreeNodeSpatialPriorWeight(childID,0.0);
      }
      }
    else
      {
    this->AutoCorrectSpatialPriorWeight(childID);
      }
   }
}



//-----------------------------------------------------------------------------
void
vtkEMSegmentLogic::
CreatePackageFilenames(vtkMRMLScene* scene, 
                       const char* vtkNotUsed(packageDirectoryName))
{
  //
  // set up mrml manager for this new scene
  vtkEMSegmentMRMLManager* newSceneManager = vtkEMSegmentMRMLManager::New();
  newSceneManager->SetMRMLScene(scene);
  vtkMRMLEMSTemplateNode* newEMSTemplateNode = dynamic_cast<vtkMRMLEMSTemplateNode*>(scene->GetNthNodeByClass(0, "vtkMRMLEMSTemplateNode"));
  if (newEMSTemplateNode == NULL)
    {
      vtkWarningMacro("CreatePackageFilenames: no EMSSegmenter node!");
      newSceneManager->Delete();
      return;
    }
  if (newSceneManager->SetNodeWithCheck(newEMSTemplateNode))
    {
       vtkWarningMacro("CreatePackageFilenames: not a valid template node!");
       newSceneManager->Delete();
       return;
    }
   
  vtkMRMLEMSWorkingDataNode* workingDataNode = 
    newSceneManager->GetWorkingDataNode();

  //
  // We might be creating volume storage nodes.  We must decide if the
  // images should be automatically centered when they are read.  Look
  // at the original input target node zero to decide if we will use
  // centering.
  bool centerImages = false;
  if (workingDataNode && workingDataNode->GetInputTargetNode())
    {
    if (workingDataNode->GetInputTargetNode()->GetNumberOfVolumes() > 0)
      {
    if (!workingDataNode->GetInputTargetNode()->GetNthVolumeNode(0)) 
      {
              vtkErrorMacro("CreatePackageFilenames: the first InputTagetNode is not defined!");
              vtkIndent ind;
          workingDataNode->GetInputTargetNode()->PrintSelf(cerr,ind);
              cout << endl;
      } 
        else 
          {
            vtkMRMLStorageNode* firstTargetStorageNode = workingDataNode->GetInputTargetNode()->GetNthVolumeNode(0)->GetStorageNode();
            vtkMRMLVolumeArchetypeStorageNode* firstTargetVolumeStorageNode = dynamic_cast<vtkMRMLVolumeArchetypeStorageNode*> (firstTargetStorageNode);
            if (firstTargetVolumeStorageNode != NULL)
            { 
             centerImages = firstTargetVolumeStorageNode->GetCenterImage();
            }
      }
       }
    }

   // get the full path to the scene
  std::vector<std::string> scenePathComponents;
  vtkstd::string rootDir = newSceneManager->GetMRMLScene()->GetRootDirectory();
  if (rootDir.find_last_of("/") == rootDir.length() - 1)
    {
      vtkDebugMacro("em seg: found trailing slash in : " << rootDir);
      rootDir = rootDir.substr(0, rootDir.length()-1);
    }
  vtkDebugMacro("em seg scene manager root dir = " << rootDir);
  vtksys::SystemTools::SplitPath(rootDir.c_str(), scenePathComponents);

  // change the storage file for the segmentation result
    {
    vtkMRMLVolumeNode* volumeNode = newSceneManager->GetOutputVolumeNode();
    if (volumeNode != NULL)
      {
      vtkMRMLStorageNode* storageNode = volumeNode->GetStorageNode();
      vtkMRMLVolumeArchetypeStorageNode* volumeStorageNode = 
        dynamic_cast<vtkMRMLVolumeArchetypeStorageNode*>(storageNode);
      if (volumeStorageNode == NULL)
      {
      // create a new storage node for this volume
      volumeStorageNode = vtkMRMLVolumeArchetypeStorageNode::New();
      scene->AddNodeNoNotify(volumeStorageNode);
      volumeNode->SetAndObserveStorageNodeID(volumeStorageNode->GetID());
      std::cout << "Added storage node : " << volumeStorageNode->GetID() 
                << std::endl;
      volumeStorageNode->Delete();
      storageNode = volumeStorageNode;
      }
      volumeStorageNode->SetCenterImage(centerImages);
    
      // create new filename
      std::string oldFilename       = 
        (storageNode->GetFileName() ? storageNode->GetFileName() :
         "SegmentationResult.mhd");
      std::string oldFilenameNoPath = 
        vtksys::SystemTools::GetFilenameName(oldFilename);

      scenePathComponents.push_back("Segmentation");
      scenePathComponents.push_back(oldFilenameNoPath);

      std::string newFilename = 
        vtksys::SystemTools::JoinPath(scenePathComponents);
      storageNode->SetFileName(newFilename.c_str());
      scenePathComponents.pop_back();
      scenePathComponents.pop_back();

      }
    }

  //
  // change the storage file for the targets
  int numTargets = newSceneManager->GetTargetNumberOfSelectedVolumes();

  // input target volumes
  if (workingDataNode->GetInputTargetNode())
    {
    for (int i = 0; i < numTargets; ++i)
      {
      vtkMRMLVolumeNode* volumeNode =
        workingDataNode->GetInputTargetNode()->GetNthVolumeNode(i);
      if (volumeNode != NULL)
        {
        vtkMRMLStorageNode* storageNode = volumeNode->GetStorageNode();
        vtkMRMLVolumeArchetypeStorageNode* volumeStorageNode = 
          dynamic_cast<vtkMRMLVolumeArchetypeStorageNode*>(storageNode);
        if (volumeStorageNode == NULL)
          {
          // create a new storage node for this volume
          volumeStorageNode = vtkMRMLVolumeArchetypeStorageNode::New();
          scene->AddNodeNoNotify(volumeStorageNode);
          volumeNode->SetAndObserveStorageNodeID(volumeStorageNode->GetID());
          std::cout << "Added storage node : " << volumeStorageNode->GetID() 
                    << std::endl;
          volumeStorageNode->Delete();
          storageNode = volumeStorageNode;
          }
        volumeStorageNode->SetCenterImage(centerImages);
      
        // create new filename
        vtkstd::stringstream defaultFilename;
        defaultFilename << "Target" << i << "_Input.mhd";
        std::string oldFilename       = 
          (storageNode->GetFileName() ? storageNode->GetFileName() :
           defaultFilename.str().c_str());
        std::string oldFilenameNoPath = 
          vtksys::SystemTools::GetFilenameName(oldFilename);
        scenePathComponents.push_back("Target");
        scenePathComponents.push_back("Input");
        scenePathComponents.push_back(oldFilenameNoPath);
        std::string newFilename = 
          vtksys::SystemTools::JoinPath(scenePathComponents);
        
        storageNode->SetFileName(newFilename.c_str());
        scenePathComponents.pop_back();
        scenePathComponents.pop_back();
        scenePathComponents.pop_back();
        }
      }  
    }

  // aligned target volumes
  if (workingDataNode->GetAlignedTargetNode())
    {
    for (int i = 0; i < numTargets; ++i)
      {
      vtkMRMLVolumeNode* volumeNode =
        workingDataNode->GetAlignedTargetNode()->GetNthVolumeNode(i);
      if (volumeNode != NULL)
        {
        vtkMRMLStorageNode* storageNode = volumeNode->GetStorageNode();
        vtkMRMLVolumeArchetypeStorageNode* volumeStorageNode = 
          dynamic_cast<vtkMRMLVolumeArchetypeStorageNode*>(storageNode);
        if (volumeStorageNode == NULL)
          {
          // create a new storage node for this volume
          volumeStorageNode = vtkMRMLVolumeArchetypeStorageNode::New();
          scene->AddNodeNoNotify(volumeStorageNode);
          volumeNode->SetAndObserveStorageNodeID(volumeStorageNode->GetID());
          std::cout << "Added storage node : " << volumeStorageNode->GetID() 
                    << std::endl;
          volumeStorageNode->Delete();
          storageNode = volumeStorageNode;
          }
        volumeStorageNode->SetCenterImage(centerImages);
          
        // create new filename
        vtkstd::stringstream defaultFilename;
        defaultFilename << "Target" << i << "_Aligned.mhd";
        std::string oldFilename       = 
          (storageNode->GetFileName() ? storageNode->GetFileName() :
           defaultFilename.str().c_str());
        std::string oldFilenameNoPath = 
          vtksys::SystemTools::GetFilenameName(oldFilename);
        scenePathComponents.push_back("Target");
        scenePathComponents.push_back("Aligned");
        scenePathComponents.push_back(oldFilenameNoPath);
        std::string newFilename = 
          vtksys::SystemTools::JoinPath(scenePathComponents);
        
        storageNode->SetFileName(newFilename.c_str());
    scenePathComponents.pop_back();
    scenePathComponents.pop_back();
    scenePathComponents.pop_back();
        }
      }  
    }

  //
  // change the storage file for the atlas
  int numAtlasVolumes = newSceneManager->GetAtlasInputNode()->
    GetNumberOfVolumes();

  // input atlas volumes
  if (newSceneManager->GetAtlasInputNode())
    {
    for (int i = 0; i < numAtlasVolumes; ++i)
      {
      vtkMRMLVolumeNode* volumeNode =
         newSceneManager->GetAtlasInputNode()->GetNthVolumeNode(i);
      if (volumeNode != NULL)
        {
        vtkMRMLStorageNode* storageNode = volumeNode->GetStorageNode();
        vtkMRMLVolumeArchetypeStorageNode* volumeStorageNode = 
          dynamic_cast<vtkMRMLVolumeArchetypeStorageNode*>(storageNode);
        if (volumeStorageNode == NULL)
          {
          // create a new storage node for this volume
          volumeStorageNode = vtkMRMLVolumeArchetypeStorageNode::New();
          scene->AddNodeNoNotify(volumeStorageNode);
          volumeNode->SetAndObserveStorageNodeID(volumeStorageNode->GetID());
          std::cout << "Added storage node : " << volumeStorageNode->GetID() 
                    << std::endl;
          volumeStorageNode->Delete();
          storageNode = volumeStorageNode;
          }
        volumeStorageNode->SetCenterImage(centerImages);
      
        // create new filename
        vtkstd::stringstream defaultFilename;
        defaultFilename << "Atlas" << i << "_Input.mhd";
        std::string oldFilename       = 
          (storageNode->GetFileName() ? storageNode->GetFileName() :
           defaultFilename.str().c_str());
        std::string oldFilenameNoPath = 
          vtksys::SystemTools::GetFilenameName(oldFilename);
        scenePathComponents.push_back("Atlas");
        scenePathComponents.push_back("Input");
        scenePathComponents.push_back(oldFilenameNoPath);
        std::string newFilename = 
          vtksys::SystemTools::JoinPath(scenePathComponents);
        
        storageNode->SetFileName(newFilename.c_str());
    scenePathComponents.pop_back();
    scenePathComponents.pop_back();
    scenePathComponents.pop_back();
        }
      }  
    }

  // aligned atlas volumes
  if (workingDataNode->GetAlignedAtlasNode())
    {
    for (int i = 0; i < numAtlasVolumes; ++i)
      {
      vtkMRMLVolumeNode* volumeNode =
        workingDataNode->GetAlignedAtlasNode()->GetNthVolumeNode(i);
      if (volumeNode != NULL)
        {
        vtkMRMLStorageNode* storageNode = volumeNode->GetStorageNode();
        vtkMRMLVolumeArchetypeStorageNode* volumeStorageNode = 
          dynamic_cast<vtkMRMLVolumeArchetypeStorageNode*>(storageNode);
        if (volumeStorageNode == NULL)
          {
          // create a new storage node for this volume
          volumeStorageNode = vtkMRMLVolumeArchetypeStorageNode::New();
          scene->AddNodeNoNotify(volumeStorageNode);
          volumeNode->SetAndObserveStorageNodeID(volumeStorageNode->GetID());
          std::cout << "Added storage node : " << volumeStorageNode->GetID() 
                    << std::endl;
          volumeStorageNode->Delete();
          storageNode = volumeStorageNode;
          }
        volumeStorageNode->SetCenterImage(centerImages);
        
        // create new filename
        vtkstd::stringstream defaultFilename;
        defaultFilename << "Atlas" << i << "_Aligned.mhd";
        std::string oldFilename       = 
          (storageNode->GetFileName() ? storageNode->GetFileName() :
           defaultFilename.str().c_str());
        std::string oldFilenameNoPath = 
          vtksys::SystemTools::GetFilenameName(oldFilename);
        scenePathComponents.push_back("Atlas");
        scenePathComponents.push_back("Aligned");
        scenePathComponents.push_back(oldFilenameNoPath);
        std::string newFilename = 
          vtksys::SystemTools::JoinPath(scenePathComponents);
        
        storageNode->SetFileName(newFilename.c_str());
    scenePathComponents.pop_back();
    scenePathComponents.pop_back();
    scenePathComponents.pop_back();
        }
      }  
    }

  // clean up
  newSceneManager->Delete();
}

//-----------------------------------------------------------------------------
bool
vtkEMSegmentLogic::
CreatePackageDirectories(const char* packageDirectoryName)
{
  vtkstd::string packageDirectory(packageDirectoryName);
  
  // check that parent directory exists
  std::string parentDirectory = 
    vtksys::SystemTools::GetParentDirectory(packageDirectory.c_str());
  if (!vtksys::SystemTools::FileExists(parentDirectory.c_str()))
    {
    vtkWarningMacro
      ("CreatePackageDirectories: Parent directory does not exist!");
    return false;
    }
  
  // create package directories
  bool createdOK = true;
  std::string newDir = packageDirectory + "/Atlas/Input";
  createdOK = createdOK &&
    vtksys::SystemTools::MakeDirectory(newDir.c_str());  
  newDir = packageDirectory + "/Atlas/Aligned";
  createdOK = createdOK &&
    vtksys::SystemTools::MakeDirectory(newDir.c_str());  
  newDir = packageDirectory + "/Target/Input";
  createdOK = createdOK &&
    vtksys::SystemTools::MakeDirectory(newDir.c_str());  
  newDir = packageDirectory + "/Target/Normalized";
  createdOK = createdOK &&
    vtksys::SystemTools::MakeDirectory(newDir.c_str());  
  newDir = packageDirectory + "/Target/Aligned";
  createdOK = createdOK &&
    vtksys::SystemTools::MakeDirectory(newDir.c_str());  
  newDir = packageDirectory + "/Segmentation";
  createdOK = createdOK &&
    vtksys::SystemTools::MakeDirectory(newDir.c_str());  

  if (!createdOK)
    {
    vtkWarningMacro("CreatePackageDirectories: Could not create directories!");
    return false;
    }

  return true;
}

//-----------------------------------------------------------------------------
bool
vtkEMSegmentLogic::
WritePackagedScene(vtkMRMLScene* scene)
{
  //
  // write the volumes
  scene->InitTraversal();
  vtkMRMLNode* currentNode;
  bool allOK = true;
  while ((currentNode = scene->GetNextNodeByClass("vtkMRMLVolumeNode")) &&
         (currentNode != NULL))
    {
    vtkMRMLVolumeNode* volumeNode = 
      dynamic_cast<vtkMRMLVolumeNode*>(currentNode);

    if (volumeNode == NULL)
      {
      vtkWarningMacro("Volume node is null for node: " 
                    << currentNode->GetID());
      scene->RemoveNode(currentNode);
      allOK = false;
      continue;
      }
    if (volumeNode->GetImageData() == NULL)
      {
    vtkWarningMacro("Volume data is null for volume node: " << currentNode->GetID() << " Name : " <<  (currentNode->GetName() ? currentNode->GetName(): "(none)" ));
      scene->RemoveNode(currentNode);
      allOK = false;
      continue;
      }
    if (volumeNode->GetStorageNode() == NULL)
      {
      vtkWarningMacro("Volume storage node is null for volume node: " 
                    << currentNode->GetID());
      scene->RemoveNode(currentNode);
      allOK = false;
      continue;
      }

    try
      {
      std::cout << "Writing volume: " << volumeNode->GetName() 
                << ": " << volumeNode->GetStorageNode()->GetFileName() << "...";
      volumeNode->GetStorageNode()->SetUseCompression(0);
      volumeNode->GetStorageNode()->WriteData(volumeNode);
      std::cout << "DONE" << std::endl;
      }
    catch (...)
      {
      vtkErrorMacro("Problem writing volume: " << volumeNode->GetID());
      allOK = false;
      }
    }
  
  //
  // write the MRML scene file
  try 
    {
    scene->Commit();
    }
  catch (...)
    {
    vtkErrorMacro("Problem writing scene.");
    allOK = false;
    }  

  return allOK;
}

//-----------------------------------------------------------------------------
void vtkEMSegmentLogic::SubParcelateSegmentation(vtkImageData* segmentation, vtkIdType nodeID)
{
  unsigned int numChildren =  this->MRMLManager->GetTreeNodeNumberOfChildren(nodeID);
  for (unsigned int i = 0; i < numChildren; ++i)
    {
    vtkIdType childID = this->MRMLManager->GetTreeNodeChildNodeID(nodeID, i);
    if (this->MRMLManager->GetTreeNodeIsLeaf(childID))
      {
    vtkMRMLVolumeNode*  parcellationNode =  this->MRMLManager->GetAlignedSubParcellationFromTreeNodeID(childID);
        if ( ! parcellationNode || !parcellationNode->GetImageData() ) 
      {
            continue;
      }
        int childLabel =  this->MRMLManager->GetTreeNodeIntensityLabel(childID);
        cout << "==> Subparcellate " <<  childLabel << endl;
        vtkImageData* input = vtkImageData::New();
    input->DeepCopy(segmentation);
 
    vtkImageThreshold* roiMap =     vtkImageThreshold::New();
        roiMap->SetInput(input);
        roiMap->ThresholdBetween( childLabel,  childLabel);
        roiMap->ReplaceOutOn();
        roiMap->SetInValue(1);
        roiMap->SetOutValue(0);
        roiMap->Update();
 
        vtkImageCast* castParcellation = vtkImageCast::New();
        castParcellation->SetInput(parcellationNode->GetImageData());
    castParcellation->SetOutputScalarType(roiMap->GetOutput()->GetScalarType());
    castParcellation->Update();

        vtkImageMathematics* roiParcellation = vtkImageMathematics::New();
    roiParcellation->SetInput1(roiMap->GetOutput());
    roiParcellation->SetInput2(castParcellation->GetOutput());
        roiParcellation->SetOperationToMultiply();
        roiParcellation->Update();

    vtkImageThreshold* changedSeg =     vtkImageThreshold::New();
        changedSeg->SetInput(input);
        changedSeg->ThresholdBetween( childLabel,  childLabel);
        changedSeg->ReplaceOutOff();
        changedSeg->SetInValue(0);
        changedSeg->Update();

        vtkImageMathematics* parcellatedSeg = vtkImageMathematics::New();
    parcellatedSeg->SetInput1(changedSeg->GetOutput());
    parcellatedSeg->SetInput2(roiParcellation->GetOutput());
        parcellatedSeg->SetOperationToAdd();
        parcellatedSeg->Update();

        segmentation->DeepCopy(parcellatedSeg->GetOutput());
        parcellatedSeg->Delete();
        changedSeg->Delete();
        roiParcellation->Delete();
        castParcellation->Delete();
        roiMap->Delete();
    input->Delete();
      }
    else
      {
        this->SubParcelateSegmentation(segmentation, childID); 
      }

    }
}

//-----------------------------------------------------------------------------
void vtkEMSegmentLogic::AddDefaultTasksToList(const char* FilePath, std::vector<std::string> & DefaultTasksName,  std::vector<std::string> & DefaultTasksFile, 
                                                                                                                       std::vector<std::string> & DefinePreprocessingTasksName, std::vector<std::string> & DefinePreprocessingTasksFile)
{
 vtkDirectory *dir = vtkDirectory::New();
  // Do not give out an error message here bc it otherwise comes up when loading slicer 
  // the path might simply not be created !
  
  if (!dir->Open(FilePath))
    {
      dir->Delete();
      return;
    }
    
  int numberOfFiles = dir->GetNumberOfFiles();
    
  for (int i = 0; i < numberOfFiles; i++)
    {
    
    vtksys_stl::string filename = dir->GetFile(i);
    
    // do nothing if file is ".", ".." 
    if (strcmp(filename.c_str(), ".") && strcmp(filename.c_str(), ".."))
      {

      //  {
      //  continue;
      //  }
   
      vtksys_stl::string tmpFullFileName = vtksys_stl::string(FilePath) + vtksys_stl::string("/") + filename.c_str();
      vtksys_stl::string fullFileName = vtksys::SystemTools::ConvertToOutputPath(tmpFullFileName.c_str());
      
      // if it has a .mrml extension but is a directory, do nothing
      if (!vtksys::SystemTools::FileIsDirectory(fullFileName.c_str()))
        {
     
        if (!strcmp(vtksys::SystemTools::GetFilenameExtension(filename.c_str()).c_str(), ".mrml") && (filename.compare(0,1,"_") ) )
          {
              // Generate Name of Task from File name
              vtksys_stl::string taskName = this->MRMLManager->TurnDefaultMRMLFileIntoTaskName(filename.c_str());
              // make sure that file is not already in the list
              // we loop through the list and set existFlag to 1 if it exists already 
              int existFlag = 0;
              // we need a new index for this inner loop *grrrrr took me long to find this one
              for (int j=0; j < int(DefaultTasksName.size()); j++)
              {
                 if (!DefaultTasksName[j].compare(taskName))
                 { 
                   existFlag =1;
                 }
              }
              if (!existFlag)
               {
                 // Add to List if it does not exist
                 DefaultTasksFile.push_back(fullFileName);
                 DefaultTasksName.push_back(taskName);
           }
        }
      else if ((!strcmp(vtksys::SystemTools::GetFilenameExtension(filename.c_str()).c_str(), ".tcl")) && (filename.compare(0,1,"_") ) 
                    && strcmp(filename.c_str(), vtkMRMLEMSGlobalParametersNode::GetDefaultTaskTclFileName()))
        {
              // Generate Name of Task from File name
              vtksys_stl::string taskName = this->MRMLManager->TurnDefaultTclFileIntoPreprocessingName(filename.c_str());
              // make sure that file is not already in the list
              // we loop through the list and set existFlag to 1 if it exists already 
              int existFlag = 0;
              // we need a new index for this inner loop *grrrrr took me long to find this one
              for (int j=0; j < int(DefinePreprocessingTasksName.size()); j++)
              {
                 if (!DefinePreprocessingTasksName[j].compare(taskName))
                 { 
                   existFlag =1;
                 }
              }
              if (!existFlag)
               {
                  // Add to List if it does not exist
                  DefinePreprocessingTasksFile.push_back(fullFileName);
                  DefinePreprocessingTasksName.push_back(taskName);
               }
           }   
        } // check if it is not a directory
      } // check if the file is .,.. or does not have a .mrml extension
    } // loop through all the files
    
  dir->Delete();
}

//----------------------------------------------------------------------------
void vtkEMSegmentLogic::CreateOutputVolumeNode()
{

  // Version 1 - It is a little bit slower bc it creates image data that we do not need
  // (vtkSlicerApplication* app) 
  // vtkSlicerVolumesGUI *vgui = vtkSlicerVolumesGUI::SafeDownCast (app->GetModuleGUIByName ( "Volumes"));
  // if (!vgui)  
  // {
  //   vtkErrorMacro("CreateOutputVolumeNode: could not find vtkSlicerVolumesGUI "); 
  //   return;
  // }
  // vtkSlicerVolumesLogic* volLogic  = vgui->GetLogic();
  // if (!volLogic)  
  // {
  //   vtkErrorMacro("CreateOutputVolumeNode: could not find vtkSlicerVolumesLogic "); 
  //   return;
  // }
  //
  // vtkMRMLNode* snode = this->GetMRMLScene()->GetNodeByID(this->MRMLManager->GetTargetSelectedVolumeNthMRMLID(0));
  // vtkMRMLVolumeNode* vNode = vtkMRMLVolumeNode::SafeDownCast(snode);
  //
  // if (vNode == NULL)
  // {
  //   vtkErrorMacro("Invalid volume MRMLID: " << this->MRMLManager->GetTargetSelectedVolumeNthMRMLID(0));
  //   return;
  // }
  //  vtkMRMLScalarVolumeNode* outputNode = volLogic->CreateLabelVolume (this->GetMRMLScene(), vNode, "EM_MAP");

  // My version
  vtkMRMLScalarVolumeNode* outputNode = vtkMRMLScalarVolumeNode::New();
  outputNode->SetLabelMap(1);
   std::string uname =  this->MRMLScene->GetUniqueNameByString("EM_Map");
  outputNode->SetName(uname.c_str());
  this->GetMRMLScene()->AddNode(outputNode);

  vtkMRMLLabelMapVolumeDisplayNode* displayNode = vtkMRMLLabelMapVolumeDisplayNode::New();
  displayNode->SetScene(this->GetMRMLScene());
  this->GetMRMLScene()->AddNode(displayNode);
  displayNode->SetAndObserveColorNodeID(this->MRMLManager->GetColorNodeID());
  outputNode->SetAndObserveDisplayNodeID(displayNode->GetID());

  this->MRMLManager->SetOutputVolumeMRMLID(outputNode->GetID());
}

