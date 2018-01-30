/*=========================================================================
                                                                                
  Program:   gdcm
  Module:    $RCSfile: vtkGdcmWriter.cxx,v $
  Language:  C++
  Date:      $Date: 2007/12/13 15:16:19 $
  Version:   $Revision: 1.36 $
                                                                                
  Copyright (c) CREATIS (Centre de Recherche et d'Applications en Traitement de
  l'Image). All rights reserved. See Doc/License.txt or
  http://www.creatis.insa-lyon.fr/Public/Gdcm/License.html for details.
                                                                                
     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.
                                                                                
=========================================================================*/
                                                                                
#include "gdcmFile.h"
#include "gdcmFileHelper.h"
#include "gdcmDebug.h"
#include "gdcmUtil.h"
#include "vtkGdcmWriter.h"

#include <vtkObjectFactory.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkLookupTable.h>
#if (VTK_MAJOR_VERSION >= 5)
#include <vtkMedicalImageProperties.h>
#endif
#ifndef vtkFloatingPointType
#define vtkFloatingPointType float
#endif

vtkCxxRevisionMacro(vtkGdcmWriter, "$Revision: 1.36 $")
vtkStandardNewMacro(vtkGdcmWriter)

vtkCxxSetObjectMacro(vtkGdcmWriter,LookupTable,vtkLookupTable)
#if (VTK_MAJOR_VERSION >= 5)
vtkCxxSetObjectMacro(vtkGdcmWriter,MedicalImageProperties,vtkMedicalImageProperties)
#endif
//-----------------------------------------------------------------------------
// Constructor / Destructor
vtkGdcmWriter::vtkGdcmWriter()
{
   this->LookupTable = NULL;
   this->MedicalImageProperties = NULL;
   this->FileDimensionality = 3;
   this->WriteType = VTK_GDCM_WRITE_TYPE_EXPLICIT_VR;
   this->GdcmFile = 0;
   this->ContentType = VTK_GDCM_WRITE_TYPE_USER_OWN_IMAGE;
}

vtkGdcmWriter::~vtkGdcmWriter()
{
   this->SetMedicalImageProperties(NULL);
   this->SetLookupTable(NULL);
}

//-----------------------------------------------------------------------------
// Print
void vtkGdcmWriter::PrintSelf(ostream &os, vtkIndent indent)
{
   this->Superclass::PrintSelf(os, indent);

   os << indent << "Write type : " << this->GetWriteTypeAsString();
}

//-----------------------------------------------------------------------------
// Public
const char *vtkGdcmWriter::GetWriteTypeAsString()
{
   switch(WriteType)
   {
      case VTK_GDCM_WRITE_TYPE_EXPLICIT_VR :
         return "Explicit VR";
      case VTK_GDCM_WRITE_TYPE_IMPLICIT_VR :
         return "Implicit VR";
      case VTK_GDCM_WRITE_TYPE_ACR :
         return "ACR";
      case VTK_GDCM_WRITE_TYPE_ACR_LIBIDO :
         return "ACR Libido";
      default :
         return "Unknow type";
   }
}

//-----------------------------------------------------------------------------
// Protected
/**
 * Copy the image and reverse the Y axis
 */
// The output data must be deleted by the user of the method !!!
size_t ReverseData(vtkImageData *image,unsigned char **data)
{
#if (VTK_MAJOR_VERSION >= 5)
   vtkIdType inc[3];
#else
   int inc[3];
#endif
   int *extent = image->GetUpdateExtent();
   int dim[3] = {extent[1]-extent[0]+1,
                 extent[3]-extent[2]+1,
                 extent[5]-extent[4]+1};

   size_t lineSize = dim[0] * image->GetScalarSize()
                   * image->GetNumberOfScalarComponents();
   size_t planeSize = dim[1] * lineSize;
   size_t size = dim[2] * planeSize;

   if( size>0 )
   {
      *data = new unsigned char[size];

      image->GetIncrements(inc);
      unsigned char *src = (unsigned char *)image->GetScalarPointerForExtent(extent);
      unsigned char *dst = *data + planeSize - lineSize;
      for (int plane = extent[4]; plane <= extent[5]; plane++)
      {
         for (int line = extent[2]; line <= extent[3]; line++)
         {
            // Copy one line at proper destination:
            memcpy((void*)dst, (void*)src, lineSize);

            src += inc[1] * image->GetScalarSize();
            dst -= lineSize;
         }
         dst += 2 * planeSize;
      }
   }
   else
   {
      *data = NULL;
   }

   return size;
}

/**
 * Set the medical informations in the file, based on the user passed
 * vtkMedicalImageProperties
 */
