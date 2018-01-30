// This example illustrates how the vtkGdcmWriter vtk class can be
// used in order to:
//
// Usage:
// 
//----------------------------------------------------------------------------
#include <iostream>

#include <vtkImageMapToColors.h>
#include <vtkLookupTable.h>
#include <vtkImageData.h>

#include "vtkGdcmReader.h"
#include "vtkGdcmWriter.h"

#ifndef vtkFloatingPointType
#define vtkFloatingPointType float
#endif

//----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
   if( argc < 3 )
   {
      return 0;
   }
  
   vtkGdcmReader *reader = vtkGdcmReader::New();
   reader->AllowLookupTableOff();
   reader->SetFileName( argv[1] );
   reader->Update();

   vtkImageData *output;
   if( reader->GetLookupTable() )
   {
      //convert to color:
      vtkImageMapToColors *map = vtkImageMapToColors::New ();
      map->SetInput (reader->GetOutput());
      map->SetLookupTable (reader->GetLookupTable());
      map->SetOutputFormatToRGB();
      output = map->GetOutput();
      map->Delete();
   }
   else
   {
      output = reader->GetOutput();
   }
  
   //print debug info:
   output->Print(cout);

   //////////////////////////////////////////////////////////
   // WRITE...
   //if you wish you can export dicom to a vtk file 
   // this file will have the add of .tmp.dcm extention
   std::string fileName = argv[2];
   fileName += ".dcm";

   vtkGdcmWriter *writer = vtkGdcmWriter::New();

   // For 3D
   writer->SetFileDimensionality(3);
   writer->SetFileName(fileName.c_str());
   if(argc >= 4)
   {
      if( strcmp(argv[3],"2D" )==0 )
      {
         writer->SetFileDimensionality(2);
         writer->SetFilePrefix(argv[2]);
         writer->SetFilePattern("%s%d.dcm");
      }
   }

   writer->SetInput(output);
   writer->Write();
   //////////////////////////////////////////////////////////

   // Clean up
   writer->Delete();
   reader->Delete();

   return 0;
}
