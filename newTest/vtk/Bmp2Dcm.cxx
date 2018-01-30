/*=========================================================================
                                                                                
  Program:   gdcm
  Module:    $RCSfile: Bmp2Dcm.cxx,v $
  Language:  C++
  Date:      $Date: 2008/01/28 12:47:07 $
  Version:   $Revision: 1.4 $
                                                                                
  Copyright (c) CREATIS (Centre de Recherche et d'Applications en Traitement de
  l'Image). All rights reserved. See Doc/License.txt or
  http://www.creatis.insa-lyon.fr/Public/Gdcm/License.html for details.
                                                                                
     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.
                                                                                
=========================================================================*/

#include "gdcmArgMgr.h"
#include "gdcmDirList.h"
#include "gdcmDebug.h"
#include "gdcmUtil.h"

#include <vtkImageData.h>

#include <vtkBMPReader.h>
#include <vtkBMPWriter.h>
#include "vtkGdcmWriter.h"

#include <vtkImageExtractComponents.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

//---------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
   START_USAGE(usage)
   "\n Bmp2Dcm :\n                                                            ",  
   "usage: Bmp2Dcm {filein=inputFileName|dirin=inputDirectoryName}            ",
   "              [studyUID = ] [patName = ] [debug]                          ", 
   "                                                                          ",
   " inputFileName : Name of the (single) file user wants to transform        ",
   " inputDirectoryName : user wants to transform *all* the files             ",
   " studyUID   : *aware* user wants to add the serie                         ",
   "                                             to an already existing study ", 
   " verbose    : user wants to run the program in 'verbose mode'             ",       
   " debug      : *developper* wants to run the program in 'debug mode'       ",
   
   FINISH_USAGE

   // ----- Initialize Arguments Manager ------
  
   GDCM_NAME_SPACE::ArgMgr *am = new GDCM_NAME_SPACE::ArgMgr(argc, argv);
  
   if (am->ArgMgrDefined("usage") || argc == 1) 
   {
      am->ArgMgrUsage(usage); // Display 'usage'
      delete am;
      return 0;
   }

   if (am->ArgMgrDefined("debug"))
      GDCM_NAME_SPACE::Debug::DebugOn();

   bool verbose = ( 0 != am->ArgMgrDefined("verbose") );

   const char *fileName = am->ArgMgrGetString("filein");
   const char *dirName  = am->ArgMgrGetString("dirin");

   if ( (fileName == 0 && dirName == 0)
        ||
        (fileName != 0 && dirName != 0) )
   {
       std::cout <<std::endl
                 << "Either 'filein=' or 'dirin=' must be present;"
                 << std::endl << "Not both" << std::endl;
       am->ArgMgrUsage(usage); // Display 'usage'
       delete am;
       return 0;
 }

   std::string patName  = am->ArgMgrGetString("patname", dirName);
   bool userDefinedStudy = ( 0 != am->ArgMgrDefined("studyUID") );
   const char *studyUID;
   if (userDefinedStudy)
      studyUID  = am->ArgMgrGetString("studyUID");

   // not described *on purpose* in the Usage !
   bool userDefinedSerie = ( 0 != am->ArgMgrDefined("serieUID") );
   const char *serieUID;
   if(userDefinedSerie)
      serieUID = am->ArgMgrGetString("serieUID");

    /* if unused Param we give up */
   if ( am->ArgMgrPrintUnusedLabels() )
   {
      am->ArgMgrUsage(usage);
      delete am;
      return 0;
   }

   delete am;  // ------ we don't need Arguments Manager any longer ------


   // ----- Begin Processing -----

   int *dim;
   std::string nomFich;

   if ( fileName != 0 ) // ====== Deal with a single file ======
   {
     vtkBMPReader* Reader = vtkBMPReader::New();
     if ( Reader->CanReadFile(fileName ) == 0) {
         // skip 'non BMP' files
        Reader->Delete();
        if (verbose)
            std::cout << "Sorry, [" << fileName << "] is not a BMP file!" << std::endl;
        return 0;
    }

    if (verbose)
       std::cout << "deal with [" <<  fileName << "]" << std::endl;
     //Read BMP file

     Reader->SetFileName(fileName);
     Reader->Update();

     vtkImageExtractComponents* Red = vtkImageExtractComponents::New();
     Red->SetInput(Reader->GetOutput());
     Red->SetComponents(0);
     Red->Update();

     vtkGdcmWriter* Writer = vtkGdcmWriter::New();
     Writer->SetInput(Red->GetOutput());
     nomFich = "";
     nomFich = nomFich+fileName+".acr";   
     Writer->Write();
   
     Reader->Delete();
     Red->Delete();
     Writer->Delete();
   
     }
     else  // ====== Deal with a (single Patient) Directory ======
     { 
     
        if ( ! GDCM_NAME_SPACE::DirList::IsDirectory(dirName) )
        {
          std::cout << "KO : [" << dirName << "] is not a Directory." << std::endl;
          return 0;
        }
        else
        {
          if (verbose)
            std::cout << "OK : [" << dirName << "] is a Directory." << std::endl;
        } 
        std::string strStudyUID;
        std::string strSerieUID;

        if (userDefinedStudy)
           strSerieUID =  studyUID;
        else
           strStudyUID =  GDCM_NAME_SPACE::Util::CreateUniqueUID();
   
        if (userDefinedStudy)
          strSerieUID =  serieUID;
        else
           strStudyUID =  GDCM_NAME_SPACE::Util::CreateUniqueUID();    

       if(verbose)
           std::cout << "dirName [" << dirName << "]" << std::endl;
       
        GDCM_NAME_SPACE::DirList dirList(dirName,1); // gets recursively the file list
        GDCM_NAME_SPACE::DirListType fileList = dirList.GetFilenames();

        for( GDCM_NAME_SPACE::DirListType::iterator it  = fileList.begin();
                                   it != fileList.end();
                                   ++it )
        {
           if ( GDCM_NAME_SPACE::Util::GetName((*it)).c_str()[0] == '.' ) 
           {
              // skip hidden files
              continue;
           }
      
           vtkBMPReader* Reader = vtkBMPReader::New();
  
           if ( Reader->CanReadFile(it->c_str() ) == 0) {
              // skip 'non BMP' files  
               Reader->Delete();
               continue;
            }
   
           if (verbose)
             std::cout << "deal with [" <<  it->c_str() << "]" << std::endl;  
   
           Reader->SetFileName(it->c_str());    
           Reader->Update();
   
           dim=Reader->GetOutput()->GetDimensions();
           vtkImageExtractComponents* Red = vtkImageExtractComponents::New();
           Red->SetInput(Reader->GetOutput());
           Red->SetComponents(0);
           Red->Update();

           // At least, created files will look like a Serie from a single Study ...  
           GDCM_NAME_SPACE::File *f = GDCM_NAME_SPACE::File::New();
           f->InsertEntryString(strStudyUID, 0x0020, 0x000d, "UI");      
           f->InsertEntryString(strSerieUID, 0x0020, 0x000e, "UI");
           f->InsertEntryString(patName,     0x0010, 0x0010, "PN");   // Patient's Name 

           vtkGdcmWriter* Writer = vtkGdcmWriter::New();
           Writer->SetInput(Red->GetOutput());
           nomFich = "";
           nomFich = nomFich+it->c_str()+".acr";
           Writer->SetFileName(nomFich.c_str());
           Writer->SetGdcmFile(f);
           Writer->Write();
   
           f->Delete(); 
        
      }
   }   
   return 0;   
}
