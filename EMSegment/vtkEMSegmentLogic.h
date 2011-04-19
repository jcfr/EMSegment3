#ifndef __vtkEMSegmentLogic_h
#define __vtkEMSegmentLogic_h

#include "vtkSlicerModuleLogic.h"
#include "vtkEMSegment.h"
#include "vtkEMSegmentMRMLManager.h"

class vtkImageEMLocalSegmenter;
class vtkImageEMLocalGenericClass;
class vtkImageEMLocalSuperClass;
class vtkImageEMLocalClass;
class vtkSlicerApplicationLogic;
class vtkGridTransform;

class VTK_EMSEGMENT_EXPORT vtkEMSegmentLogic : public vtkSlicerModuleLogic
{
public:
  static vtkEMSegmentLogic *New();
  vtkTypeMacro(vtkEMSegmentLogic,vtkSlicerModuleLogic);

  // Description: The name of the Module---this is used to construct
  // the proc invocations
  vtkGetStringMacro (ModuleName);
  vtkSetStringMacro (ModuleName);

  //
  // actions
  //
  virtual bool      StartPreprocessingInitializeInputData();
  //BTX
  vtkstd::string GetTclGeneralDirectory();
  //ETX
  
  // Used within StartSegmentation to copy data from the MRMLManager
  // to the segmenter algorithm.  Possibly useful for research
  // purposes.
  virtual void      CopyDataToSegmenter(vtkImageEMLocalSegmenter* segmenter);

  //
  // progress bar related functions: not currently used, likely to
  // change
  vtkGetStringMacro(ProgressCurrentAction);
  vtkGetMacro(ProgressGlobalFractionCompleted, double);
  vtkGetMacro(ProgressCurrentFractionCompleted, double);

  //
  // MRML Related Methods.  The collection of MRML nodes for the
  // EMSegmenter is complicated.  Therefore, the management of these
  // nodes are delegated to the vtkEMSegmentMRMLManager class.
  vtkGetObjectMacro(MRMLManager, vtkEMSegmentMRMLManager);

  //
  // Register all the nodes used by this module with the current MRML
  // scene.
  virtual void RegisterMRMLNodesWithScene()
      { 
      this->MRMLManager->RegisterMRMLNodesWithScene(); 
      }

  virtual void SetAndObserveMRMLScene(vtkMRMLScene* scene)
      {
      Superclass::SetAndObserveMRMLScene(scene);
      this->MRMLManager->SetMRMLScene(scene);
      }

  virtual void ProcessMRMLEvents ( vtkObject *caller, unsigned long event,
                                   void *callData )
      { 
      this->MRMLManager->ProcessMRMLEvents(caller, event, callData); 
      }

  // events to observe
  virtual vtkIntArray* NewObservableEvents();

  void StartPreprocessingResampleAndCastToTarget(vtkMRMLVolumeNode* movingVolumeNode, vtkMRMLVolumeNode* fixedVolumeNode, vtkMRMLVolumeNode* outputVolumeNode);

  static void TransferIJKToRAS(vtkMRMLVolumeNode* volumeNode, int ijk[3], double ras[3]);
  static void TransferRASToIJK(vtkMRMLVolumeNode* volumeNode, double ras[3], int ijk[3]);


  static double GuessRegistrationBackgroundLevel(vtkMRMLVolumeNode* volumeNode);

  static void 
  SlicerImageResliceWithGrid(vtkMRMLVolumeNode* inputVolumeNode,
                             vtkMRMLVolumeNode* outputVolumeNode,
                             vtkMRMLVolumeNode* outputVolumeGeometryNode,
                             vtkGridTransform* outputRASToInputRASTransform,
                             int iterpolationType,
                             double backgroundLevel);


  // utility---should probably go to general slicer lib at some point
  static void SlicerImageReslice(vtkMRMLVolumeNode* inputVolumeNode,
                                 vtkMRMLVolumeNode* outputVolumeNode,
                                 vtkMRMLVolumeNode* outputVolumeGeometryNode,
                                 vtkTransform* outputRASToInputRASTransform,
                                  int iterpolationType,
                                 double backgroundLevel);

  // Helper Classes for tcl 
  void PrintTextNoNewLine(char *TEXT);
  void PrintText(char *TEXT);

