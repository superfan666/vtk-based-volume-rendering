/*=========================================================================
                                                                                
  Program:   gdcm
  Module:    $RCSfile: vtkgdcmSerieViewer2.cxx,v $
  Language:  C++
  Date:      $Date: 2007/06/21 14:47:16 $
  Version:   $Revision: 1.11 $
                                                                                
  Copyright (c) CREATIS (Centre de Recherche et d'Applications en Traitement de
  l'Image). All rights reserved. See Doc/License.txt or
  http://www.creatis.insa-lyon.fr/Public/Gdcm/License.html for details.
                                                                                
     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.
                                                                                
=========================================================================*/
// This example illustrates how the vtkGdcmReader vtk class can 
// use the result of GDCM_NAME_SPACE::SerieHelper constructor and check
// the various Setters :
//     SerieHelper::SetOrderToReverse, 
//     SerieHelper::SetUserLessThanFunction
//     SerieHelper::SetLoadMode
//     SerieHelper::SetDropDuplicatePositions
//     vtkGdcmReader::SetUserFunction
//     vtkGdcmReader::SetCoherentFileList
// Usage:
//  * the Directory name that contains the Dicom images constituting the stack 
//    should be given as command line argument (keyword : dirname=),
//  * you can navigate through the stack by hitting any character key,
//  * the produced vtk file is named "foo.vtk" (in the invocation directory).
// 
//----------------------------------------------------------------------------
#include <vtkRenderWindowInteractor.h>
#include <vtkImageViewer2.h>
#include <vtkStructuredPoints.h>
#include <vtkStructuredPointsWriter.h>
#include <vtkCommand.h>
#include <vtkRenderer.h>
#include <vtkImageMapToColors.h>
#include <vtkLookupTable.h>

#include "vtkGdcmReader.h"
#include "gdcmDocument.h"  // for NO_SHADOWSEQ
#include "gdcmSerieHelper.h"
#include "gdcmDebug.h"
#include "gdcmDataEntry.h"

#include "gdcmArgMgr.h" // for Argument Manager functions
#include <string.h>     // for strcmp
#ifndef vtkFloatingPointType
#define vtkFloatingPointType float
#endif

void userSuppliedMirrorFunction (uint8_t *im, GDCM_NAME_SPACE::File *f);
void userSuppliedUpsideDownFunction(uint8_t *im, GDCM_NAME_SPACE::File *f);
bool userSuppliedLessThanFunction(GDCM_NAME_SPACE::File *f1, GDCM_NAME_SPACE::File *f2);
bool userSuppliedLessThanFunction2(GDCM_NAME_SPACE::File *f1, GDCM_NAME_SPACE::File *f2);

int orderNb;
uint16_t *elemsToOrderOn;

//----------------------------------------------------------------------------
// Callback for the interaction
class vtkgdcmObserver : public vtkCommand
{
public:
   virtual char const *GetClassName() const 
   { 
      return "vtkgdcmObserver";
   }

   static vtkgdcmObserver *New() 
   { 
      return new vtkgdcmObserver; 
   }

   vtkgdcmObserver()
   {
      this->ImageViewer = NULL;
   }

   virtual void Execute(vtkObject *, unsigned long event, void* )
   {
      if ( this->ImageViewer )
      {
         if ( event == vtkCommand::CharEvent )
         {
#if (VTK_MAJOR_VERSION >= 5)
            int max = ImageViewer->GetSliceMax();
            int slice = (ImageViewer->GetSlice() + 1 ) % ++max;
            ImageViewer->SetSlice( slice );
#else
            int max = ImageViewer->GetWholeZMax();
            int slice = (ImageViewer->GetZSlice() + 1 ) % ++max;
            ImageViewer->SetZSlice( slice );
#endif
#if !( (VTK_MAJOR_VERSION >= 5) || ( VTK_MAJOR_VERSION == 4 && VTK_MINOR_VERSION >= 5 ) )
         // This used to be a bug in version VTK 4.4 and earlier
            ImageViewer->GetRenderer()->ResetCameraClippingRange();
#endif
            ImageViewer->Render();
         }
      }
   }
   vtkImageViewer2 *ImageViewer;
};

