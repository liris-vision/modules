#include "activecontour_main.h"
#include "activecontour.h"
#include "binaryregion.h"
#include <iostream>

void ExecActiveContour(const cv::Mat *pImgInput, cv::Mat *pImgOutput, const cv::Point *pCenter, const cv::Point *pRadii, const ActiveContour_OpenParams &params)
{
	CDeformableModelBase *pDeformableModel;
	CCouple<float> pfEllipseCenter, pfEllipseRadii;
	int iIteration, iNbIterationsMax, iNbPassesPerIteration;
	bool bIsEvolving;

	enum {MODELTYPE_PARAMETRIC, MODELTYPE_IMPLICIT} iModelType;

	CBinaryRegionBase::InitNeighborhood();

    iModelType = MODELTYPE_IMPLICIT;

    // Allocate the deformable model to a grayscale or color instance depending on image format
	if (pImgInput->type()==CV_8UC1)
	{
		// Allocate the deformable model to CActiveContourGrayscale or CBinaryRegionGrayscale
		// depending on model type
		if (iModelType==MODELTYPE_PARAMETRIC)
			pDeformableModel = new CActiveContourGrayscale();
		else
			pDeformableModel = new CBinaryRegionGrayscale();
	}
	else if (pImgInput->type()==CV_8UC3)
	{
		// Allocate the deformable model to CActiveContourColor or CBinaryRegionColor
		// depending on model type
		if (iModelType==MODELTYPE_PARAMETRIC)
		{
			CActiveContourColor *pActiveContourColor;

			pDeformableModel = new CActiveContourColor();
			pActiveContourColor = (CActiveContourColor *)pDeformableModel;

			// Set parameters specific to the color binary region model
			pActiveContourColor->iColorSpace = params.colorSpace;

			if (pActiveContourColor->iColorSpace!=CDeformableModelBase::RGB)
				pActiveContourColor->bIgnoreBrightnessComponent = true;
			else
				pActiveContourColor->bIgnoreBrightnessComponent = false;
		}
		else {
			CBinaryRegionColor *pBinaryRegionColor;

			pDeformableModel = new CBinaryRegionColor();
			pBinaryRegionColor = (CBinaryRegionColor *)pDeformableModel;

			// Set parameters specific to the color active contour model
			pBinaryRegionColor->iColorSpace = params.colorSpace;

			if (pBinaryRegionColor->iColorSpace!=CDeformableModelBase::RGB)
				pBinaryRegionColor->bIgnoreBrightnessComponent = true;
			else
				pBinaryRegionColor->bIgnoreBrightnessComponent = false;
		}
	}
	else {
		cerr<<"ERROR: number of bits/pixel is not supported."<<endl;
		return;
	}

	// Set energy configuration
	pDeformableModel->iInsideMode = params.insideMode;
	pDeformableModel->iOutsideMode = params.outsideMode;

    if (pDeformableModel->iInsideMode!=CDeformableModelBase::REGION &&
        pDeformableModel->iInsideMode!=CDeformableModelBase::REGION_INIT &&
        pDeformableModel->iInsideMode!=CDeformableModelBase::BAND)
    {
        cout<<"WARNING: inconsistent mode for inner region energy. Setting to default REGION mode"<<endl;
        pDeformableModel->iInsideMode = CDeformableModelBase::REGION;
    }

    if (pDeformableModel->iOutsideMode!=CDeformableModelBase::REGION &&
        pDeformableModel->iOutsideMode!=CDeformableModelBase::BAND &&
        pDeformableModel->iOutsideMode!=CDeformableModelBase::LINE)
    {
        cout<<"WARNING: inconsistent value for outer region energy. Setting to default LINE mode"<<endl;
        pDeformableModel->iOutsideMode = CDeformableModelBase::LINE;
    }

	// To work properly, the second narrow band region energy (the local one)
	// related to equations (6) and (19) in [Mille09] often requires the bias to be enabled
	if (pDeformableModel->iOutsideMode==CDeformableModelBase::LINE)
	{
		pDeformableModel->bRegionEnergyBias = true;
		pDeformableModel->fWeightBalloon = -0.3f;
	}

	pDeformableModel->fWeightGradient = params.weightGradient;
	pDeformableModel->fWeightRegion = params.weightRegion;
	pDeformableModel->fWeightBalloon = params.weightBalloon;
	pDeformableModel->fStdDeviationGaussianFilter = params.stdDeviationGaussianSmooth;

	// Other parameters may be modified here, before attaching the model to the image
	// ...

	// Attach model to image data
	pDeformableModel->AttachImage(pImgInput);

	pfEllipseCenter.Set(pCenter->x, pCenter->y);
	pfEllipseRadii.Set(pRadii->x, pRadii->y);

	iNbIterationsMax = 300;
	iNbPassesPerIteration = 5;

	// Initialize the region as an ellipse with given center and radius
	pDeformableModel->InitEllipse(pfEllipseCenter, pfEllipseRadii.x, pfEllipseRadii.y);

	// Main loop
	bIsEvolving = true;
	for (iIteration=0; iIteration<iNbIterationsMax && bIsEvolving==true; iIteration++)
	{
		cout<<"Iteration "<<iIteration<<endl;

		// Evolve the model with a few passes of gradient descent
		bIsEvolving = pDeformableModel->EnergyGradientDescent(iNbPassesPerIteration);
	}

	if (bIsEvolving==false)
	    cout<<endl<<"Stopping criterion is met: deformable model has reached stability"<<endl;
    else if (iIteration>=iNbIterationsMax)
        cout<<endl<<"Maximum number of iterations is exceeded, but deformable model has not reached stability"<<endl;

    // Make binary mask of final segmentation
    pDeformableModel->MakeBinaryMask(*pImgOutput);

	// Destroy model
	delete pDeformableModel;
}
