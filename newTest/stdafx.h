// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
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



// TODO:  在此处引用程序需要的其他头文件
