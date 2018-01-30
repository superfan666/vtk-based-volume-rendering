/*=========================================================================
                                                                                
  Program:   gdcm
  Module:    $RCSfile: vtkGdcmReader.cxx,v $
  Language:  C++
  Date:      $Date: 2009/11/03 14:07:00 $
  Version:   $Revision: 1.97 $
                                                                                
  Copyright (c) CREATIS (Centre de Recherche et d'Applications en Traitement de
  l'Image). All rights reserved. See Doc/License.txt or
  http://www.creatis.insa-lyon.fr/Public/Gdcm/License.html for details.
                                                                                
     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.
                                                                                
=========================================================================*/
                                                                                
//-----------------------------------------------------------------------------
// //////////////////////////////////////////////////////////////
//
//===>  Many users expect from vtkGdcmReader it 'orders' the images
//     (that's the job of GDCM_NAME_SPACE::SerieHelper ...)
//     When user *knows* the files with same Serie UID 
//        have same sizes, same 'pixel' type, same color convention, ...
//     the right way to proceed is as follow :
//
//      GDCM_NAME_SPACE::SerieHelper *sh= new GDCM_NAME_SPACE::SerieHelper();
//      // if user wants *not* to load some parts of the file headers
//      sh->SetLoadMode(loadMode);
//
//      // if user wants *not* to load some files 
//      sh->AddRestriction(group, element, value, operator);
//      sh->AddRestriction( ...
//      sh->SetDirectory(directoryWithImages);
//
//      // if user wants to sort reverse order
//      sh->SetSortOrderToReverse(); 
//
//      // here, we suppose only the first 'Serie' is of interest
//      // it's up to the user to decide !
//      GDCM_NAME_SPACE::FileList *l = sh->GetFirstSingleSerieUIDFileSet();
//
//      // if user is doesn't trust too much the files with same Serie UID 
//      if ( !sh->IsCoherent(l) )
//         return; // not same sizes, same 'pixel' type -> stop
//
//      // WARNING : all  that follows works only with 'bona fide' Series
//      // (In some Series; there are more than one 'orientation'
//      // Don't expected to build a 'volume' with that!
//      //
//      // -> use sh->SplitOnOrientation(l)
//      //  - or sh->SplitOnPosition(l), or SplitOnTagValue(l, gr, el) -
//      // depending on what you want to do
//      // and iterate on the various 'X Coherent File Sets'
//
//      // if user *knows* he has to drop the 'duplicates' images
//      // (same Position)
//      sh->SetDropDuplicatePositions(true);
//
//      // Sorting the list is mandatory
//      // a side effect is to compute ZSpacing for the file set
//      sh->OrderFileList(l);        // sort the list
//
//      vtkGdcmReader *reader = vtkGdcmReader::New();
//
//      // if user wants to modify pixel order (Mirror, TopDown, 90°Rotate, ...)
//      // he has to supply the function that does the job 
//      // (a *very* simple example is given in vtkgdcmSerieViewer.cxx)
//      reader->SetUserFunction (userSuppliedFunction);
//
//      // to pass a 'Coherent File List' as produced by GDCM_NAME_SPACE::SerieHelper
//      reader->SetCoherentFileList(l); 
//      reader->Update();
//
// WARNING TODO CLEANME 
// Actual limitations of this code 
//  when a Coherent File List from SerieHelper is not used (bad idea :-(
//
// //////////////////////////////////////////////////////////////

#include "gdcmFileHelper.h"
#include "gdcmFile.h"
#include "gdcmSerieHelper.h" // for ImagePositionPatientOrdering()

#include "vtkGdcmReader.h"
#include "gdcmDebug.h"
#include "gdcmCommon.h"

#include <vtkObjectFactory.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkLookupTable.h>

vtkCxxRevisionMacro(vtkGdcmReader, "$Revision: 1.97 $")
vtkStandardNewMacro(vtkGdcmReader)