#if (VTK_MAJOR_VERSION >= 5)
void SetMedicalImageInformation(GDCM_NAME_SPACE::FileHelper *file, vtkMedicalImageProperties *medprop)
{
   // For now only do:
   // PatientName, PatientID, PatientAge, PatientSex, PatientBirthDate, StudyID
   std::ostringstream str;
   if( medprop )
     {
     if (medprop->GetPatientName())
        {
        str.str("");
        str << medprop->GetPatientName();
        file->InsertEntryString(str.str(),0x0010,0x0010,"PN"); // PN 1 Patient's Name
        }

     if (medprop->GetPatientID())
        {
        str.str("");
        str << medprop->GetPatientID();
        file->InsertEntryString(str.str(),0x0010,0x0020,"LO"); // LO 1 Patient ID
        }

     if (medprop->GetPatientAge())
        {
        str.str("");
        str << medprop->GetPatientAge();
        file->InsertEntryString(str.str(),0x0010,0x1010,"AS"); // AS 1 Patient's Age
        }

     if (medprop->GetPatientSex())
        {
        str.str("");
        str << medprop->GetPatientSex();
        file->InsertEntryString(str.str(),0x0010,0x0040,"CS"); // CS 1 Patient's Sex
        }

     if (medprop->GetPatientBirthDate())
        {
        str.str("");
        str << medprop->GetPatientBirthDate();
        file->InsertEntryString(str.str(),0x0010,0x0030,"DA"); // DA 1 Patient's Birth Date
        }

     if (medprop->GetStudyID())
        {
        str.str("");
        str << medprop->GetStudyID();
        file->InsertEntryString(str.str(),0x0020,0x0010,"SH"); // SH 1 Study ID
        }
     }
}
#endif

/**
 * Set the data informations in the file
 */
void SetImageInformation(GDCM_NAME_SPACE::FileHelper *file, vtkImageData *image)
{
   std::ostringstream str;

   // Image size
   int *extent = image->GetUpdateExtent();
   int dim[3] = {extent[1]-extent[0]+1,
                 extent[3]-extent[2]+1,
                 extent[5]-extent[4]+1};

   str.str("");
   str << dim[0];
   file->InsertEntryString(str.str(),0x0028,0x0011,"US"); // Columns

   str.str("");
   str << dim[1];
   file->InsertEntryString(str.str(),0x0028,0x0010,"US"); // Rows

   if(dim[2]>1)
   {
      str.str("");
      str << dim[2];
      //file->Insert(str.str(),0x0028,0x0012); // Planes
      file->InsertEntryString(str.str(),0x0028,0x0008,"IS"); // Number of Frames
   }

   // Pixel type
   str.str("");
   str << image->GetScalarSize()*8;
   file->InsertEntryString(str.str(),0x0028,0x0100,"US"); // Bits Allocated
   file->InsertEntryString(str.str(),0x0028,0x0101,"US"); // Bits Stored

   str.str("");
   str << image->GetScalarSize()*8-1;
   file->InsertEntryString(str.str(),0x0028,0x0102,"US"); // High Bit

   // Pixel Repr
   // FIXME : what do we do when the ScalarType is 
   // VTK_UNSIGNED_INT or VTK_UNSIGNED_LONG
   str.str("");
   if( image->GetScalarType() == VTK_UNSIGNED_CHAR  ||
       image->GetScalarType() == VTK_UNSIGNED_SHORT ||
       image->GetScalarType() == VTK_UNSIGNED_INT   ||
       image->GetScalarType() == VTK_UNSIGNED_LONG )
   {
      str << "0"; // Unsigned
   }
   else
   {
      str << "1"; // Signed
   }
   file->InsertEntryString(str.str(),0x0028,0x0103,"US"); // Pixel Representation

   // Samples per pixel
   str.str("");
   str << image->GetNumberOfScalarComponents();
   file->InsertEntryString(str.str(),0x0028,0x0002,"US"); // Samples per Pixel

   /// \todo : Spacing Between Slices is meaningfull ONLY for CT an MR modality
   ///       We should perform some checkings before forcing the Entry creation

   // Spacing
   vtkFloatingPointType *sp = image->GetSpacing();

   str.str("");
   // We are about to enter floating point value. 
   // By default ostringstream are smart and don't do fixed point
   // thus forcing to fixed point value
   str.setf( std::ios::fixed );
   str << sp[1] << "\\" << sp[0];
   file->InsertEntryString(str.str(),0x0028,0x0030,"DS"); // Pixel Spacing
   str.str("");
   str << sp[2];
   file->InsertEntryString(str.str(),0x0018,0x0088,"DS"); // Spacing Between Slices

   // Origin
   vtkFloatingPointType *org = image->GetOrigin();

   /// \todo : Image Position Patient is meaningfull ONLY for CT an MR modality
   ///       We should perform some checkings before forcing the Entry creation

   str.str("");
   str << org[0] << "\\" << org[1] << "\\" << org[2];
   file->InsertEntryString(str.str(),0x0020,0x0032,"DS"); // Image Position Patient
   str.unsetf( std::ios::fixed ); //done with floating point values

   // Window / Level
   vtkFloatingPointType *rng = image->GetScalarRange();

   str.str("");
   str << rng[1]-rng[0];
   file->InsertEntryString(str.str(),0x0028,0x1051,"DS"); // Window Width
   str.str("");
   str << (rng[1]+rng[0])/2.0;
   file->InsertEntryString(str.str(),0x0028,0x1050,"DS"); // Window Center

   // Pixels
   unsigned char *data;
   size_t size = ReverseData(image,&data);
   file->SetUserData(data,size);
}

