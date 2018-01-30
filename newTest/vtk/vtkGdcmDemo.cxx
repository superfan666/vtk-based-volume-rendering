// $Header: /cvs/public/gdcm/vtk/vtkGdcmDemo.cxx,v 1.2 2004/11/09 11:21:33 regrain Exp $

//----------------------------------------------------------------------------
// A simple straightfoward example of vtkGdcmReader vtk class usage.
//
// The vtkGdcmReader vtk class behaves like any other derived class of
// vtkImageReader. It's usage within a vtk pipeline hence follows the
// classical vtk pattern.
// This example is a really simple Dicom image viewer demo.
// It builds the minimal vtk rendering pipeline in order to display
// (with the native vtk classes) a single Dicom image parsed with gdcm.
//
// Usage: the filename of the Dicom image to display should be given as
//        command line arguments,
//----------------------------------------------------------------------------

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkImageMapper.h>
#include <vtkImageData.h>
#include <vtkImageViewer.h>
#include <vtkMatrix4x4.h>
#include <vtkLookupTable.h>
#include <vtkMatrixToLinearTransform.h>
#include <vtkTexture.h>
#include <vtkPlaneSource.h>
#include <vtkTextureMapToPlane.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkImageCast.h>
#include <vtkPNGWriter.h>
#include <vtkTexture.h>

#include "vtkGdcmReader.h"

  
int main( int argc, char *argv[] )
{
   vtkGdcmReader *reader = vtkGdcmReader::New();

   if (argc < 2)
   {
      cerr << "Usage: " << argv[0] << " image.dcm\n";
      return 0;
   }

   reader->SetFileName( argv[1] );

   reader->UpdateWholeExtent();
   vtkImageData* ima = reader->GetOutput();

   ///////// Display image size on terminal:
   int* Size = ima->GetDimensions();
   cout << "Dimensions of the picture as read with gdcm: "
        << Size[0] << " x " << Size[1] << endl;

   ///////// A simple display pipeline:
   // 
   vtkLookupTable* VTKtable = vtkLookupTable::New();
   VTKtable->SetNumberOfColors(1000);
   VTKtable->SetTableRange(0,1000);
   VTKtable->SetSaturationRange(0,0);
   VTKtable->SetHueRange(0,1);
   VTKtable->SetValueRange(0,1);
   VTKtable->SetAlphaRange(1,1);
   VTKtable->Build();

   //// Texture
   vtkTexture* VTKtexture = vtkTexture::New();
   VTKtexture->SetInput(ima);
   VTKtexture->InterpolateOn();
   VTKtexture->SetLookupTable(VTKtable);

   //// PlaneSource
   vtkPlaneSource* VTKplane = vtkPlaneSource::New();
   VTKplane->SetOrigin( -0.5, -0.5, 0.0);
   VTKplane->SetPoint1(  0.5, -0.5, 0.0);
   VTKplane->SetPoint2( -0.5,  0.5, 0.0);

   //// PolyDataMapper
   vtkPolyDataMapper *VTKplaneMapper = vtkPolyDataMapper::New();
   VTKplaneMapper->SetInput(VTKplane->GetOutput());

   //// Actor
   vtkActor* VTKplaneActor = vtkActor::New();
   VTKplaneActor->SetTexture(VTKtexture);
   VTKplaneActor->SetMapper(VTKplaneMapper);
   VTKplaneActor->PickableOn();

   //// Final rendering with simple interactor:
   vtkRenderer        *ren = vtkRenderer::New();
   vtkRenderWindow *renwin = vtkRenderWindow::New();
   renwin->AddRenderer(ren);
   vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
   iren->SetRenderWindow(renwin);
   ren->AddActor(VTKplaneActor);
   ren->SetBackground(0,0,0.5);
   renwin->Render();
   iren->Start();

   //// Clean up:
   reader->Delete();
   VTKtable->Delete();
   VTKtexture->Delete();
   VTKplane->Delete();
   VTKplaneMapper->Delete();
   VTKplaneActor->Delete();
   ren->Delete();
   renwin->Delete();
   iren->Delete();

   return(0);
}