//-----------------------------------------------------------------------------
// Constructor / Destructor
vtkGdcmReader::vtkGdcmReader()
{
   this->LookupTable = NULL;
   this->AllowLookupTable = false;
   //this->AllowLightChecking = false;
   this->LoadMode = GDCM_NAME_SPACE::LD_ALL; // Load everything (possible values : 
                                  //  - LD_NOSEQ, 
                                  //  - LD_NOSHADOW,
                                  //  - LD_NOSHADOWSEQ)
   this->CoherentFileList = 0;
   this->UserFunction     = 0;

   this->OwnFile=true;
   // this->Execution=false; // For VTK5.0
   
   this->KeepOverlays = false;
   
   this->FlipY = true; // to keep old behaviour  
}

vtkGdcmReader::~vtkGdcmReader()
{
   this->RemoveAllFileName();
   this->InternalFileNameList.clear();
   if(this->LookupTable) 
      this->LookupTable->Delete();
}

//-----------------------------------------------------------------------------
// Print
void vtkGdcmReader::PrintSelf(ostream &os, vtkIndent indent)
{
   this->Superclass::PrintSelf(os,indent);
   os << indent << "Filenames  : " << endl;
   vtkIndent nextIndent = indent.GetNextIndent();
   for (std::list<std::string>::iterator it = FileNameList.begin();
        it != FileNameList.end();
        ++it)
   {
      os << nextIndent << it->c_str() << endl ;
   }
}

//-----------------------------------------------------------------------------
// Public
/*
 * Remove all files from the list of images to read.
 */
void vtkGdcmReader::RemoveAllFileName(void)
{
   this->FileNameList.clear();
   this->Modified();
}

/*
 * Adds a file name to the list of images to read.
 */
void vtkGdcmReader::AddFileName(const char* name)
{
   // We need to bypass the const pointer [since list<>.push_bash() only
   // takes a char* (but not a const char*)] by making a local copy:
   this->FileNameList.push_back(name);
   this->Modified();
}

/*
 * Sets up a filename to be read.
 */
void vtkGdcmReader::SetFileName(const char *name) 
{
   vtkImageReader2::SetFileName(name);
   // Since we maintain a list of filenames, when building a volume,
   // (see vtkGdcmReader::AddFileName), we additionaly need to purge
   // this list when we manually positionate the filename.
   vtkDebugMacro(<< "Clearing all files given with AddFileName");
   this->FileNameList.clear();
   this->Modified();
}

//-----------------------------------------------------------------------------
// Protected
/*
 * Configure the output e.g. WholeExtent, spacing, origin, scalar type...
 */