/**
 * Write of the files
 * The call to this method is recursive if there is some files to write
 */ 
void vtkGdcmWriter::RecursiveWrite(int axis, vtkImageData *image, 
                    ofstream *file)
{
   if(file)
   {
      vtkErrorMacro( <<  "File must not be open");
      return;
   }

   if( image->GetScalarType() == VTK_FLOAT || 
       image->GetScalarType() == VTK_DOUBLE )
   {
      vtkErrorMacro(<< "Bad input type. Scalar type must not be of type "
                    << "VTK_FLOAT or VTK_DOUBLE (found:"
                    << image->GetScalarTypeAsString() << ")" );
      return;
   }

   RecursiveWrite(axis,image, image, file);
   //WriteDcmFile(this->FileName,image);
}

void vtkGdcmWriter::RecursiveWrite(int axis, vtkImageData *cache, 
                                   vtkImageData *image, ofstream *file)
{
   int idx, min, max;

   // if the file is already open then just write to it
   if( file )
   {
      vtkErrorMacro( <<  "File musn't be open");
      return;
   }

   // if we need to open another slice, do it
   if( (axis + 1) == this->FileDimensionality )
   {
      // determine the name
      if (this->FileName)
      {
         sprintf(this->InternalFileName, "%s", this->FileName);
      }
      else 
      {
         if (this->FilePrefix)
         {
            sprintf(this->InternalFileName, this->FilePattern, 
            this->FilePrefix, this->FileNumber);
         }
         else
         {
            sprintf(this->InternalFileName, this->FilePattern,this->FileNumber);
         }
// Remove this code in case user is using VTK 4.2...
#if !(VTK_MAJOR_VERSION == 4 && VTK_MINOR_VERSION == 2)
         if (this->FileNumber < this->MinimumFileNumber)
         {
            this->MinimumFileNumber = this->FileNumber;
         }
         else if (this->FileNumber > this->MaximumFileNumber)
         {
            this->MaximumFileNumber = this->FileNumber;
         }
#endif
      }

      // Write the file
      WriteDcmFile(this->InternalFileName,image);
      ++this->FileNumber;
      return;
   }

   // if the current region is too high a dimension for the file
   // the we will split the current axis
   cache->GetAxisUpdateExtent(axis, min, max);

   // if it is the y axis then flip by default
   if (axis == 1 && !this->FileLowerLeft)
   {
      for(idx = max; idx >= min; idx--)
      {
         cache->SetAxisUpdateExtent(axis, idx, idx);
         this->RecursiveWrite(axis - 1, cache, image, file);
      }
   }
   else
   {
      for(idx = min; idx <= max; idx++)
      {
         cache->SetAxisUpdateExtent(axis, idx, idx);
         this->RecursiveWrite(axis - 1, cache, image, file);
      }
   }

   // restore original extent
   cache->SetAxisUpdateExtent(axis, min, max);
}

void vtkGdcmWriter::WriteDcmFile(char *fileName, vtkImageData *image)
{
   GDCM_NAME_SPACE::FileHelper *dcmFile;
   if ( GdcmFile != 0)
      dcmFile = GDCM_NAME_SPACE::FileHelper::New(GdcmFile);
   else
      dcmFile = GDCM_NAME_SPACE::FileHelper::New();
   
   // From here, the write of the file begins

   // Set the medical informations:
#if (VTK_MAJOR_VERSION >= 5)  
   SetMedicalImageInformation(dcmFile, this->MedicalImageProperties);
#endif
      
   // Set the image informations
   SetImageInformation(dcmFile, image);

   // Write the image
   switch(this->WriteType)
   {
      case VTK_GDCM_WRITE_TYPE_EXPLICIT_VR :
         dcmFile->SetWriteTypeToDcmExplVR();
         break;
      case VTK_GDCM_WRITE_TYPE_IMPLICIT_VR :
         dcmFile->SetWriteTypeToDcmImplVR();
         break;
      case VTK_GDCM_WRITE_TYPE_ACR :
         dcmFile->SetWriteTypeToAcr();
         break;
      case VTK_GDCM_WRITE_TYPE_ACR_LIBIDO :
         dcmFile->SetWriteTypeToAcrLibido();
         break;
      default :
         dcmFile->SetWriteTypeToDcmExplVR();
   }
  
   dcmFile->SetContentType((GDCM_NAME_SPACE::ImageContentType)ContentType);
 
   if(!dcmFile->Write(fileName))
   {
      vtkErrorMacro( << "File "  <<  this->FileName  <<  "cannot be written by "
                     << " the gdcm library");
   }
   // Clean up
   if( dcmFile->GetUserData() && dcmFile->GetUserDataSize()>0 )
   {
      delete[] dcmFile->GetUserData();
   }
   dcmFile->Delete();
}

//-----------------------------------------------------------------------------
// Private

//-----------------------------------------------------------------------------