int main(int argc, char *argv[])
{
   START_USAGE(usage)
   " \n vtkgdcmSerieViewer2 : \n",
   " Display a 'Serie' (same Serie UID) within a Directory                    ",
   " You can navigate through the stack by hitting any character key.         ",
   " usage: vtkgdcmSerieViewer dirname=sourcedirectory                        ",
   "                           [noshadowseq][noshadow][noseq]                 ",
   "                           [reverse] [{[mirror]|[topdown]|[rotate]}]      ",
   "                           [order=] [nodup][check][debug]                 ",
   "      sourcedirectory : name of the directory holding the images          ",
   "                        if it holds more than one serie,                  ",
   "                        only the first one is displayed.                  ",
   "      noshadowseq: user doesn't want to load Private Sequences            ",
   "      noshadow   : user doesn't want to load Private groups (odd number)  ",
   "      noseq      : user doesn't want to load Sequences                    ",
   "      reverse    : user wants to sort the images reverse order            ",
   "      mirror     : user wants to 'mirror' the images    | just some simple",
   "      upsidedown : user wants to 'upsidedown' the images| examples of user",
   "                                                        | suppliedfunction",
   "      check      : user wants to force more coherence checking            ",
   "      order=     : group1-elem1,group2-elem2,... (in hexa, no space)      ",
   "                   if we want to use them as a sort criterium             ",
   "                   Right now : ValEntries only -just an example-          ",
   "        or                                                                ",
   "      order=     : order=name if we want to sort on file name (why not ?) ",
   "      nodup       : user wants to drop duplicate positions                ",
   "      debug      : developper wants to run the program in 'debug mode'    ",
   FINISH_USAGE


   // Initialize Arguments Manager   
   GDCM_NAME_SPACE::ArgMgr *am= new GDCM_NAME_SPACE::ArgMgr(argc, argv);
  
   if (argc == 1 || am->ArgMgrDefined("usage") )
   {
      am->ArgMgrUsage(usage); // Display 'usage'
      delete am;
      return 0;
   }

   char *dirName = am->ArgMgrWantString("dirname",usage);

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

   int reverse = am->ArgMgrDefined("reverse");
   int nodup   = am->ArgMgrDefined("nodup");
   int mirror     = am->ArgMgrDefined("mirror");
   int upsidedown = am->ArgMgrDefined("upsidedown");

   if ( mirror && upsidedown )
   {
      std::cout << "*EITHER* mirror *OR* upsidedown !"
                << std::endl;
      delete am;
      return 0;
   }

   int check   = am->ArgMgrDefined("check");
  
   // This is so ugly, a cstring is NOT a char * (god damit!)
   bool bname = ( strcmp(am->ArgMgrGetString("order", "not found"),"name")==0 );
   if (bname)
      elemsToOrderOn = am->ArgMgrGetXInt16Enum("order", &orderNb);

   if (am->ArgMgrDefined("debug"))
      GDCM_NAME_SPACE::Debug::DebugOn();

   /* if unused Param we give up */
   if ( am->ArgMgrPrintUnusedLabels() )
   {
      am->ArgMgrUsage(usage);
      delete am;
      return 0;
   } 

   delete am;  // we don't need Argument Manager any longer

   // ----------------------- End Arguments Manager ----------------------
  
   GDCM_NAME_SPACE::SerieHelper *sh = GDCM_NAME_SPACE::SerieHelper::New();
   sh->SetLoadMode(loadMode);
   if (reverse)
      sh->SetSortOrderToReverse();
   sh->SetDirectory( dirName, true);
    
   // Just to see

   int nbFiles;
   // For all the 'Single Serie UID' FileSets of the GDCM_NAME_SPACE::Serie
   GDCM_NAME_SPACE::FileList *l = sh->GetFirstSingleSerieUIDFileSet();
   if (l == 0 )
   {
      std::cout << "Oops! No 'Single Serie UID' FileSet found ?!?" << std::endl;
      return 0;
   }

   if (bname)
     sh->SetUserLessThanFunction(userSuppliedLessThanFunction2);
   else if (orderNb != 0)
      sh->SetUserLessThanFunction(userSuppliedLessThanFunction);

   if (nodup)
      sh->SetDropDuplicatePositions(true);
      
   while (l)
   { 
      nbFiles = l->size() ;
      if ( l->size() > 1 )
      {
         std::cout << "Sort list : " << nbFiles << " long" << std::endl;
 
         //---------------------------------------------------------
         sh->OrderFileList(l);  // sort the list (and compute ZSpacing !)
         //---------------------------------------------------------
 
         double zsp = sh->GetZSpacing();
         std::cout << "List sorted, ZSpacing = " << zsp << std::endl;
         break;  // The first one is OK. user will have to check
      }
      else
      {
         std::cout << "Oops! Empty 'Single Serie UID' FileSet found ?!?"
                   << std::endl;
      }
      l = sh->GetNextSingleSerieUIDFileSet();
   }

   if (check)
   {
      if ( !sh->IsCoherent(l) ) // just be sure (?)
      {
         std::cout << "Files are not coherent. Stop everything " << std::endl;
         sh->Delete();
         return 0;
      }
   }

   vtkGdcmReader *reader = vtkGdcmReader::New();
   reader->AllowLookupTableOff();

   if (mirror)
      reader->SetUserFunction (userSuppliedMirrorFunction);
   else if (upsidedown)
      reader->SetUserFunction (userSuppliedUpsideDownFunction);

   // Only the first FileList is dealt with (just an example)
   // (The files will not be parsed twice by the reader)

   //---------------------------------------------------------
   reader->SetCoherentFileList(l);
   //---------------------------------------------------------

   // because we passed a Coherent File List from a SerieHelper,
   // setting LoadMode is useless in this case
   //  reader->SetLoadMode(NO_SHADOWSEQ);  
   reader->Update();

   //print debug info:
   reader->GetOutput()->Print( cout );

   vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();

   vtkImageViewer2 *viewer = vtkImageViewer2::New();

   if( reader->GetLookupTable() )
   {
      //convert to color:
      vtkImageMapToColors *map = vtkImageMapToColors::New ();
      map->SetInput (reader->GetOutput());
      map->SetLookupTable (reader->GetLookupTable());
      map->SetOutputFormatToRGB();
      viewer->SetInput ( map->GetOutput() );
      map->Delete();
   }
   else
   {
      vtkFloatingPointType *range = reader->GetOutput()->GetScalarRange();
      viewer->SetColorLevel (0.5 * (range[1] + range[0]));
      viewer->SetColorWindow (range[1] - range[0]);

      viewer->SetInput ( reader->GetOutput() );
   }
   viewer->SetupInteractor (iren);
  
   //vtkFloatingPointType *range = reader->GetOutput()->GetScalarRange();
   //viewer->SetColorWindow (range[1] - range[0]);
   //viewer->SetColorLevel (0.5 * (range[1] + range[0]));

   // Here is where we setup the observer, 
   vtkgdcmObserver *obs = vtkgdcmObserver::New();
   obs->ImageViewer = viewer;
   iren->AddObserver(vtkCommand::CharEvent,obs);
   obs->Delete();

   //viewer->Render();
   iren->Initialize();
   iren->Start();

   //if you wish you can export dicom to a vtk file  
   vtkStructuredPointsWriter *writer = vtkStructuredPointsWriter::New();
   writer->SetInput( reader->GetOutput());
   writer->SetFileName( "foo.vtk" );
   writer->SetFileTypeToBinary();
   //writer->Write();

   reader->Delete();
   iren->Delete();
   viewer->Delete();
   writer->Delete();

   return 0;
}