void vtkGdcmReader::ExecuteInformation()
{
//   if(this->Execution)  // For VTK5.0
//      return;
//
//   this->Execution=true; // end For VTK5.0
   this->RemoveAllInternalFile();
   if(this->MTime>this->fileTime)
   {
      this->TotalNumberOfPlanes = 0;

      if ( this->CoherentFileList != 0 )
      {
         this->UpdateFileInformation();
      }
      else
      {
         this->BuildFileListFromPattern();
         this->LoadFileInformation();
      }

      if ( this->TotalNumberOfPlanes == 0)
      {
         vtkErrorMacro(<< "File set is not coherent. Exiting...");
         return;
      }

      // if the user has not set the extent, but has set the VOI
      // set the z axis extent to the VOI z axis
      if (this->DataExtent[4]==0 && this->DataExtent[5] == 0 &&
         (this->DataVOI[4] || this->DataVOI[5]))
      {
         this->DataExtent[4] = this->DataVOI[4];
         this->DataExtent[5] = this->DataVOI[5];
      }

      // When the user has set the VOI, check it's coherence with the file content.
      if (this->DataVOI[0] || this->DataVOI[1] || 
      this->DataVOI[2] || this->DataVOI[3] ||
      this->DataVOI[4] || this->DataVOI[5])
      { 
         if ((this->DataVOI[0] < 0) ||
             (this->DataVOI[1] >= this->NumColumns) ||
             (this->DataVOI[2] < 0) ||
             (this->DataVOI[3] >= this->NumLines) ||
             (this->DataVOI[4] < 0) ||
             (this->DataVOI[5] >= this->TotalNumberOfPlanes ))
         {
            vtkWarningMacro(<< "The requested VOI is larger than expected extent.");
            this->DataVOI[0] = 0;
            this->DataVOI[1] = this->NumColumns - 1;
            this->DataVOI[2] = 0;
            this->DataVOI[3] = this->NumLines - 1;
            this->DataVOI[4] = 0;
            this->DataVOI[5] = this->TotalNumberOfPlanes - 1;
         }
      }

      // Set the Extents.
      this->DataExtent[0] = 0;
      this->DataExtent[1] = this->NumColumns - 1;
      this->DataExtent[2] = 0;
      this->DataExtent[3] = this->NumLines - 1;
      this->DataExtent[4] = 0;
      this->DataExtent[5] = this->TotalNumberOfPlanes - 1;
  
      // We don't need to set the Endian related stuff (by using
      // this->SetDataByteOrderToBigEndian() or SetDataByteOrderToLittleEndian()
      // since the reading of the file is done by gdcm.
      // But we do need to set up the data type for downstream filters:
      if      ( ImageType == "8U" )
      {
         vtkDebugMacro(<< "8 bits unsigned image");
         this->SetDataScalarTypeToUnsignedChar(); 
      }
      else if ( ImageType == "8S" )
      {
         vtkErrorMacro(<< "Cannot handle 8 bit signed files");
         return;
      }
      else if ( ImageType == "16U" )
      {
         vtkDebugMacro(<< "16 bits unsigned image");
         this->SetDataScalarTypeToUnsignedShort();
      }
      else if ( ImageType == "16S" )
      {
         vtkDebugMacro(<< "16 bits signed image");
         this->SetDataScalarTypeToShort();
      }
      else if ( ImageType == "32U" )
      {
         vtkDebugMacro(<< "32 bits unsigned image");
         vtkDebugMacro(<< "WARNING: forced to signed int !");
         this->SetDataScalarTypeToInt();
      }
      else if ( ImageType == "32S" )
      {
         vtkDebugMacro(<< "32 bits signed image");
         this->SetDataScalarTypeToInt();
      }
      else if ( ImageType == "FD" )  // This is not genuine DICOM, but so usefull
      {
         vtkDebugMacro(<< "64 bits Double image");
         this->SetDataScalarTypeToDouble();
      }
      //Set number of scalar components:
      this->SetNumberOfScalarComponents(this->NumComponents);

      this->fileTime=this->MTime;
   }

   this->Superclass::ExecuteInformation();  

   //this->GetOutput()->SetUpdateExtentToWholeExtent();// For VTK5.0
   //this->BuildData(this->GetOutput());

   //this->Execution=false;
   //this->RemoveAllInternalFile();                   // End For VTK5.0
}
 
/*
 * Update => ouput->Update => UpdateData => Execute => ExecuteData 
 * (see vtkSource.cxx for last step).
 * This function (redefinition of vtkImageReader::ExecuteData, see 
 * VTK/IO/vtkImageReader.cxx) reads a data from a file. The data
 * extent/axes are assumed to be the same as the file extent/order.
 */
