#include "EMSegmentTclInterpreterCLP.h"
#include "EMSegmentAPIHelper.h"


// -============================
// MAIN
// -============================

int main(int argc, char** argv)
{
  if (argc < 2) {
    cout << "Error: No Input Defined "<< endl;
    return EXIT_FAILURE;
  }

  // =======================================================================
  //  Initialize TCL
  // =======================================================================

  // interp has to be set to initialize vtkSlicer
  Tcl_Interp *interp = CreateTclInterp(argc,argv);
  if (!interp)
    {
      return EXIT_FAILURE;
    }

  vtkSlicerApplication* app = vtkSlicerApplication::GetInstance();
  vtkSlicerApplicationLogic* appLogic = InitializeApplication(interp,app,argc,argv);
  if (!appLogic)
    {
      CleanUp(app,appLogic);
      return EXIT_FAILURE;
    }

  // =======================================================================
  //  Souce TCL File
  // =======================================================================
  int exitFlag = EXIT_SUCCESS;

  try
    {
      std::string CMD = std::string("source ") + argv[1] ;
      app->Script(CMD.c_str());
    }
  catch (...)
    {
      exitFlag = EXIT_FAILURE;
    }

  // =======================================================================
  //  Clean up
  // =======================================================================
  CleanUp(app,appLogic);
  return exitFlag;
}