  void DefineValidSegmentationBoundary(); 
  void AutoCorrectSpatialPriorWeight(vtkIdType nodeID);

  vtkMRMLScalarVolumeNode* AddArchetypeScalarVolume (const char* filename, const char* volname, vtkSlicerApplicationLogic* appLogic,  vtkMRMLScene* mrmlScene);


  //BTX
  std::string GetErrorMessage() {return this->ErrorMsg;}
  //ETX 

  virtual void                              CreateOutputVolumeNode();

  void SubParcelateSegmentation(vtkImageData* segmentation, vtkIdType nodeID);

  // functions for packaging and writing intermediate results
  virtual void CreatePackageFilenames(vtkMRMLScene* scene, 
                                      const char* packageDirectoryName);
  virtual bool CreatePackageDirectories(const char* packageDirectoryName);
  virtual bool WritePackagedScene(vtkMRMLScene* scene);

//BTX
  void AddDefaultTasksToList(const char* FilePath, std::vector<std::string> & DefaultTasksName,  std::vector<std::string> & DefaultTasksFile, 
                 std::vector<std::string> & DefinePreprocessingTasksName, std::vector<std::string>  & DefinePreprocessingTasksFile);
//ETX

protected: 
  // the mrml manager is created in the constructor
  vtkSetObjectMacro(MRMLManager, vtkEMSegmentMRMLManager);

  //BTX
  template <class T>
  static T GuessRegistrationBackgroundLevel(vtkImageData* imageData);
  //ETX

  static void
  ComposeGridTransform(vtkGridTransform* inGrid,
                       vtkMatrix4x4*     preMultiply,
                       vtkMatrix4x4*     postMultiply,
                       vtkGridTransform* outGrid);

  // Description:
  // Convenience method for determining if two volumes have same geometry
  static bool IsVolumeGeometryEqual(vtkMRMLVolumeNode* lhs,
                                    vtkMRMLVolumeNode* rhs);

  static void PrintImageInfo(vtkMRMLVolumeNode* volumeNode);
  static void PrintImageInfo(vtkImageData* image);

  // copy data from MRML to algorithm
  virtual void CopyAtlasDataToSegmenter(vtkImageEMLocalSegmenter* segmenter);
  virtual void CopyTargetDataToSegmenter(vtkImageEMLocalSegmenter* segmenter);
  virtual void CopyGlobalDataToSegmenter(vtkImageEMLocalSegmenter* segmenter);
  virtual void CopyTreeDataToSegmenter(vtkImageEMLocalSuperClass* node,
                                       vtkIdType nodeID);
  virtual void CopyTreeGenericDataToSegmenter(vtkImageEMLocalGenericClass* 
                                              node,
                                              vtkIdType nodeID);
  virtual void CopyTreeParentDataToSegmenter(vtkImageEMLocalSuperClass* node,
                                             vtkIdType nodeID);
  virtual void CopyTreeLeafDataToSegmenter(vtkImageEMLocalClass* node,
                                           vtkIdType nodeID);  

  //
  // convenience methods for translating enums between algorithm and
  // this module
  virtual int
    ConvertGUIEnumToAlgorithmEnumStoppingConditionType(int guiEnumValue);
  virtual int
    ConvertGUIEnumToAlgorithmEnumInterpolationType(int guiEnumValue);


  // not currently used
  vtkSetStringMacro(ProgressCurrentAction);
  vtkSetMacro(ProgressGlobalFractionCompleted, double);
  vtkSetMacro(ProgressCurrentFractionCompleted, double);


  //
  // because the mrml nodes are very complicated for this module, we
  // delegate the handeling of them to a MRML manager
  vtkEMSegmentMRMLManager* MRMLManager;

  char *ModuleName;

  //
  // information related to progress bars: this mechanism is not
  // currently implemented and might me best implemented elsewhere
  char*  ProgressCurrentAction;
  double ProgressGlobalFractionCompleted;
  double ProgressCurrentFractionCompleted;
  //BTX
  std::string ErrorMsg; 
  //ETX
  vtkEMSegmentLogic();
  ~vtkEMSegmentLogic();

private:
  vtkEMSegmentLogic(const vtkEMSegmentLogic&);
  void operator=(const vtkEMSegmentLogic&);
};

#endif