void vtkGdcmReader::ExecuteData(vtkDataObject *output)
{
   vtkImageData *data=vtkImageData::SafeDownCast(output);
   data->SetExtent(this->DataExtent);

/*   if ( CoherentFileList != 0 )   // When a list of names is passed
   {
      if (this->CoherentFileList->empty())
      {
         vtkErrorMacro(<< "Coherent File List must have at least a valid File*.");
         return;
      }
   }
   else if (this->InternalFileNameList.empty())
   {
      vtkErrorMacro(<< "A least a valid FileName must be specified.");
      return;
   }
*/
  
  // data->AllocateScalars();  // For VTK5.0
  // if (this->UpdateExtentIsEmpty(output))
  // {
  //    return;
  // }
//}                           // end For VTK5.0

   data->AllocateScalars();  // For VTK5.0
   
#if (VTK_MAJOR_VERSION >= 5) || ( VTK_MAJOR_VERSION == 4 && VTK_MINOR_VERSION > 2 )
//#if (VTK_MAJOR_VERSION >= 5)
   if (this->UpdateExtentIsEmpty(output))
   {
      return;
   }
#endif

   data->GetPointData()->GetScalars()->SetName("DicomImage-Volume");

   // Test if output has valid extent
   // Prevent memory errors
   if((this->DataExtent[1]-this->DataExtent[0]>=0) &&
      (this->DataExtent[3]-this->DataExtent[2]>=0) &&
      (this->DataExtent[5]-this->DataExtent[4]>=0))
   {
      // The memory size for a full stack of images of course depends
      // on the number of planes and the size of each image:
      //size_t StackNumPixels = this->NumColumns * this->NumLines
      //                      * this->TotalNumberOfPlanes * this->NumComponents;
      //size_t stack_size = StackNumPixels * this->PixelSize; //not used
      // Allocate pixel data space itself.

      // Variables for the UpdateProgress. We shall use 50 steps to signify
      // the advance of the process:
      unsigned long UpdateProgressTarget = (unsigned long) ceil (this->NumLines
                                         * this->TotalNumberOfPlanes
                                         / 50.0);
      // The actual advance measure:
      unsigned long UpdateProgressCount = 0;

      // Filling the allocated memory space with each image/volume:

      size_t size = this->NumColumns * this->NumLines * this->NumPlanes
                  * data->GetScalarSize() * this->NumComponents;
      unsigned char *Dest = (unsigned char *)data->GetScalarPointer();
      for (std::vector<GDCM_NAME_SPACE::File* >::iterator it =  InternalFileList.begin();
                                               it != InternalFileList.end();
                                             ++it)
      {
         this->LoadImageInMemory(*it, Dest,
                                 UpdateProgressTarget,
                                 UpdateProgressCount); 
         Dest += size;
      }
   }
   this->RemoveAllInternalFile(); // For VTK5.0
}

/*
 * vtkGdcmReader can have the file names specified through two ways:
 * (1) by calling the vtkImageReader2::SetFileName(), SetFilePrefix() and
 *     SetFilePattern()
 * (2) By successive calls to vtkGdcmReader::AddFileName()
 * When the first method was used by caller we need to update the local
 * filename list
 */
void vtkGdcmReader::BuildFileListFromPattern()
{
   this->RemoveAllInternalFileName();

   // Test miscellanous cases
   if ((! this->FileNameList.empty()) && this->FileName )
   {
      vtkErrorMacro(<< "Both AddFileName and SetFileName schemes were used");
      vtkErrorMacro(<< "No images loaded ! ");
      return;
   }

   if ((! this->FileNameList.empty()) && this->FilePrefix )
   {
      vtkErrorMacro(<< "Both AddFileName and SetFilePrefix schemes were used");
      vtkErrorMacro(<< "No images loaded ! ");
      return;
   }

   if (this->FileName && this->FilePrefix)
   {
      vtkErrorMacro(<< "Both SetFileName and SetFilePrefix schemes were used");
      vtkErrorMacro(<< "No images loaded ! ");
      return;
   }

   // Create the InternalFileNameList
   if (! this->FileNameList.empty()  )
   {
      vtkDebugMacro(<< "Using the AddFileName specified files");
      this->InternalFileNameList=this->FileNameList;
      return;
   }

   if (!this->FileName && !this->FilePrefix)
   {
      vtkErrorMacro(<< "FileNames are not set. Either use AddFileName() or");
      vtkErrorMacro(<< "specify a FileName or FilePrefix.");
      return;
   }

   if( this->FileName )
   {
      // Single file loading (as given with ::SetFileName()):
      // Case of multi-frame file considered here
      this->ComputeInternalFileName(this->DataExtent[4]);
      vtkDebugMacro(<< "Adding file " << this->InternalFileName);
      this->AddInternalFileName(this->InternalFileName);
   }
   else
   {
      // Multi file loading (as given with ::SetFilePattern()):
      for (int idx = this->DataExtent[4]; idx <= this->DataExtent[5]; ++idx)
      {
         this->ComputeInternalFileName(idx);
         vtkDebugMacro(<< "Adding file " << this->InternalFileName);
         this->AddInternalFileName(this->InternalFileName);
      }
   }
}

