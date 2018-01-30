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

#include "gdcmArgMgr.h" // for Argument Manager functions
#include "gdcmFile.h"

//----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
   START_USAGE(usage)
   " \n vtkWriteDicomExtended : \n",
   " Reads a DICOM file and re writes it according to user's requirements.    ",
   "                                                                          ",
   " usage: vtkWriteDicomExtended filein=dicom file to read                   ",
   "                           [filecontent = ] [2D]                          ",
   "                           [noshadowseq][noshadow][noseq]                 ",
   "                           [debug]                                        ",
   "      filecontent = 1 : USER_OWN_IMAGE                                    ",
   "                  = 2 : FILTERED_IMAGE                                    ",
   "                  = 3 : CREATED_IMAGE                                     ",
   "                  = 4 : UNMODIFIED_PIXELS_IMAGE                           ",
   "      noshadowseq: user doesn't want to load Private Sequences            ",
   "      noshadow   : user doesn't want to load Private groups (odd number)  ",
   "      noseq      : user doesn't want to load Sequences                    ",
   "      debug      : user wants to run the program in 'debug mode'          ",
   FINISH_USAGE
   
   
    // Initialize Arguments Manager   
   GDCM_NAME_SPACE::ArgMgr *am= new GDCM_NAME_SPACE::ArgMgr(argc, argv);
  
   if (argc == 1 || am->ArgMgrDefined("usage") )
   {
      am->ArgMgrUsage(usage); // Display 'usage'
      delete am;
      return 0;
   }
   
   int loadMode = GDCM_NAME_SPACE::LD_ALL;
   if ( am->ArgMgrDefined("noshadowseq") )
      loadMode |= GDCM_NAME_SPACE::LD_NOSHADOWSEQ;
   else 
   {
      if ( am->ArgMgrDefined("noshadow") )
         loadMode |= GDCM_NAME_SPACE::LD_NOSHADOW;
      if ( am->ArgMgrDefined("noseq") )
         loadMode |= GDCM_NAME_SPACE::LD_NOSEQ;
   }
   
   int filecontent =  am->ArgMgrGetInt("filecontent", 1);
   
   char *filein = am->ArgMgrWantString("filein",usage);
   char *fileout = (char *)(am->ArgMgrGetString("fileout","fileout"));
   
   if (am->ArgMgrDefined("debug"))
      GDCM_NAME_SPACE::Debug::DebugOn();
      
   int deuxD = am->ArgMgrDefined("2D");

   /* if unused Param we give up */
   if ( am->ArgMgrPrintUnusedLabels() )
   {
      am->ArgMgrUsage(usage);
      delete am;
      return 0;
   }
   
// ------------------------------------------------------------  
   std::vector<GDCM_NAME_SPACE::File* > cfl;
         
   GDCM_NAME_SPACE::File *f = GDCM_NAME_SPACE::File::New();
   f->SetFileName(filein);
   f->Load();
   cfl.push_back(f);
  
   vtkGdcmReader *reader = vtkGdcmReader::New();
   reader->AllowLookupTableOff();
   //reader->SetFileName( filein );
   // in order not to parse twice the input file.
   reader->SetCoherentFileList(&cfl);
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
   
   std::string fileName(filein);
   vtkGdcmWriter *writer = vtkGdcmWriter::New();
      
   switch (filecontent)
   {
      case 1:
         writer->SetContentTypeToUserOwnImage();
         fileName = fileName + "_UserOwnImage.dcm";
         break;
 
      case 2:
         writer->SetContentTypeToFilteredImage();
         writer->SetGdcmFile( f );
         fileName = fileName + "_FilteredImage.dcm";
         break;
 
      case 3:
         writer->SetContentTypeToUserCreatedImage();
         writer->SetGdcmFile( f );
         fileName = fileName + "_UserCreatedImage.dcm";
         break;
 
      case 4:
         writer->SetContentTypeToUserCreatedImage();
         writer->SetGdcmFile( f ); 
         fileName = fileName + "_UnmodifiedPixelsImage.dcm";
         break; 
   }
   
/// \todo : fix stupid generated image names (later : JPRx)

   if(deuxD)
   {
         writer->SetFileDimensionality(2);
         writer->SetFilePrefix(fileout);
         writer->SetFilePattern("%s%d.dcm");
   }
   else
   {
      fileName += ".dcm";
      // For 3D
      writer->SetFileDimensionality(3);
      writer->SetFileName(fileName.c_str());   
   }

   writer->SetInput(output);
   writer->Write();
   //////////////////////////////////////////////////////////

   // Clean up
   writer->Delete();
   reader->Delete();

   return 0;
}
