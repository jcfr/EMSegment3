#ifndef __vtkEMSegmentKWLogic_h
#define __vtkEMSegmentKWLogic_h

#include  "vtkSlicerApplication.h" 
#include  "vtkEMSegmentLogic.h"

// lists all functions that include KW Applications , i.e. make tcl calls 
class VTK_EMSEGMENT_EXPORT vtkEMSegmentKWLogic : 
  public vtkObject
{
public:
  static vtkEMSegmentKWLogic *New();
  vtkTypeMacro(vtkEMSegmentKWLogic,  vtkObject);

  //
  // actions
  //
  virtual bool      SaveIntermediateResults(vtkSlicerApplicationLogic *appLogic);

  virtual int       SourceTclFile(const char *tclFile);
  virtual int       SourceTaskFiles();
  virtual int       SourcePreprocessingTclFiles(); 
  virtual int       StartSegmentationWithoutPreprocessing(vtkSlicerApplicationLogic *appLogic);
  int               ComputeIntensityDistributionsFromSpatialPrior();


  //BTX
  vtkstd::string GetTclTaskDirectory();
  vtkstd::string DefineTclTaskFileFromMRML();
  vtkstd::string DefineTclTaskFullPathName(const char* TclFileName);
  vtkstd::string GetTemporaryTaskDirectory();
  //ETX
  
  // copy all nodes relating to the EMSegmenter into newScene
  // and write to file 
  virtual bool PackageAndWriteData(vtkSlicerApplicationLogic *appLogic, const char* packageDirectoryName);

  int UpdateTasks();

//BTX
  void CreateDefaultTasksList(std::vector<std::string> & DefaultTasksName,  std::vector<std::string> & DefaultTasksFile, 
                  std::vector<std::string> & DefinePreprocessingTasksName, std::vector<std::string> & DefinePreprocessingTasksFile);
//ETX

  void UpdateIntensityDistributionAuto(vtkIdType nodeID);

  vtkSetObjectMacro(SlicerApp, vtkSlicerApplication);
  vtkSetObjectMacro(EMSLogic,  vtkEMSegmentLogic);
  vtkGetObjectMacro(EMSLogic,  vtkEMSegmentLogic);

 //BTX
  std::string GetErrorMessage() {return this->ErrorMsg;}
  //ETX 

private:
  vtkEMSegmentKWLogic();
  ~vtkEMSegmentKWLogic();
  vtkEMSegmentKWLogic(const vtkEMSegmentKWLogic&);
  void operator=(const vtkEMSegmentKWLogic&);

  vtkSlicerApplication* SlicerApp;
  vtkEMSegmentLogic *EMSLogic;
  //BTX
  std::string ErrorMsg; 
  //ETX
};

#endif