/**
 * Load all the files and set it in the InternalFileList
 * For each file, the readability and the coherence of image caracteristics 
 * are tested. If an image doesn't agree the required specifications, it
 * isn't considered and no data will be set for the planes corresponding
 * to this image
 *
 * The source of this work is the list of file name generated by the
 * BuildFileListFromPattern method
 */
void vtkGdcmReader::LoadFileInformation()
{
   GDCM_NAME_SPACE::File *file;
   bool foundReference=false;
   std::string type;

   this->OwnFile=true;
   for (std::list<std::string>::iterator filename = InternalFileNameList.begin();
        filename != InternalFileNameList.end();
        ++filename)
   {
      // check for file readability
      FILE *fp;
      fp = fopen(filename->c_str(),"rb");
      if (!fp)
      {
         vtkErrorMacro(<< "Unable to open file " << filename->c_str());
         vtkErrorMacro(<< "Removing this file from read files: "
                       << filename->c_str());
         file = NULL;
         InternalFileList.push_back(file);
         continue;
      }
      fclose(fp);

      // Read the file
      file=GDCM_NAME_SPACE::File::New();
      file->SetLoadMode( LoadMode );
      file->SetFileName(filename->c_str() );
      file->Load();

      // Test the Dicom file readability
      if(!file->IsReadable())
      {
         vtkErrorMacro(<< "Gdcm cannot parse file " << filename->c_str());
         vtkErrorMacro(<< "Removing this file from read files: "
                        << filename->c_str());
         file->Delete();
         file=NULL;
         InternalFileList.push_back(file);
         continue;
      }

      // Test the Pixel Type recognition
      type = file->GetPixelType();
      if (   (type !=  "8U") && (type !=  "8S")
          && (type != "16U") && (type != "16S")
          && (type != "32U") && (type != "32S")
          && (type != "FD")  )                // Sure this one is NOT kosher
      {
         vtkErrorMacro(<< "Bad File Type for file " << filename->c_str() << "\n"
                       << "   File type found : " << type.c_str() 
                       << " (might be 8U, 8S, 16U, 16S, 32U, 32S, FD) \n"
                       << "   Removing this file from read files");
         file->Delete();
         file=NULL;
         InternalFileList.push_back(file);
         continue;
      }

      // Test the image informations
      if(!foundReference)
      {
         foundReference = true;
         GetFileInformation(file);

         vtkDebugMacro(<< "This file taken as coherence reference:"
                        << filename->c_str());
         vtkDebugMacro(<< "Image dimensions of reference file as read from Gdcm:" 
                        << this->NumColumns << " " << this->NumLines << " " 
                        << this->NumPlanes);
      }
      else if(!TestFileInformation(file))
      {
         file->Delete();
         file=NULL;
      }

      InternalFileList.push_back(file);
   }
}

/**
 * Update the file informations.
 * This works exactly like LoadFileInformation, but the source of work
 * is the list of coherent files
 */
