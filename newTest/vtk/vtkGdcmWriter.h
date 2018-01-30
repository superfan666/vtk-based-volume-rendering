/*=========================================================================
                                                                                
  Program:   gdcm
  Module:    $RCSfile: vtkGdcmWriter.h,v $
  Language:  C++
  Date:      $Date: 2007/06/21 14:47:16 $
  Version:   $Revision: 1.12 $
                                                                                
  Copyright (c) CREATIS (Centre de Recherche et d'Applications en Traitement de
  l'Image). All rights reserved. See Doc/License.txt or
  http://www.creatis.insa-lyon.fr/Public/Gdcm/License.html for details.
                                                                                
     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.
                                                                                
=========================================================================*/
                                                                                
#ifndef __vtkGdcmWriter_h
#define __vtkGdcmWriter_h

#include "gdcmCommon.h" // To avoid warnings concerning the std
#include "gdcmFile.h"   // for GDCM_NAME_SPACE::File

#include <vtkImageWriter.h>
#include <vtkLookupTable.h>
#include <string>

//-----------------------------------------------------------------------------

#define VTK_GDCM_WRITE_TYPE_EXPLICIT_VR 1
#define VTK_GDCM_WRITE_TYPE_IMPLICIT_VR 2
#define VTK_GDCM_WRITE_TYPE_ACR         3
#define VTK_GDCM_WRITE_TYPE_ACR_LIBIDO  4

#define VTK_GDCM_WRITE_TYPE_USER_OWN_IMAGE          1
#define VTK_GDCM_WRITE_TYPE_FILTERED_IMAGE          2
#define VTK_GDCM_WRITE_TYPE_CREATED_IMAGE           3
#define VTK_GDCM_WRITE_TYPE_UNMODIFIED_PIXELS_IMAGE 4

//-----------------------------------------------------------------------------
class vtkLookupTable;
class vtkMedicalImageProperties;

class VTK_EXPORT vtkGdcmWriter : public vtkImageWriter
{
public:
   static vtkGdcmWriter *New();
   vtkTypeRevisionMacro(vtkGdcmWriter, vtkImageWriter);

   void PrintSelf(ostream &os, vtkIndent indent);

   //vtkSetObjectMacro(LookupTable, vtkLookupTable);
   virtual void SetLookupTable(vtkLookupTable*);
   vtkGetObjectMacro(LookupTable, vtkLookupTable);

   void SetWriteTypeToDcmImplVR(){SetWriteType(VTK_GDCM_WRITE_TYPE_EXPLICIT_VR);}
   void SetWriteTypeToDcmExplVR(){SetWriteType(VTK_GDCM_WRITE_TYPE_IMPLICIT_VR);}
   void SetWriteTypeToAcr()      {SetWriteType(VTK_GDCM_WRITE_TYPE_ACR);        }
   void SetWriteTypeToAcrLibido(){SetWriteType(VTK_GDCM_WRITE_TYPE_ACR_LIBIDO); }
   

   // gdcm cannot guess how user built his image (and therefore cannot be clever about some Dicom fields)
   // It's up to the user to tell gdcm what he did. 
   // -1) user created ex nihilo his own image and wants to write it as a Dicom image.
   // USER_OWN_IMAGE
   // -2) user modified the pixels of an existing image.
   // FILTERED_IMAGE
   // -3) user created a new image, using existing a set of images (eg MIP, MPR, cartography image)
   //  CREATED_IMAGE
   // -4) user modified/added some tags *without processing* the pixels (anonymization..
   //  UNMODIFIED_PIXELS_IMAGE 
   // -Probabely some more to be added 
   //(see gdcmFileHelper.h for more explanations)
   
   void SetContentTypeToUserOwnImage()         {SetContentType(VTK_GDCM_WRITE_TYPE_USER_OWN_IMAGE);}   
   void SetContentTypeToFilteredImage()        {SetContentType(VTK_GDCM_WRITE_TYPE_FILTERED_IMAGE);}   
   void SetContentTypeToUserCreatedImage()     {SetContentType(VTK_GDCM_WRITE_TYPE_CREATED_IMAGE);}   
   void SetContentTypeToUnmodifiedPixelsImage(){SetContentType(VTK_GDCM_WRITE_TYPE_UNMODIFIED_PIXELS_IMAGE);}   
   
   vtkSetMacro(WriteType, int);
   vtkGetMacro(WriteType, int);
   const char *GetWriteTypeAsString();


//BTX
   // Description:
   // Aware user is allowed to pass his own GDCM_NAME_SPACE::File *, so he may set *any Dicom field* he wants.
   // (including his own Shadow Elements, or any GDCM_NAME_SPACE::SeqEntry)
   // GDCM_NAME_SPACE::FileHelper::CheckMandatoryElements() will check inconsistencies, as far as it knows how.
   // Sorry, not yet available under Python.
   vtkSetMacro(GdcmFile, GDCM_NAME_SPACE::File *);
   vtkGetMacro(GdcmFile, GDCM_NAME_SPACE::File *);
//ETX

   vtkSetMacro(ContentType, int);
   vtkGetMacro(ContentType, int);

   // Description:
   // To pass in some extra information from a VTK context a user can pass a
   // vtkMedicalImageProperties object
#if (VTK_MAJOR_VERSION >= 5)   
   void SetMedicalImageProperties(vtkMedicalImageProperties*);
#else
   void SetMedicalImageProperties(vtkMedicalImageProperties*) {}
#endif
        
protected:
   vtkGdcmWriter();
   ~vtkGdcmWriter();

  virtual void RecursiveWrite(int axis, vtkImageData *image, ofstream *file);
  virtual void RecursiveWrite(int axis, vtkImageData *image, 
                              vtkImageData *cache, ofstream *file);
  void WriteDcmFile(char *fileName, vtkImageData *image);

private:
// Variables
   vtkLookupTable *LookupTable;
   vtkMedicalImageProperties *MedicalImageProperties;   
   int WriteType;
//BTX
   GDCM_NAME_SPACE::File *GdcmFile;
//ETX
   int ContentType;
   
};

//-----------------------------------------------------------------------------
#endif
