#include "heads.h"

int main(){

	VTK_MODULE_INIT(vtkRenderingVolumeOpenGL);
	int choose;
	cin >> choose;
	if (choose == 1)GpuVolumeRendering();
	else CpuVolumeRendering();
	return 1;
}