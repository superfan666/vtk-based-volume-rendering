/*=========================================================================
                                                                                
  Program:   gdcm
  Module:    $RCSfile: vtkGdcmReader.h,v $
  Language:  C++
  Date:      $Date: 2009/11/03 14:05:23 $
  Version:   $Revision: 1.37 $
                                                                                
  Copyright (c) CREATIS (Centre de Recherche et d'Applications en Traitement de
  l'Image). All rights reserved. See Doc/License.txt or
  http://www.creatis.insa-lyon.fr/Public/Gdcm/License.html for details.
                                                                                
     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.
                                                                                
=========================================================================*/
                                                                                
#ifndef __vtkGdcmReader_h
#define __vtkGdcmReader_h

#include "gdcmCommon.h" // To avoid warnings concerning the std
#include "gdcmFile.h"

#include <vtkImageReader.h>
#include <list>
#include <string>
#include <vector>

typedef void (*VOID_FUNCTION_PUINT8_PFILE_POINTER)(uint8_t *, GDCM_NAME_SPACE::File *);

//-----------------------------------------------------------------------------
class vtkLookupTable;

//-----------------------------------------------------------------------------
class VTK_EXPORT vtkGdcmReader : public vtkImageReader
{
public:
   static vtkGdcmReader *New();
   vtkTypeRevisionMacro(vtkGdcmReader, vtkImageReader);
   void PrintSelf(ostream& os, vtkIndent indent);

   virtual void RemoveAllFileName(void);
   virtual void AddFileName(const char *name);
   virtual void SetFileName(const char *name);

   //BTX
   void SetCoherentFileList( std::vector<GDCM_NAME_SPACE::File* > *cfl) {
                                                      CoherentFileList = cfl; }    
   //ETX

   //vtkSetMacro(AllowLightChecking, bool);
   //vtkGetMacro(AllowLightChecking, bool);
   //vtkBooleanMacro(AllowLightChecking, bool);

   //BTX
   
   /// \todo fix possible problems around VTK pipelining
   
   void SetUserFunction (VOID_FUNCTION_PUINT8_PFILE_POINTER userFunc )
                        { UserFunction = userFunc; } 
   //ETX
  
   // Description:
   // If this flag is set and the DICOM reader encounters a dicom file with 
   // lookup table the data will be kept as unsigned chars and a lookuptable 
   // will be exported and accessible through GetLookupTable()
   
   vtkSetMacro(AllowLookupTable, bool);
   vtkGetMacro(AllowLookupTable, bool);
   vtkBooleanMacro(AllowLookupTable, bool);

   vtkSetMacro(KeepOverlays, bool);
   vtkGetMacro(KeepOverlays, bool);
   vtkBooleanMacro(KeepOverlays, bool);
 
//  Implementation note: when FileLowerLeft (gdcm2) is set to on the image is not flipped
// upside down as VTK would expect, use this option only if you know what you are doing.   
  // vtkSetMacro(FileLowerLeft, bool);
  // vtkGetMacro(FileLowerLeft, bool);
  // vtkBooleanMacro(FileLowerLeft, bool);
     
  vtkSetMacro(FlipY, bool);
  vtkGetMacro(FlipY, bool);
  vtkBooleanMacro(FlipY, bool);
     
   vtkGetObjectMacro(LookupTable, vtkLookupTable);

// FIXME : HOW to doxygen a VTK macro?
/*
 * \ brief Sets the LoadMode as a boolean string. 
 *        gdcm.LD_NOSEQ, gdcm.LD_NOSHADOW, gdcm.LD_NOSHADOWSEQ... 
 *        (nothing more, right now)
 *        WARNING : before using NO_SHADOW, be sure *all* your files
 *        contain accurate values in the 0x0000 element (if any) 
 *        of *each* Shadow Group. The parser will fail if the size is wrong !
 * @param   mode Load mode to be used    
 */
   vtkSetMacro(LoadMode, int);
   vtkGetMacro(LoadMode, int);
   vtkBooleanMacro(LoadMode, int);
 
/*
 * \ brief drop images with duplicate position  
 *         and therefore calculate ZSpacing for the whole file set
 * @param   mode user wants to drop images with duplicate position    
 */   
   vtkSetMacro(DropDuplicatePositions, bool);
   vtkGetMacro(DropDuplicatePositions, bool);
   vtkBooleanMacro(DropDuplicatePositions, bool);      

protected:
   vtkGdcmReader();
   ~vtkGdcmReader();