void vtkGdcmReader::UpdateFileInformation()
{
   this->InternalFileList=*(this->CoherentFileList);
   this->OwnFile=false;

   for(gdcmFileList::iterator it=InternalFileList.begin();
                              it!=InternalFileList.end();
                              ++it)
   {
      if( *it != NULL)
      {
         GetFileInformation(*it);
         break;
      }
   }
}

/**
 * Get the informations from a file.
 * These informations are required to specify the output image
 * caracteristics
 */
void vtkGdcmReader::GetFileInformation(GDCM_NAME_SPACE::File *file)
{
   // Get the image caracteristics
   this->NumColumns = file->GetXSize();
   this->NumLines   = file->GetYSize();
   this->NumPlanes  = file->GetZSize();

   if (CoherentFileList == 0)
      this->TotalNumberOfPlanes = this->NumPlanes*InternalFileNameList.size();
   else
      this->TotalNumberOfPlanes = this->NumPlanes*CoherentFileList->size();

   this->ImageType = file->GetPixelType();
   this->PixelSize = file->GetPixelSize();

   this->DataSpacing[0] = file->GetXSpacing();
   this->DataSpacing[1] = file->GetYSpacing();
   
   //  Most of the file headers have NO z spacing
   //  It must be calculated from the whole GDCM_NAME_SPACE::Serie (if any)
   //  using Jolinda Smith's algoritm.
   //  see GDCM_NAME_SPACE::SerieHelper::ImagePositionPatientOrdering()
   if (CoherentFileList == 0)   
      this->DataSpacing[2] = file->GetZSpacing();
   else
   {
       // Just because OrderFileList() is a member of GDCM_NAME_SPACE::SerieHelper
       // we need to instanciate sh.
      GDCM_NAME_SPACE::SerieHelper *sh = GDCM_NAME_SPACE::SerieHelper::New();
      sh->OrderFileList(CoherentFileList); // calls ImagePositionPatientOrdering()
      this->DataSpacing[2] = sh->GetZSpacing();
      sh->Delete();         
   } 

   // Get the image data caracteristics
   if( file->HasLUT() && this->AllowLookupTable )
   {
      // I could raise an error is AllowLookupTable is on and HasLUT() off
      this->NumComponents = file->GetNumberOfScalarComponentsRaw();
   }
   else
   {
      this->NumComponents = file->GetNumberOfScalarComponents(); //rgb or mono
   }
}

/*
 * When more than one filename is specified (i.e. we expect loading
 * a stack or volume) we need to check that the corresponding images/volumes
 * to be loaded are coherent i.e. to make sure:
 *     - they all share the same X dimensions
 *     - they all share the same Y dimensions
 *     - they all share the same ImageType ( 8 bit signed, or unsigned...)
 *
 * Eventually, we emit a warning when all the files do NOT share the
 * Z dimension, since we can still build a stack but the
 * files are not coherent in Z, which is probably a source a trouble...
 *   When files are not readable (either the file cannot be opened or
 * because gdcm cannot parse it), they are flagged as "GDCM_UNREADABLE".  
 *   This method returns the total number of planar images to be loaded
 * (i.e. an image represents one plane, but a volume represents many planes)
 */
/**
 * Test the coherent informations of the file with the reference informations
 * used as image caracteristics. The tested informations are :
 * - they all share the same X dimensions
 * - they all share the same Y dimensions
 * - they all share the same Z dimensions
 * - they all share the same number of components
 * - they all share the same ImageType ( 8 bit signed, or unsigned...)
 *
 * \return True if the file match, False otherwise
 */
