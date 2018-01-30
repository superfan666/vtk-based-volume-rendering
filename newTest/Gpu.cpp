#include "stdafx.h"
/*#include "vtkImageCast.h"
#include "vtkMetaImageWriter.h"
#include "vtkDICOMImageReader.h"
#include "vtkImageViewer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkImageData.h"
#include "vtkRenderer.h"
*/

void GpuVolumeRendering()
{
 vtkSmartPointer<vtkDICOMImageReader>reader = vtkSmartPointer<vtkDICOMImageReader>::New();
 reader->SetDirectoryName("C:/Users/superfan/Desktop/Head");
 reader->SetDataScalarTypeToUnsignedShort();
 reader->SetDataSpacing(1, 1, 1);
 reader->SetDataByteOrderToLittleEndian();

/*vtkImageCast *readerImageCast = vtkImageCast::New();
readerImageCast->SetInputConnection(reader->GetOutputPort());
readerImageCast->SetOutputScalarTypeToUnsignedChar();
readerImageCast->ClampOverflowOn();


vtkImageWriter *pvTemp200 = vtkImageWriter::New();
pvTemp200->SetInputConnection(readerImageCast->GetOutputPort());
pvTemp200->SetFileName("C:/Users/superfan/Desktop/head.raw");
pvTemp200->Write();

pvTemp200->Delete();
readerImageCast->Delete();*/


//图像数据预处理。常见两种操作。一种是类型转换，通过 vtkImageShiftScale 将不同类型的数据集转换为 VTK 可以处理的数据；  
//另一种是剔除冗余数据，通过 vtkStripper 放置无效的旧单元的存在，提高绘制速度。  
vtkImageShiftScale*shiftScale = vtkImageShiftScale::New();
shiftScale->SetInputConnection(reader->GetOutputPort());
//shiftScale->SetOutputScalarTypeToUnsignedChar();

//reader->SetDataByteOrderToLittleEndian();  
//reader->SetDataScalarTypeToUnsignedShort();  


// The volume will be displayed by ray-cast alpha compositing.  
// A ray-cast mapper is needed to do the ray-casting, and a  
// compositing function is needed to do the compositing along the ray.  
vtkSmartPointer<vtkVolumeRayCastCompositeFunction>rayCastFunction =
vtkSmartPointer<vtkVolumeRayCastCompositeFunction>::New();



//vtkSmartPointer<vtkVolumeRayCastMapper> volumeMapper = vtkSmartPointer<vtkVolumeRayCastMapper>::New();  
vtkSmartPointer<vtkGPUVolumeRayCastMapper>volumeMapper = vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
//volumeMapper->SetInputConnection(v16->GetOutputPort());  
//volumeMapper->SetInputConnection(reader->GetOutputPort());  
volumeMapper->SetInputConnection(shiftScale->GetOutputPort());

//volumeMapper->SetVolumeRayCastFunction(rayCastFunction);// 如果不用GPU绘制，就使用这句话  
volumeMapper->SetBlendModeToComposite();// GPU的绘制不能SetVolumeRayCastFunction()  
//volumeMapper->SetSampleDistance(0.1);
//[cpp] view plain copy


// The color transfer function maps voxel intensities to colors.  
// It is modality-specific, and often anatomy-specific as well.  
// The goal is to one color for flesh (between 500 and 1000)  
// and another color for bone (1150 and over).  
vtkSmartPointer<vtkColorTransferFunction>volumeColor =
vtkSmartPointer<vtkColorTransferFunction>::New();
/*volumeColor->AddRGBPoint(-3024, 0.0, 0.0, 0.0);
volumeColor->AddRGBPoint(200, 0.72, 0.25, 0.30);
volumeColor->AddRGBPoint(400, 1.0, 1.0, 1.0);
volumeColor->AddRGBPoint(500, 1.0, 1.0, 1.0);
volumeColor->AddRGBPoint(510, 1.0, 1.0, 1.0);
volumeColor->AddRGBPoint(3071, 1.0, 1.0, 1.0); */
/*volumeColor->AddRGBPoint(-3024, 0, 0, 0);
volumeColor->AddRGBPoint(-200, 0.042, 0.73, 0.55);
volumeColor->AddRGBPoint(641, 0.088, 0.67, 0.88);
//volumeColor->AddRGBPoint(1333, 1, 1, 1);
volumeColor->AddRGBPoint(3074, 0.75, 0.34, 1);
//volumeColor->AddRGBPoint(220.0, 0.20, 0.20, 0.20);*/

/*volumeColor->AddRGBPoint(-3024, 0, 0, 0, 0.5, 0.0);//cpu同等参数
volumeColor->AddRGBPoint(-200, 0.73, 0.25, 0.30, 0.49, .61);
volumeColor->AddRGBPoint(641, .90, .82, .56, .5, 0.0);
volumeColor->AddRGBPoint(3071, 1, 1, 1, .5, 0.0);*/

volumeColor->AddRGBPoint(-3024, 0.00, 0.00, 0.00);   //参数2
volumeColor->AddRGBPoint(-16.4, 0.72, 0.25, 0.30);
volumeColor->AddRGBPoint(641.38, 0.91, 0.81, 0.55);
volumeColor->AddRGBPoint(3071, 1, 1, 1);

// The opacity transfer function is used to control the opacity  
// of different tissue types.  
vtkSmartPointer<vtkPiecewiseFunction>volumeScalarOpacity =
vtkSmartPointer<vtkPiecewiseFunction>::New();
/*volumeScalarOpacity->AddPoint(-3024,  0);
volumeScalarOpacity->AddPoint(-200, 0);
volumeScalarOpacity->AddPoint(220, 0);
volumeScalarOpacity->AddPoint(233,0.65);
volumeScalarOpacity->AddPoint(1500, 0.833);*/

/*volumeScalarOpacity->AddPoint(-3024, 0, 0.5, 0.0);    cpu同等参数
volumeScalarOpacity->AddPoint(-220, 0, .49, .61);
volumeScalarOpacity->AddPoint(625, .71, .5, 0.0);
volumeScalarOpacity->AddPoint(3071, 0.0, 0.5, 0.0);*/ 


volumeScalarOpacity->AddPoint(-3024, 0.0);  //参数2
volumeScalarOpacity->AddPoint(-16.4, 0.0);
volumeScalarOpacity->AddPoint(641.38, 0.72);
volumeScalarOpacity->AddPoint(3071, 0.71);

//volumeScalarOpacity->AddPoint(300, 1);
//volumeScalarOpacity->AddPoint(400, 1);





// The gradient opacity function is used to decrease the opacity  
// in the "flat" regions of the volume while maintaining the opacity  
// at the boundaries between tissue types.  The gradient is measured  
// as the amount by which the intensity changes over unit distance.  
// For most medical data, the unit distance is 1mm.  
vtkSmartPointer<vtkPiecewiseFunction>volumeGradientOpacity =   //0, 2.0 500, 2.0 1300, 0.1
vtkSmartPointer<vtkPiecewiseFunction>::New();
volumeGradientOpacity->AddPoint(0, 1);
volumeGradientOpacity->AddPoint(255, 1);

/*volumeGradientOpacity->AddPoint(0, 2.0);//cpu同等参数
volumeGradientOpacity->AddPoint(500, 2.0);
volumeGradientOpacity->AddSegment(600, 0.73, 900, 0.9);
volumeGradientOpacity->AddPoint(1300, 0.1);*/



vtkSmartPointer<vtkVolumeProperty>volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
volumeProperty->SetColor(volumeColor);                 //颜色传递函数
volumeProperty->SetScalarOpacity(volumeScalarOpacity); //灰度不透明度传递函数
volumeProperty->SetGradientOpacity(volumeGradientOpacity);//梯度传递函数
volumeProperty->ShadeOn();
volumeProperty->SetAmbient(0.1);
volumeProperty->SetDiffuse(0.9);
volumeProperty->SetSpecular(0.2);
volumeProperty->SetSpecularPower(11.0);
volumeProperty->SetInterpolationTypeToLinear();



// The vtkVolume is a vtkProp3D (like a vtkActor) and controls the position  
// and orientation of the volume in world coordinates.  
vtkSmartPointer<vtkVolume>volume = vtkSmartPointer<vtkVolume>::New();
volume->SetMapper(volumeMapper);
volume->SetProperty(volumeProperty);    
//[cpp] view plain copy



// Create the renderer, the render window, and the interactor. The renderer  
// draws into the render window, the interactor enables mouse- and  
// keyboard-based interaction with the scene.  
vtkSmartPointer<vtkRenderer>ren = vtkSmartPointer<vtkRenderer>::New();
vtkSmartPointer<vtkRenderWindow>renWin = vtkSmartPointer<vtkRenderWindow>::New();
if (NULL == renWin)
{
	std::cout << "error" << std::endl;
	//return 0;  
}
renWin->AddRenderer(ren);
vtkSmartPointer<vtkRenderWindowInteractor>iren =
vtkSmartPointer<vtkRenderWindowInteractor>::New();
iren->SetRenderWindow(renWin);



// Finally, add the volume to the renderer  
ren->AddViewProp(volume);



// Set up an initial view of the volume.  The focal point will be the  
// center of the volume, and the camera position will be 400mm to the  
// patient's left (which is our right).  
vtkCamera*camera = ren->GetActiveCamera();
double*c = volume->GetCenter();
camera->SetFocalPoint(c[0], c[1], c[2]);
camera->SetPosition(c[0] + 400, c[1], c[2]);
camera->SetViewUp(0, 0, -1);
//[cpp] view plain copy
ren->SetBackground(0.0, 0.0, 0.0);

// Increase the size of the render window  
renWin->SetSize(640, 480);
//[cpp] view plain copy


// Interact with the data.  
iren->Initialize();
iren->Start();
//[cpp] view plain copy



}