   virtual void ExecuteInformation();
   virtual void ExecuteData(vtkDataObject *output);

   //virtual void BuildData(vtkDataObject *output); // for VTK5.0
   virtual void BuildFileListFromPattern();
   virtual void LoadFileInformation();
   virtual void UpdateFileInformation();
   //BTX
   virtual void GetFileInformation(GDCM_NAME_SPACE::File *file);
   virtual bool TestFileInformation(GDCM_NAME_SPACE::File *file);
   //ETX

private:
   void RemoveAllInternalFileName(void);
   void AddInternalFileName(const char *name);
   void RemoveAllInternalFile(void);

   //BTX
   void IncrementProgress(const unsigned long updateProgressTarget,
                          unsigned long &updateProgressCount);
   /*void LoadImageInMemory(std::string fileName, unsigned char *dest,
                          const unsigned long updateProgressTarget,
                          unsigned long &updateProgressCount);*/

   void LoadImageInMemory(GDCM_NAME_SPACE::File *f, unsigned char *dest,
                          const unsigned long updateProgressTarget,
                          unsigned long &updateProgressCount);
   //ETX

// Variables
   //BTX
   typedef std::vector<GDCM_NAME_SPACE::File *> gdcmFileList;
   //ETX

   vtkLookupTable *LookupTable;
   vtkTimeStamp fileTime;

   bool AllowLookupTable;
   bool AllowLightChecking;

   //BTX
   // Number of columns of the image/volume to be loaded
   int NumColumns;
   // Number of lines of the image/volume to be loaded
   int NumLines;
   // Number of lines of the image/volume to be loaded
   int NumPlanes;
   // Total number of planes (or images) of the stack to be build.
   int TotalNumberOfPlanes;
   // Number of scalar components of the image to be loaded (1=monochrome 3=rgb)
   int NumComponents;
   // Type of the image[s]: 8/16/32 bits, signed/unsigned:
   std::string ImageType;
   // Pixel size (in number of bytes):
   size_t PixelSize;
   // List of filenames to be read in order to build a stack of images
   // or volume. The order in the list shall be the order of the images.
   std::list<std::string> FileNameList;
   gdcmFileList *CoherentFileList;
   bool OwnFile;

   // List of filenames created in ExecuteInformation and used in
   // ExecuteData.
   // If FileNameList isn't empty, InternalFileNameList is a copy of
   //    FileNameList
   // Otherwise, InternalFileNameList correspond to the list of 
   //    files patterned
   std::list<std::string> InternalFileNameList;
   gdcmFileList InternalFileList;
   //bool Execution;  // For VTK5.0
  
   //ETX

   /// \brief Bit string integer (each one considered as a boolean)
   ///        Bit 0 : Skip Sequences,    if possible
   ///        Bit 1 : Skip Shadow Groups if possible
   ///        Bit 2 : Skip Sequences inside a Shadow Group, if possible
   ///        Probabely (?), some more to add
   int LoadMode;
    
   bool DropDuplicatePositions;
   
   bool KeepOverlays;
   
  // bool FileLowerLeft;
   bool FlipY;
   /// Pointer to a user suplied function to allow modification of pixel order
   VOID_FUNCTION_PUINT8_PFILE_POINTER UserFunction;

};

//-----------------------------------------------------------------------------
#endif