bool vtkGdcmReader::TestFileInformation(GDCM_NAME_SPACE::File *file)
{
   int numColumns = file->GetXSize();
   int numLines   = file->GetYSize();
   int numPlanes  = file->GetZSize();
   int numComponents;
   unsigned int pixelSize  = file->GetPixelSize();

   if( file->HasLUT() && this->AllowLookupTable )
      numComponents = file->GetNumberOfScalarComponentsRaw();
   else
      numComponents = file->GetNumberOfScalarComponents(); //rgb or mono

   if( numColumns != this->NumColumns )
   {
      vtkErrorMacro(<< "File X value doesn't match with the previous ones: "
                    << file->GetFileName().c_str()
                    << ". Found " << numColumns << ", must be "
                    << this->NumColumns);
      return false;
   }
   if( numLines != this->NumLines )
   {
      vtkErrorMacro(<< "File Y value doesn't match with the previous ones: "
                    << file->GetFileName().c_str()
                    << ". Found " << numLines << ", must be "
                    << this->NumLines);
      return false;
   }
   if( numPlanes != this->NumPlanes )
   {
      vtkErrorMacro(<< "File Z value doesn't match with the previous ones: "
                    << file->GetFileName().c_str()
                    << ". Found " << numPlanes << ", must be "
                    << this->NumPlanes);
      return false;
   }
   if( numComponents != this->NumComponents )
   {
      vtkErrorMacro(<< "File Components count doesn't match with the previous ones: "
                    << file->GetFileName().c_str()
                    << ". Found " << numComponents << ", must be "
                    << this->NumComponents);
      return false;
   }
   if( pixelSize != this->PixelSize )
   {
      vtkErrorMacro(<< "File pixel size doesn't match with the previous ones: "
                    << file->GetFileName().c_str()
                    << ". Found " << pixelSize << ", must be "
                    << this->PixelSize);
      return false;
   }

   return true;
}

//-----------------------------------------------------------------------------
// Private
/*
 * Remove all file names to the internal list of images to read.
 */
void vtkGdcmReader::RemoveAllInternalFileName(void)
{
   this->InternalFileNameList.clear();
}

/*
 * Adds a file name to the internal list of images to read.
 */
void vtkGdcmReader::AddInternalFileName(const char *name)
{
   char *LocalName = new char[strlen(name) + 1];
   strcpy(LocalName, name);
   this->InternalFileNameList.push_back(LocalName);
   delete[] LocalName;
}

/*
 * Remove all file names to the internal list of images to read.
 */
void vtkGdcmReader::RemoveAllInternalFile(void)
{
   if(this->OwnFile)
   {
      for(gdcmFileList::iterator it=InternalFileList.begin();
                                 it!=InternalFileList.end();
                                 ++it)
      {
         (*it)->Delete();
      }
   }
   this->InternalFileList.clear();
}

void vtkGdcmReader::IncrementProgress(const unsigned long updateProgressTarget,
                                      unsigned long &updateProgressCount)
{
   // Update progress related for bad files:
   updateProgressCount += this->NumLines;
   if (updateProgressTarget > 0)
   {
      if (!(updateProgressCount%updateProgressTarget))
      {
         this->UpdateProgress(
             updateProgressCount/(50.0*updateProgressTarget));
      }
   }
}

/*
 * Loads the contents of the image/volume contained by char *fileName at
 * the dest memory address. Returns the size of the data loaded.
 */
/*void vtkGdcmReader::LoadImageInMemory(
             std::string fileName, 
             unsigned char *dest,
             const unsigned long updateProgressTarget,
             unsigned long &updateProgressCount)
{
   vtkDebugMacro(<< "Copying to memory image [" << fileName.c_str() << "]");

   GDCM_NAME_SPACE::File *f;
   f = new GDCM_NAME_SPACE::File();
   f->SetLoadMode( LoadMode );
   f->SetFileName( fileName.c_str() );
   f->Load( );

   LoadImageInMemory(f,dest,
                     updateProgressTarget,
                     updateProgressCount);
   delete f;
}*/

/*
 * Loads the contents of the image/volume contained by GDCM_NAME_SPACE::File* f at
 * the Dest memory address. Returns the size of the data loaded.
 * \ param f File to consider. NULL if the file must be skiped
 * \remarks Assume that if (f != NULL) then its caracteristics match
 * with the previous ones
 */
