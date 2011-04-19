#include "vtkSlicerApplication.h"
#include "vtkKWTkUtilities.h"

extern "C" int Slicerbasegui_Init(Tcl_Interp *interp);
extern "C" int Emsegment_Init(Tcl_Interp *interp);
extern "C" int Vtkteem_Init(Tcl_Interp *interp);
extern "C" int Vtkitk_Init(Tcl_Interp *interp);
extern "C" int Slicerbaselogic_Init(Tcl_Interp *interp);
extern "C" int Mrml_Init(Tcl_Interp *interp);
extern "C" int Mrmlcli_Init(Tcl_Interp *interp); 
extern "C" int Commandlinemodule_Init(Tcl_Interp *interp);
extern "C" int Kwwidgets_Init(Tcl_Interp *interp);

#define tgVtkCreateMacro(name,type)                                     \
  name  = type::New();                                                  \
  name##Tcl = vtksys::SystemTools::DuplicateString(vtkKWTkUtilities::GetTclNameFromPointer(interp, name)); 

#define tgVtkDefineMacro(name,type)             \
  type *name;                                   \
  std::string name##Tcl;                        \
  tgVtkCreateMacro(name,type); 

#define tgSetDataMacro(name,matrix)                             \
  virtual int Set##name(const char *fileName) {                 \
    if (strcmp(fileName,"None")) {                              \
      tgVtkCreateMacro(this->name,vtkImageData);                \
      this->matrix = vtkMatrix4x4::New();                       \
      return tgReadVolume(fileName,this->name,this->matrix);    \
    }                                                           \
    this->name = NULL;                                          \
    this->matrix = NULL;                                        \
    std::cout << "Here" << std::endl;                           \
    return 0;                                                   \
  }


vtksys_stl::string tgGetSLICER_HOME(char** argv)  
{ 
  vtksys_stl::string slicerHome = "";
  if ( !vtksys::SystemTools::GetEnv("Slicer3_HOME", slicerHome) )
    {
      std::string programPath;
      std::string errorMessage;
      if ( !vtksys::SystemTools::FindProgramPath(argv[0], programPath, errorMessage) ) return slicerHome;

      slicerHome = vtksys::SystemTools::GetFilenamePath(programPath.c_str()) + "/../../../";
    } 
  return slicerHome;
}

int tgSetSLICER_HOME(char** argv)  
{ 
  vtksys_stl::string slicerHome = "";
  if ( !vtksys::SystemTools::GetEnv("Slicer3_HOME", slicerHome) )
    {
      std::string programPath;
      std::string errorMessage;

      if ( !vtksys::SystemTools::FindProgramPath(argv[0], programPath, errorMessage) ) return 1;

      std::string homeEnv = "Slicer3_HOME=";
      homeEnv += vtksys::SystemTools::GetFilenamePath(programPath.c_str()) + "/../../../";
   
      cout << "Set environment: " << homeEnv.c_str() << endl;
      vtkKWApplication::PutEnv(const_cast <char *> (homeEnv.c_str()));
    } else {
    cout << "Slicer3_HOME found: " << slicerHome << endl;
  }
  return 0;
}

Tcl_Interp* CreateTclInterp(int argc, char** argv) 
{
  Tcl_Interp *interp = vtkKWApplication::InitializeTcl(argc, argv, &cout);
  if (!interp)
    {
      cout << "Error: InitializeTcl failed" << endl;
      return NULL; 
    }

  // This is necessary to load in EMSEgmenter package in TCL interp.
  Emsegment_Init(interp);
  Slicerbasegui_Init(interp);
  Slicerbaselogic_Init(interp);
  Mrml_Init(interp);
  Mrmlcli_Init(interp); 
  Vtkteem_Init(interp);
  Vtkitk_Init(interp);
  Commandlinemodule_Init(interp);
  Kwwidgets_Init(interp);
  // Atlascreatorcxxmodule_Init(interp);
  return interp;
}

#ifdef _WIN32
std::string StripBackslashes(const std::string& s)
{
  std::string outString;
  for (unsigned int i = 0; i < s.size(); ++i)
    {
      if (s[i] != '\\')
        {
          outString.push_back(s[i]);
        }
      else if (i > 0 && s[i-1] == '\\')
        {
          outString.push_back(s[i]);
        }
    }
  return outString;
}
#endif

vtkSlicerApplicationLogic* InitializeApplication(Tcl_Interp *interp, vtkSlicerApplication *app, int argc, char** argv) 
{
  // SLICER_HOME
  cout << "Setting SLICER home: " << endl;
  vtkstd::string slicerHome = tgGetSLICER_HOME(argv);
  if(!slicerHome.size())
    {
      cout << "Error: Cannot find executable" << endl;
      return NULL;
    }
  cout << "Slicer home is " << slicerHome << endl;

  app->PromptBeforeExitOff();
  std::string appTcl;
  appTcl = vtksys::SystemTools::DuplicateString(vtkKWTkUtilities::GetTclNameFromPointer(interp, app));;

  app->Script ("namespace eval slicer3 set Application %s", appTcl.c_str());

  tgVtkDefineMacro(appLogic,vtkSlicerApplicationLogic);

  app->Script ("namespace eval slicer3 set ApplicationLogic %s", appLogicTcl.c_str());

  // set BinDir to make functionality like GetSvnRevision available
  std::string slicerBinDir = slicerHome + "/bin";
  slicerBinDir = vtksys::SystemTools::CollapseFullPath(slicerBinDir.c_str());
  app->SetBinDir(slicerBinDir.c_str());

  // Make generic later 
  app->Script("set ::env(KILIS_MODULE) KilisSandbox");
  std::string CMD = std::string("set ::env(SLICER_HOME) ") + slicerHome + "/..";
  app->Script(CMD.c_str());

  CMD = "set argv { "; 
  for (int i = 2 ; i < argc ; i++) CMD += std::string(argv[i]) + " ";
  CMD += " }  "; 
  app->Script(CMD.c_str());
  return  appLogic;
}


void CleanUp(vtkSlicerApplication* app, vtkSlicerApplicationLogic* appLogic  )
{
  if (appLogic)
    {
      appLogic->Delete();
      appLogic = NULL;  
    }

  if (app)
    {
      app->Exit();
      app->Delete();
      app = NULL;
    }
}