// --------------------------------------------------------
// This is just a *very* simple example of user supplied function
//      to mirror (why not ?) the image
// It's *not* part of gdcm.
// --------------------------------------------------------

#define UF(ty)                          \
   int i, j;                            \
   ty *imj;                             \
   ty tamp;                             \
   for (j=0;j<ny;j++)                   \
   {                                    \
      imj = (ty *)im +j*nx;             \
      for (i=0;i<nx/2;i++)              \
      {                                 \
        tamp       =imj[i];             \
        imj[i]     =imj[nx-1-i];        \
        imj[nx-1-i]=tamp;               \
      }                                 \
   }                                    \
   if (nx%2 != 0)                       \
   {                                    \
      i = nx / 2;                       \
      for (j=0;j<ny;j++)                \
      {                                 \
        imj = (ty *)im  +j*nx;          \
        tamp       =imj[i];             \
        imj[i]     =imj[nx/2+1];        \
        imj[nx/2+1]=tamp;               \
      }                                 \
   }

void userSuppliedMirrorFunction(uint8_t *im, GDCM_NAME_SPACE::File *f)
{
   if (f->GetZSize() != 1)
   {
      std::cout << "mirror : Multiframe images not yet dealt with" << std::endl;
      return;
   }

   if (f->GetSamplesPerPixel() != 1 || f->GetBitsAllocated() == 24)
   {
      std::cout << "mirror : RGB / YBR not yet dealt with" << std::endl;
      return;
   }
   int nx = f->GetXSize();
   int ny = f->GetYSize();

   std::string pixelType = f->GetPixelType();
   if ( pixelType ==  "8U" || pixelType == "8S" )
   {
      UF(uint8_t)
      return;
   }
   if ( pixelType == "16U" || pixelType == "16S")
   {
      UF(uint16_t)
      return;
   }
   std::cout << "mirror : Pixel Size (!=8, !=16) not yet dealt with" 
             << std::endl;
   return;
}