void vtkGdcmReader::LoadImageInMemory(
             GDCM_NAME_SPACE::File *f, 
             unsigned char *dest,
             const unsigned long updateProgressTarget,
             unsigned long &updateProgressCount)
{
   if(!f)
      return;

   GDCM_NAME_SPACE::FileHelper *fileH = GDCM_NAME_SPACE::FileHelper::New( f );
   fileH->SetUserFunction( UserFunction );
   
   fileH->SetKeepOverlays ( this->KeepOverlays);
   
   int numColumns = f->GetXSize();
   int numLines   = f->GetYSize();
   int numPlanes  = f->GetZSize();
   int numComponents;

   if( f->HasLUT() && this->AllowLookupTable )
      numComponents = f->GetNumberOfScalarComponentsRaw();
   else
      numComponents = f->GetNumberOfScalarComponents(); //rgb or mono
   vtkDebugMacro(<< "numComponents:" << numComponents);
   vtkDebugMacro(<< "Copying to memory image [" << f->GetFileName().c_str() << "]");
   //size_t size;

   // If the data structure of vtk for image/volume representation
   // were straigthforwards the following would be enough:
   //    GdcmFile.GetImageDataIntoVector((void*)Dest, size);
   // But vtk chooses to invert the lines of an image, that is the last
   // line comes first (for some axis related reasons?). Hence we need
   // to load the image line by line, starting from the end.

   int lineSize   = NumComponents * numColumns * f->GetPixelSize();
   int planeSize  = lineSize * numLines;

   unsigned char *src;
   
   if( fileH->GetFile()->HasLUT() && AllowLookupTable )
   {
      // to avoid bcc 5.5 w
      /*size               = */ fileH->GetImageDataSize(); 
      src                = (unsigned char*) fileH->GetImageDataRaw();
      unsigned char *lut = (unsigned char*) fileH->GetLutRGBA();

      if(!this->LookupTable)
      {
         this->LookupTable = vtkLookupTable::New();
      }

      this->LookupTable->SetNumberOfTableValues(256);
      for (int tmp=0; tmp<256; tmp++)
      {
         this->LookupTable->SetTableValue(tmp,
         (float)lut[4*tmp+0]/255.0,
         (float)lut[4*tmp+1]/255.0,
         (float)lut[4*tmp+2]/255.0,
         1);
      }
      this->LookupTable->SetRange(0,255);
      vtkDataSetAttributes *a = this->GetOutput()->GetPointData();
      a->GetScalars()->SetLookupTable(this->LookupTable);
      delete[] lut;
   }
   else
   {
      //size = fileH->GetImageDataSize(); 
      // useless - just an accessor;  'size' unused
      //if (this->GetFlipY())
         src  = (unsigned char*)fileH->GetImageData();
      //else
      // very strange, but it doesn't work (I have to memcpy the pixels ?!?)
      //   dest  = (unsigned char*)fileH->GetImageData();        
   }

if (this->GetFlipY()) {
   unsigned char *dst = dest + planeSize - lineSize;
   for (int plane = 0; plane < numPlanes; plane++)
   {
      for (int line = 0; line < numLines; line++)
      {
         // Copy one line at proper destination:
         memcpy((void*)dst, (void*)src, lineSize);
         src += lineSize;
         dst -= lineSize;
         // Update progress related:
         if (!(updateProgressCount%updateProgressTarget))
         {
            this->UpdateProgress(
               updateProgressCount/(50.0*updateProgressTarget));
         }
         updateProgressCount++;
      }
      dst += 2 * planeSize;
   }
}
else // we don't flip (upside down) the image
{
  memcpy((void*)dest, (void*)src,  numPlanes * numLines * lineSize);
}
   fileH->Delete();
}

//-----------------------------------------------------------------------------
