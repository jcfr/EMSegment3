
#ifndef __vtkEMSegmentWin32Header_h
#define __vtkEMSegmentWin32Header_h

#if !defined(Slicer3_USE_QT)

#include <vtkEMSegmentConfigure.h>

#if defined(_WIN32) && !defined(VTKSLICER_STATIC)
#if defined(EMSegment_EXPORTS)
#define VTK_EMSEGMENT_EXPORT __declspec( dllexport ) 
#else
#define VTK_EMSEGMENT_EXPORT __declspec( dllimport ) 
#endif
#else
#define VTK_EMSEGMENT_EXPORT 
#endif
#endif

#endif // only do this stuff if this is not Slicer4