// --------------------------------------------------------
// This is just a *very* simple example of user supplied function
//      to upsidedown (why not ?) the image
// It's *not* part of gdcm.
// --------------------------------------------------------

#define UF2(ty)                         \
   int i, j;                            \
   ty *imj, *imJ;                       \
   ty tamp;                             \
   for (j=0;j<ny/2;j++)                 \
   {                                    \
      imj = (ty *)im +j*nx;             \
      imJ = (ty *)im +(ny-1-j)*nx;      \
      for (i=0;i<nx;i++)                \
      {                                 \
        tamp       =imj[i];             \
        imj[i]     =imJ[i];             \
        imJ[i]     =tamp;               \
      }                                 \
   }

void userSuppliedUpsideDownFunction(uint8_t *im, GDCM_NAME_SPACE::File *f)
{
   if (f->GetZSize() != 1)
   {
      std::cout << "mirror : Multiframe images not yet dealt with" << std::endl;
      return;
   }

   if (f->GetSamplesPerPixel() != 1 || f->GetBitsAllocated() == 24)
   {
      std::cout << "mirror : RGB / YBR not yet dealt with" << std::endl;
      return;
   }
   int nx = f->GetXSize();
   int ny = f->GetYSize();

   std::string pixelType = f->GetPixelType();
   if ( pixelType ==  "8U" || pixelType == "8S" )
   {
      UF2(uint8_t)
      return;
   }
   if ( pixelType == "16U" || pixelType == "16S")
   {
      UF2(uint16_t)
      return;
   }
   std::cout << "topdown : Pixel Size (!=8, !=16) not yet dealt with" 
             << std::endl;
   return;
}

// --------------------------------------------------------
// This is just a *very* simple example of user supplied 'LessThan' function
// It's *not* part of gdcm.
//
// Note : orderNb and elemsToOrderOn are here global variables.
// Within a 'normal' function they would't be any orderNb and elemsToOrderOn var
// User *knows* on what field(s) he wants to compare; 
// He just writes a decent function.
// Here, we want to get info from the command line Argument Manager.
//
// Warning : it's up to 'vtkgdcmSerieViewer' user to find a suitable data set !
// --------------------------------------------------------


bool userSuppliedLessThanFunction(GDCM_NAME_SPACE::File *f1, GDCM_NAME_SPACE::File *f2)
{
   // for *this* user supplied function, I supposed only ValEntries are checked.
// 
   std::string s1, s2;
   GDCM_NAME_SPACE::DataEntry *e1,*e2;
   for (int ri=0; ri<orderNb; ri++)
   {
      std::cout << std::hex << elemsToOrderOn[2*ri] << "|" 
                            << elemsToOrderOn[2*ri+1]
                            << std::endl;
 
      e1= f1->GetDataEntry( elemsToOrderOn[2*ri],
                 elemsToOrderOn[2*ri+1]);

      e2= f2->GetDataEntry( elemsToOrderOn[2*ri],
                                 elemsToOrderOn[2*ri+1]);
      if(!e2 || !e2)
      {
         std::cout << std::hex << elemsToOrderOn[2*ri] << "|"
                              << elemsToOrderOn[2*ri+1]
                              << " not found" << std::endl;
         continue;
      }
      s1 = e1->GetString();      
      s2 = e2->GetString();
      std::cout << "[" << s1 << "] vs [" << s2 << "]" << std::endl;
      if ( s1 < s2 ) 
         return true;
      else if (s1 == s2 )
         continue;
      else 
         return false;
   }
   return false; // all fields equal
}

// --------------------------------------------------------
// This is just an other *very* simple example of user supplied 'LessThan'
// function
// It's *not* part of gdcm.
//
// Warning : it's up to 'vtkgdcmSerieViewer' user to find a suitable data set !
// --------------------------------------------------------

bool userSuppliedLessThanFunction2(GDCM_NAME_SPACE::File *f1, GDCM_NAME_SPACE::File *f2)
{
   std::cout << "[" << f1->GetFileName() << "] vs [" 
                    << f2->GetFileName() << "]" << std::endl;
   return f1->GetFileName() < f2->GetFileName();
}
