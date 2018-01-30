// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <vtkSmartPointer.h> 
#include <vtkRenderer.h> 
#include <vtkRenderWindow.h> 
#include <vtkRenderWindowInteractor.h> 
#include <vtkVolume16Reader.h> 
#include <vtkVolume.h> 
#include <vtkVolumeRayCastMapper.h> 
#include <vtkGPUVolumeRayCastMapper.h> 
#include <vtkVolumeRayCastCompositeFunction.h> 
#include <vtkVolumeProperty.h> 
#include <vtkColorTransferFunction.h> 
#include <vtkPiecewiseFunction.h> 
#include <vtkCamera.h> 
#include <vtkDICOMImageReader.h> 
#include <vtkImageShiftScale.h> 
#include<vtkDICOMImageReader.h>
#include<vtkImageShiftScale.h>

#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL);
VTK_MODULE_INIT(vtkRenderingFreeType);
VTK_MODULE_INIT(vtkInteractionStyle);

void GpuVolumeRendering();
void CpuVolumeRendering();



// TODO:  �ڴ˴����ó�����Ҫ������ͷ�ļ�
