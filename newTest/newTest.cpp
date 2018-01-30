// newTest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"




void CpuVolumeRendering()
{
	
	//定义绘制器aRenderer
	vtkRenderer *aRenderer = vtkRenderer::New();
	//定义绘制窗口renWin，并加入aRenderer
	vtkRenderWindow *renWin = vtkRenderWindow::New();
	renWin->AddRenderer(aRenderer);
	//定义窗口交互器iren，加入到renWin
	vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
	iren->SetRenderWindow(renWin);


	//DICOM读取
	//CT格式数据读取类
	vtkDICOMImageReader *reader = vtkDICOMImageReader::New();
	reader->SetDirectoryName("C:/Users/superfan/Desktop/Head");


	reader->SetDataScalarTypeToUnsignedShort();
	reader->SetDataByteOrderToLittleEndian();
	reader->Update();
	vtkImageShiftScale *shiftScale = vtkImageShiftScale::New();
	shiftScale->SetInputConnection(reader->GetOutputPort());
	shiftScale->SetOutputScalarTypeToUnsignedShort();//将数据转换为体绘制能处理的数据，后面代替reader


	vtkPiecewiseFunction*opacityTransferFunction = vtkPiecewiseFunction::New();//设置不透明度传递函数
	//opacityTransferFunction->AddPoint(-1024, 0.0);
	//opacityTransferFunction->AddPoint(1269, 0.17);
	//opacityTransferFunction->AddPoint(2365, 0.46);
	//opacityTransferFunction->AddPoint(3500, 0.0);
	//opacityTransferFunction->AddSegment(-1024, 0, 1269, 0.17);
	//opacityTransferFunction->AddSegment(1269, 0.17, 2365, 0.46);
	//opacityTransferFunction->AddSegment(2365, 0.46, 3500, 0);
	//网上不透明度设置
	opacityTransferFunction->AddPoint(-3024, 0, 0.5, 0.0);
	opacityTransferFunction->AddPoint(-220, 0, .49, .61);
	opacityTransferFunction->AddPoint(625, .71, .5, 0.0);
	opacityTransferFunction->AddPoint(3071, 0.0, 0.5, 0.0);
	
	vtkColorTransferFunction *colorTransferFunction = vtkColorTransferFunction::New();//设置颜色传递函数  0, 0, 0, 0 2150.0, 1.0, 0.0, 0.0 2800.0, 0.5, 0.77, 0.6 4095.0, 0.5, 0.9, 0.9
	//colorTransferFunction->AddRGBPoint(0, 0, 0, 0);//此处颜色设置为灰度值
	//colorTransferFunction->AddRGBPoint(2150.0, 1.0, 0.0, 0.0);
	//colorTransferFunction->AddRGBPoint(2800.0, 0.5, 0.77, 0.6);
	//colorTransferFunction->AddRGBPoint(4095.0, 0.5, 0.9, 0.9);

	colorTransferFunction->AddRGBPoint(-3024, 0, 0, 0, 0.5, 0.0);//网上
	colorTransferFunction->AddRGBPoint(-200, 0.73, 0.25, 0.30, 0.49, .61);
	colorTransferFunction->AddRGBPoint(641, .90, .82, .56, .5, 0.0);
	colorTransferFunction->AddRGBPoint(3071, 1, 1, 1, .5, 0.0);

	vtkPiecewiseFunction *gradientTransferFunction = vtkPiecewiseFunction::New();//设置梯度传递函数 0, 2.0 500, 2.0 1300, 0.1
	/*gradientTransferFunction->AddPoint(0, 2.0);
	gradientTransferFunction->AddPoint(500, 2.0);
	gradientTransferFunction->AddSegment(600, 0.73, 900, 0.9);
	gradientTransferFunction->AddPoint(1300, 0.1);*/

	gradientTransferFunction->AddPoint(0, 2.0);//网上
	gradientTransferFunction->AddPoint(500, 2.0);
	gradientTransferFunction->AddSegment(600, 0.73, 900, 0.9);
	gradientTransferFunction->AddPoint(1300, 0.1);
	vtkVolumeProperty *volumeProperty = vtkVolumeProperty::New();//定义并设置相关体属性
	volumeProperty->SetColor(colorTransferFunction);
	volumeProperty->SetScalarOpacity(opacityTransferFunction);
	volumeProperty->SetGradientOpacity(gradientTransferFunction);
	volumeProperty->ShadeOn();
	volumeProperty->SetAmbient(.5);
	volumeProperty->SetDiffuse(1.0);
	volumeProperty->SetSpecular(.5);
	volumeProperty->SetSpecularPower(25);
	volumeProperty->SetInterpolationTypeToLinear();
	vtkVolumeRayCastCompositeFunction*compositeRaycastFunction = vtkVolumeRayCastCompositeFunction::New();//定义光线投射方法为合成体绘制方法
	vtkVolumeRayCastMapper *volumeMapper = vtkVolumeRayCastMapper::New();
	//vtkGPUVolumeRayCastMapper *volumeMapper = vtkGPUVolumeRayCastMapper::New();
	volumeMapper->SetVolumeRayCastFunction(compositeRaycastFunction);//载入体绘制方法
	volumeMapper->SetInputConnection(shiftScale->GetOutputPort());
	volumeMapper->SetBlendModeToComposite();
	vtkVolume *volume = vtkVolume::New();//定义Volume
	volume->SetMapper(volumeMapper);
	volume->SetProperty(volumeProperty);//设置体属性
	aRenderer->AddVolume(volume); //将Volume装载到绘制类中
	aRenderer->SetBackground(0.0, 0.0, 0.0);
	renWin->SetSize(500, 500); //设置背景颜色和绘制窗口大小
	renWin->Render(); //窗口进行绘制
	iren->Initialize();
	iren->Start(); //初始化并进行交互绘制
	


	
}

