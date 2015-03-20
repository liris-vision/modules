/*
	activecontour.cpp

	Copyright 2010 Julien Mille (julien.mille@liris.cnrs.fr)
	http://liris.cnrs.fr/~jmille/code.html

	Source file of library implementing the active contour model with narrow band
	region energy described in

	[Mille09] J. Mille, "Narrow band region-based active contours and surfaces for
	          2D and 3D segmentation", Computer Vision and Image Understanding,
			  volume 113, issue 9, pages 946-965, 2009

	A preprint version of this paper may be downloaded at
	http://liris.cnrs.fr/~jmille/doc/cviu09temp.pdf

	If you use this code for research purposes, please cite the aforementioned paper
	in any resulting publication.

	The current source file corresponds to the explicit (parametric) implementation
	of the active contour model.
	See sections 2 and 5 in [Mille09] for the continuous variational model and its
	explicit implementation.
*/

#include "activecontour.h"
#include "cvmat2array.h"
#include <iostream>

/**********************************************************************
*                      CActiveContourBase                             *
**********************************************************************/

CActiveContourBase::CActiveContourBase():CDeformableModelBase()
{
	// Enable the precomputation of image integrals
	bUseImageIntegrals = true;

	// Sampling parameters
	fSampling = 1.0f;
	bResample = true;

	// Display parameters
	bDisplayEdges = true;
	bDisplayVertices = false;
	bDisplayParallelCurves = true;

	rgbEdgeColor.Set(255, 0, 0);
	rgbVertexColor.Set(0, 0, 255);
	rgbEdgeParallelColor.Set(127,127,0);
	iVertexWidth = 2;

	// Narrow band areas
	fInnerNarrowBandArea = 0.0f;
	fOuterNarrowBandArea = 0.0f;
}

void CActiveContourBase::Resample()
{
	list<CVertex2D>::iterator itVertex, itVertexNext;
	CVertex2D vertexNew;
	unsigned int i;
	float dmin2, dmax2;

	// Compute min and max bounds for allowed edge lengths
	dmin2 = fSampling*fSampling;
	dmax2 = 4.0f*dmin2;

	if (listVertices.size()<2)
		return;

	// Merge vertices
	i=0;
	itVertex = listVertices.begin();
	itVertexNext = itVertex;
	itVertexNext++;

	while (i<listVertices.size()-1)
	{
		while (i<listVertices.size()-1 && (itVertex->pfPos-itVertexNext->pfPos).L2Norm2()<dmin2)
		{
			itVertex->pfPos = (itVertex->pfPos + itVertexNext->pfPos)*0.5f;
			itVertex->vfNormal = (itVertex->vfNormal + itVertexNext->vfNormal)*0.5f;

			listVertices.erase(itVertexNext);

			itVertexNext = itVertex;
			if (i<listVertices.size()-1)
				itVertexNext++;
		}
		if (i<listVertices.size()-1)
		{
			itVertex++;
			itVertexNext++;
			i++;
		}
	}

	// Add vertices
	i = 0;
	itVertex = listVertices.begin();
	itVertexNext = itVertex;
	itVertexNext++;

	while (i<listVertices.size()-1)
	{
		while ((itVertex->pfPos-itVertexNext->pfPos).L2Norm2()>dmax2)
		{
			vertexNew.pfPos = (itVertex->pfPos + itVertexNext->pfPos)*0.5f;
			vertexNew.vfNormal = (itVertex->vfNormal + itVertexNext->vfNormal)*0.5f;

			listVertices.insert(itVertexNext, vertexNew);

			itVertexNext = itVertex;
			itVertexNext++;
		}
		itVertex++;
		itVertexNext++;
		i++;
	}
}

void CActiveContourBase::Translate(const CCouple<float> &fTranslate)
{
	list<CVertex2D>::iterator itVertex;

	for (itVertex=listVertices.begin(); itVertex!=listVertices.end(); itVertex++)
		itVertex->pfPos += fTranslate;
}

void CActiveContourBase::Rotate(float angle)
{
	list<CVertex2D>::iterator itVertex;
	CCouple<float> pfCentroid, pfCentered;
	float cs, ss;

	cs = cos(angle);
	ss = sin(angle);

	pfCentroid = GetCentroid();

	for (itVertex=listVertices.begin(); itVertex!=listVertices.end(); itVertex++)
	{
		pfCentered = itVertex->pfPos - pfCentroid;
		itVertex->pfPos.x = pfCentroid.x + cs*pfCentered.x - ss*pfCentered.y;
		itVertex->pfPos.y = pfCentroid.y + ss*pfCentered.x + cs*pfCentered.y;
	}
}

bool CActiveContourBase::EnergyGradientDescent(int iNbPasses)
{
	list<CVertex2D>::iterator itVertex, itVertexPrev, itVertexNext;
	int i, iPass, iNbVertices;
	CCouple<float> vfForceSmoothness, vfForceRegion, vfForceGradient, vfForceBalloon;
	CArray1D<CCouple<float> > forces;
	CCouple<float> ptMin, ptMax; // Image bounds

	ptMin.Set(1.0f, 1.0f);
	ptMax.Set((float)pInputImage->cols-2.0f, (float)pInputImage->rows-2.0f);

	// If edge information is used, check if vector field is initialized
	if (fWeightGradient!=0.0f && (arrayGradientField.GetWidth()!=pInputImage->cols || arrayGradientField.GetHeight()!=pInputImage->rows))
	{
		cerr<<"ERROR in CActiveContourBase::EnergyGradientDescent(...): vector field derived from image gradient is not initialized. Call AttachImage() again."<<endl;
		return false;
	}

	for (iPass=0; iPass<iNbPasses; iPass++)
	{
		iNbVertices = listVertices.size();
		if (iNbVertices<3)
			return false;

		forces.Init(iNbVertices);
		forces.Fill(0.0f);

		// Smoothness forces
		if (fWeightSmoothness!=0.0f && bUseGaussianFilter==false)
		{
			// Smoothness force of first vertex
			itVertex = listVertices.begin();
			itVertexPrev = --listVertices.end();
			itVertexNext = itVertex;
			itVertexNext++;

			vfForceSmoothness = (itVertexPrev->pfPos + itVertexNext->pfPos)*0.5f - itVertex->pfPos;
			forces[0] += vfForceSmoothness*fWeightSmoothness;

			// Smoothness forces of other vertices
			for (itVertex=++listVertices.begin(), i=1; itVertex!=--listVertices.end(); itVertex++, i++)
			{
				itVertexPrev = itVertex;
				itVertexPrev--;
				itVertexNext = itVertex;
				itVertexNext++;

				vfForceSmoothness = (itVertexPrev->pfPos + itVertexNext->pfPos)*0.5f - itVertex->pfPos;
				forces[i] += vfForceSmoothness*fWeightSmoothness;
			}

			// Smoothness force of last vertex
			itVertex = --listVertices.end();
			itVertexPrev = itVertex;
			itVertexPrev--;
			itVertexNext = listVertices.begin();

			vfForceSmoothness = (itVertexPrev->pfPos + itVertexNext->pfPos)*0.5f - itVertex->pfPos;
			forces[iNbVertices-1] += vfForceSmoothness*fWeightSmoothness;
		}

		// Region forces
		if (fWeightRegion!=0.0f)
		{
			// Region force of first vertex
			itVertex = listVertices.begin();
			itVertexPrev = --listVertices.end();
			itVertexNext = itVertex;
			itVertexNext++;

			vfForceRegion = RegionForce(&(*itVertexPrev), &(*itVertex), &(*itVertexNext));
			forces[0] += vfForceRegion*fWeightRegion;

			// Region forces of other vertices
			for (itVertex=++listVertices.begin(), i=1; itVertex!=--listVertices.end(); itVertex++, i++)
			{
				itVertexPrev = itVertex;
				itVertexPrev--;
				itVertexNext = itVertex;
				itVertexNext++;

				vfForceRegion = RegionForce(&(*itVertexPrev), &(*itVertex), &(*itVertexNext));
				forces[i] += vfForceRegion*fWeightRegion;
			}

			// Region force of last vertex
			itVertex = --listVertices.end();
			itVertexPrev = itVertex;
			itVertexPrev--;
			itVertexNext = listVertices.begin();

			vfForceRegion = RegionForce(&(*itVertexPrev), &(*itVertex), &(*itVertexNext));
			forces[iNbVertices-1] += vfForceRegion*fWeightRegion;
		}

		// Gradient forces
		if (fWeightGradient!=0.0f)
		{
			for (itVertex=listVertices.begin(), i=0; itVertex!=listVertices.end(); itVertex++, i++)
			{
				vfForceGradient = arrayGradientField.GetElementInterpolate(itVertex->pfPos);
				forces[i] += vfForceGradient*fWeightGradient;
			}
		}

		// Balloon forces
		// If region energy is used and bias is enabled, do not apply extra balloon force
		if (fWeightBalloon!=0.0f && (fWeightRegion==0.0f || bRegionEnergyBias==false))
		{
			for (itVertex=listVertices.begin(), i=0; itVertex!=listVertices.end(); itVertex++, i++)
			{
				vfForceBalloon = itVertex->vfNormal;
				forces[i] += vfForceBalloon*fWeightBalloon;
			}
		}

		// Update vertices positions
		for (itVertex=listVertices.begin(), i=0; itVertex!=listVertices.end(); itVertex++, i++)
		{
			itVertex->pfPos += forces[i];
			itVertex->pfPos.LimitWithinRange(ptMin, ptMax);
		}

		// If gaussian smoothing is enabled...
		if (fWeightSmoothness!=0.0f && fStdDeviationGaussianFilter!=0.0f && bUseGaussianFilter==true)
			GaussianSmooth();

		// If resampling is enabled...
		if (bResample==true)
			Resample();

		// Update normal vectors
		ComputeNormals();
	}
	UpdateAfterEvolution();

	return true;
}

// Curve smoothing with Gaussian kernel
// Each vertex is moved with the Gaussian smoothing force of equation (55) in [Mille09]
// To avoid shrinkage, we apply the two-pass method described in
// G. Taubin, "Curve smoothing without shrinkage", ICCV, 1995
void CActiveContourBase::GaussianSmooth()
{
	int iWeight, iVertex, iNbVertices, iGaussianSize, iPass;
	list<CVertex2D>::iterator itVertex, itVertexPrev, itVertexNext;
	CArray1D<float> vectGaussianWeights;
	CArray1D<CCouple<float> > vectForces;
	float f2Sigma2;
	static const float fCoefPass[2] = {1.0f, -1.0f};

	iNbVertices = listVertices.size();

	if (iNbVertices==0 || fStdDeviationGaussianFilter==0.0f)
		return;

	vectForces.Init(iNbVertices);

	// Compute size of gaussian kernel, considering three times the standard derivation
	iGaussianSize = (int)(3.0f*fStdDeviationGaussianFilter);
	if (iGaussianSize==0)
		iGaussianSize = 1;
	vectGaussianWeights.Init(iGaussianSize+1);

	// Precompute gaussian weights
	f2Sigma2 = 2.0f*fStdDeviationGaussianFilter*fStdDeviationGaussianFilter;
	for (iWeight=0; iWeight<vectGaussianWeights.GetSize(); iWeight++)
		vectGaussianWeights[iWeight] = exp(-(float)(iWeight*iWeight)/f2Sigma2)/(fStdDeviationGaussianFilter*2.5066f);

	for (iPass=0; iPass<2; iPass++)
	{
		for (itVertex=listVertices.begin(), iVertex=0; itVertex!=listVertices.end(); itVertex++, iVertex++)
		{
			// Convolve neighboring vertices with the gaussian kernel
			vectForces[iVertex] = itVertex->pfPos * vectGaussianWeights[0];
			itVertexPrev = itVertex;
			itVertexNext = itVertex;

			for (iWeight=1;iWeight<vectGaussianWeights.GetSize();iWeight++)
			{
				// Get the i^th previous vertex
				if (itVertexPrev!=listVertices.begin())
					itVertexPrev--;
				else
					itVertexPrev = --listVertices.end();

				// Get the i^th next vertex
				itVertexNext++;
				if (itVertexNext==listVertices.end())
					itVertexNext = listVertices.begin();

				vectForces[iVertex] += vectGaussianWeights[iWeight]*(itVertexPrev->pfPos + itVertexNext->pfPos);
			}

			// At this stage, vectForces[iVertex] holds the smoothed vertex position, resulting from the convolution of
			// its neighboring vertices with the gaussian kernel.
			// Subtract the current vertex position, so that it actually corresponds to a displacement vector
			vectForces[iVertex] -= itVertex->pfPos;
		}

		// Move vertices with computed forces
		// At 1st pass, the force is applied as is
		// At 2nd pass, the force is applied in the opposite direction
		for (itVertex=listVertices.begin(), iVertex=0; itVertex!=listVertices.end(); itVertex++, iVertex++)
			itVertex->pfPos += fWeightSmoothness*fCoefPass[iPass]*vectForces[iVertex];
	}
}

// Update unit inward normal vectors once vertices have been moved or initialized
void CActiveContourBase::ComputeNormals()
{
	CCouple<float> norm, tang;
	list<CVertex2D>::iterator itVertex, itVertexPrev, itVertexNext;

	if (listVertices.size()<3)
		return;

	// Normal vector of first vertex
	itVertex = listVertices.begin();
	itVertexPrev = --listVertices.end();
	itVertexNext = itVertex;
	itVertexNext++;

	tang = itVertexNext->pfPos - itVertexPrev->pfPos;
	norm.x = -tang.y;
	norm.y = tang.x;
	itVertex->vfNormal = norm.Normalized();

	// Normal vector of other vertices
	for (itVertex=++listVertices.begin(); itVertex!=--listVertices.end(); itVertex++)
	{
		itVertexPrev = itVertex;
		itVertexPrev--;
		itVertexNext = itVertex;
		itVertexNext++;

		tang = itVertexNext->pfPos - itVertexPrev->pfPos;
		norm.x = -tang.y;
		norm.y = tang.x;
		itVertex->vfNormal = norm.Normalized();
	}

	// Normal vector of last vertex
	itVertex = --listVertices.end();
	itVertexPrev = itVertex;
	itVertexPrev--;
	itVertexNext = listVertices.begin();

	tang = itVertexNext->pfPos - itVertexPrev->pfPos;
	norm.x = -tang.y;
	norm.y = tang.x;
	itVertex->vfNormal = norm.Normalized();
}

// Return position of approximated centroid of curve (just the average of vertices)
CCouple<float> CActiveContourBase::GetCentroid() const
{
	list<CVertex2D>::const_iterator itVertex;
	CCouple<float> pfCentroid(0.0f, 0.0f);
	int iNbVertices;

	iNbVertices = listVertices.size();

	if (iNbVertices==0)
		return pfCentroid;

	for (itVertex=listVertices.begin(); itVertex!=listVertices.end(); itVertex++)
		pfCentroid += itVertex->pfPos;

	return pfCentroid/(float)iNbVertices;
}

// Return length of curve by summing lengths of edges between vertices
float CActiveContourBase::GetLength() const
{
	list<CVertex2D>::const_iterator itVertex, itVertexNext;
	float fLength;
	int iNbVertices;

	iNbVertices = listVertices.size();
	if (iNbVertices<2)
		return 0.0f;

	fLength = 0.0f;

	// Sum up the lengths of all edges except the last one
	for (itVertex=listVertices.begin(); itVertex!=--listVertices.end(); itVertex++)
	{
		itVertexNext = itVertex;
		itVertexNext++;
		fLength += (itVertex->pfPos-itVertexNext->pfPos).L2Norm();
	}

	// Add length of last edge
	itVertex = --listVertices.end();
	itVertexNext = listVertices.begin();
	fLength += (itVertex->pfPos-itVertexNext->pfPos).L2Norm();

	return fLength;
}

void CActiveContourBase::InitCircle(const CCouple<float> &pfCenter, float fRadius)
{
	CVertex2D vertexNew;
	CCouple<float> ptMin, ptMax;
	int i, iNbVertices;
	static const float _2pi = 6.28319f;
	float fAngle;

	listVertices.clear();

	if (pInputImage==NULL)
	{
		cerr<<"ERROR in CActiveContourBase::InitCircle(...): cannot initialize the contour: not attached to image. Call AttachImage() before"<<endl;
		return;
	}

	// Number of vertices depends on sampling and circle perimeter
	iNbVertices = (int)((_2pi*fRadius)/fSampling);
	if (iNbVertices<3)
	{
		cerr<<"ERROR in CActiveContourBase::InitCircle(...): cannot initialize the contour: circle is too small to yield enough vertices with current sampling"<<endl;
		return;
	}

	ptMin.Set(1.0f, 1.0f);
	ptMax.Set((float)pInputImage->cols-2.0f, (float)pInputImage->rows-2.0f);

	// Create vertices
	for (i=0;i<iNbVertices;i++)
	{
		fAngle = _2pi*(float)i/(float)iNbVertices;

		vertexNew.pfPos.x = pfCenter.x + fRadius*cos(fAngle);
		vertexNew.pfPos.y = pfCenter.y + fRadius*sin(fAngle);
		vertexNew.pfPos.LimitWithinRange(ptMin, ptMax);

		listVertices.push_back(vertexNew);
	}

	ComputeNormals();
	UpdateAfterInitialization();
}

void CActiveContourBase::InitEllipse(const CCouple<float> &pfCenter, float fRadiusX, float fRadiusY)
{
	CVertex2D vertexNew;
	CCouple<float> ptMin, ptMax;
	int i, iNbVertices;
	static const float _pi = 3.14159f, _2pi = 6.28319f;
	float fAngle;

	listVertices.clear();

	if (pInputImage==NULL)
	{
		cerr<<"ERROR in CActiveContourBase::InitEllipse(...): cannot initialize the contour: not attached to image. Call AttachImage() before"<<endl;
		return;
	}

	// Number of vertices depends on sampling and approximated ellipse perimeter
	iNbVertices = (int)(_pi*(3.0f*(fRadiusX+fRadiusY)-sqrt((fRadiusX + 3.0f*fRadiusY)*(3.0f*fRadiusX + fRadiusY)))/fSampling);
	if (iNbVertices<3)
	{
		cerr<<"ERROR in CActiveContourBase::InitEllipse(...): cannot initialize the contour: ellipse is too small to yield enough vertices with current sampling"<<endl;
		return;
	}

	ptMin.Set(1.0f, 1.0f);
	ptMax.Set((float)pInputImage->cols-2.0f, (float)pInputImage->rows-2.0f);

	// Create vertices
	for (i=0;i<iNbVertices;i++)
	{
		fAngle = _2pi*(float)i/(float)iNbVertices;

		vertexNew.pfPos.x = pfCenter.x + fRadiusX*cos(fAngle);
		vertexNew.pfPos.y = pfCenter.y + fRadiusY*sin(fAngle);
		vertexNew.pfPos.LimitWithinRange(ptMin, ptMax);

		listVertices.push_back(vertexNew);
	}

	ComputeNormals();
	UpdateAfterInitialization();
}

void CActiveContourBase::DrawInImageRGB(cv::Mat &im, int iZoom) const
{
	list<CVertex2D>::const_iterator itVertex, itVertexNext;
	CArray1D<cv::Point> vectIntPos;
	CCouple<int> piTemp;
	int i, iNbVertices;

	iNbVertices = listVertices.size();
	if (iNbVertices<3)
		return;

	if (im.type()!=CV_8UC3)
	{
		cerr<<"ERROR in CActiveContourBase::DrawInImageRGB(...): image is not RGB"<<endl;
		return;
	}

	vectIntPos.Init(iNbVertices);

	// Compute integer coordinates of vertices with current zooming factor
	for (itVertex=listVertices.begin(), i=0; itVertex!=listVertices.end(); itVertex++, i++)
	{
	    piTemp = (CCouple<int>)(itVertex->pfPos*(float)iZoom) + CCouple<int>(iZoom/2, iZoom/2);
	    vectIntPos[i].x = piTemp.x;
	    vectIntPos[i].y = piTemp.y;
	}

    cv::Scalar rgbEdgeCV = cv::Scalar(rgbEdgeColor.x, rgbEdgeColor.y, rgbEdgeColor.z);

	// Draw curve
	if (bDisplayEdges==true)
	{
		for (i=0; i<iNbVertices-1; i++)
            cv::line(im, vectIntPos[i], vectIntPos[i+1], rgbEdgeCV);
			// im.DrawLineRGB(vectIntPos[i], vectIntPos[i+1], rgbEdgeColor);
		cv::line(im, vectIntPos[iNbVertices-1], vectIntPos[0], rgbEdgeCV);
		// im.DrawLineRGB(vectIntPos[iNbVertices-1], vectIntPos[0], rgbEdgeColor);
	}

    cv::Scalar rgbVertexCV = cv::Scalar(rgbVertexColor.x, rgbVertexColor.y, rgbVertexColor.z);

	// Draw vertices
	if (bDisplayVertices==true)
	{
		for (i=0; i<iNbVertices; i++)
            cv::circle(im, vectIntPos[i], iVertexWidth, rgbVertexCV);
			// im.DrawFilledCircleRGB(vectIntPos[i], iVertexWidth, rgbVertexColor);
	}

    /*
	// Draw inner and outer parallel curves
	if (bDisplayParallelCurves==true)
	{
		CCouple<float> pfParallelPoint;
		float fBandThickness = (float)iBandThickness;

		// Compute integer coordinates of vertices on the inner parallel curve with current zooming factor
		for (itVertex=listVertices.begin(), i=0; itVertex!=listVertices.end(); itVertex++, i++)
		{
			pfParallelPoint = itVertex->pfPos + itVertex->vfNormal*fBandThickness;
			vectIntPos[i] = (CCouple<int>)(pfParallelPoint*(float)iZoom) + CCouple<int>(iZoom/2, iZoom/2);
		}

		// Draw inner parallel curve
		for (i=0; i<iNbVertices-1; i++)
			im.DrawLineRGB(vectIntPos[i], vectIntPos[i+1], rgbEdgeParallelColor);
		im.DrawLineRGB(vectIntPos[iNbVertices-1], vectIntPos[0], rgbEdgeParallelColor);

		// Compute integer coordinates of vertices on the outer parallel curve with current zooming factor
		for (itVertex=listVertices.begin(), i=0; itVertex!=listVertices.end(); itVertex++, i++)
		{
			pfParallelPoint = itVertex->pfPos - itVertex->vfNormal*fBandThickness;
			vectIntPos[i] = (CCouple<int>)(pfParallelPoint*(float)iZoom) + CCouple<int>(iZoom/2, iZoom/2);
		}

		// Draw outer parallel curve
		for (i=0; i<iNbVertices-1; i++)
			im.DrawLineRGB(vectIntPos[i], vectIntPos[i+1], rgbEdgeParallelColor);
		im.DrawLineRGB(vectIntPos[iNbVertices-1], vectIntPos[0], rgbEdgeParallelColor);
	}
	*/
}

void CActiveContourBase::MakeBinaryMask(cv::Mat &imgMask) const
{
    CCouple<float> pfHalf(0.5f, 0.5f);
    list<CVertex2D>::const_iterator itVertex;
    CArray1D<cv::Point> vectIntPos;
    CCouple<int> piTemp;
    unsigned int i, iNbVertices;

	if (imgMask.rows!=pInputImage->rows || imgMask.cols!=pInputImage->cols || imgMask.type()!=CV_8UC1)
		imgMask.create(pInputImage->rows, pInputImage->cols, CV_8UC1);

    iNbVertices = listVertices.size();
    vectIntPos.Init(iNbVertices);

	// Compute integer coordinates of vertices
	for (itVertex=listVertices.begin(), i=0; itVertex!=listVertices.end(); itVertex++, i++)
	{
		piTemp = (CCouple<int>)(itVertex->pfPos + pfHalf);
        vectIntPos[i].x = piTemp.x;
	    vectIntPos[i].y = piTemp.y;
	}

	// Clear image in black and draw curve
    imgMask.setTo(0);
    for (i=0; i<iNbVertices-1; i++)
        cv::line(imgMask, vectIntPos[i], vectIntPos[i+1], 255);
    cv::line(imgMask, vectIntPos[iNbVertices-1], vectIntPos[0], 255);
}

void CActiveContourBase::Empty()
{
	listVertices.clear();
}

void CActiveContourBase::UpdateAfterEvolution()
{
	if (fWeightRegion!=0.0f)
	{
		if (iInsideMode==REGION || iOutsideMode==REGION)
			ComputeRegionMeansGreenTheorem();

		if (iInsideMode==BAND || iOutsideMode==BAND)
			ComputeBandMeans();
	}
}

/**********************************************************************
*                      CActiveContourGrayscale                        *
**********************************************************************/

CActiveContourGrayscale::CActiveContourGrayscale():CActiveContourBase()
{
	fInnerMean = 0.0f;
	fOuterMean = 0.0f;

	fInnerNarrowBandMean = 0.0f;
	fOuterNarrowBandMean = 0.0f;
}

void CActiveContourGrayscale::AttachImage(const cv::Mat *pImage)
{
	// int iBitsPerPixel;

	// iBitsPerPixel = pImage->GetBitsPerPixel();
	if (pImage->type()!=CV_8UC1)
	{
		cerr<<"ERROR in CActiveContourGrayscale::AttachImage(): instances of CActiveContourGrayscale can be attached to 8-bit images only"<<endl;
		pInputImage = NULL;
		return;
	}

	pInputImage = pImage;

    ConvertCvMatToArray2DFloat(arrayImage, *pImage);
	//pInputImage->ConvertToArray2DFloat(arrayImage);

	// If edge information is used, then precompute vector field which will be used for gradient force
	if (fWeightGradient!=0.0f)
	{
		CArray2D<float> arrayGaussian, arrayDiffX, arrayDiffY, arrayGradientNorm;
		// CImage2D imageGradient;
		float *pDiffX, *pDiffY, *pNorm;
		float fScale;
		int i, iSize;

		fScale = 1.0f;

		// Gaussian mask's half size should be at least 3
		if (fScale<1.0f)
			arrayGaussian.SetGaussianKernel(fScale, 3);
		else
			arrayGaussian.SetGaussianKernel(fScale);

		// Convolve the image with x and y-derivatives of gaussian
		arrayDiffX = arrayImage.Convolve(arrayGaussian.DerivativeX(1, ARRAYNDFLOAT_CENTERED));
		arrayDiffY = arrayImage.Convolve(arrayGaussian.DerivativeY(1, ARRAYNDFLOAT_CENTERED));

		// Compute the magnitude of the obtained "smooth gradient" for every pixel
		arrayGradientNorm.Init(arrayImage.GetSize());
		pDiffX = arrayDiffX.GetBuffer();
		pDiffY = arrayDiffY.GetBuffer();
		pNorm = arrayGradientNorm.GetBuffer();
		iSize = arrayGradientNorm.GetWidth()*arrayGradientNorm.GetHeight();
		for (i=0; i<iSize; i++)
		{
			*pNorm = sqrt((*pDiffX)*(*pDiffX) + (*pDiffY)*(*pDiffY));
			pDiffX++;
			pDiffY++;
			pNorm++;
		}

		// Final vector field is taken as the gradient of the smooth gradient magnitude
		arrayGradientField = arrayGradientNorm.Gradient();
	}

	if (fWeightRegion!=0.0f)
	{
		if (bUseImageIntegrals==true)
			ComputeImageIntegrals();
		else {
			// Compute sum of image intensities
			// Useful to compute average intensity outside the contour if needed
			int x, y, iWidth, iHeight;

			iWidth = arrayImage.GetWidth();
			iHeight = arrayImage.GetHeight();

			fImageSum = 0.0f;

			for (y=0;y<iHeight;y++)
				for (x=0;x<iWidth;x++)
					fImageSum += arrayImage.Element(x,y);
		}
	}
}

void CActiveContourGrayscale::ComputeImageIntegrals()
{
	int x, y, width, height;

	width = arrayImage.GetWidth();
	height = arrayImage.GetHeight();

	arrayImageIntegralX.Init(width, height);
	arrayImageIntegralY.Init(width, height);

	// Compute cumulative intensities in x-direction
	for (y=0;y<height;y++)
	{
		arrayImageIntegralX.Element(0,y) = arrayImage.Element(0,y);
		for (x=1;x<width;x++)
			arrayImageIntegralX.Element(x,y) = arrayImageIntegralX.Element(x-1,y)+arrayImage.Element(x,y);
	}

	// Compute cumulative intensities in y-direction
	for (x=0;x<width;x++)
	{
		arrayImageIntegralY.Element(x,0) = arrayImage.Element(x,0);
		for (y=1;y<height;y++)
			arrayImageIntegralY.Element(x,y) = arrayImageIntegralY.Element(x,y-1)+arrayImage.Element(x,y);
	}

	// Compute sum of image intensities
	fImageSum = 0.0f;
	for (y=0;y<height;y++)
		fImageSum += arrayImageIntegralX.Element(width-1,y);
}

void CActiveContourGrayscale::ComputeRegionMeansGreenTheorem()
{
	list<CVertex2D>::const_iterator itVertex, itVertexPrev, itVertexNext;
	int iWidth, iHeight;
	CCouple<float> vfDerivative1;
	CCouple<int> piPos, piIntegral;
	float fInnerSum, fOuterSum;
	float fSumX, fSumY;

	if (listVertices.size()<3)
		return;

	iWidth = arrayImage.GetWidth();
	iHeight = arrayImage.GetHeight();

	fInnerRegionArea = 0.0f;
	fInnerSum = 0.0f;

	for (itVertex=listVertices.begin(); itVertex!=listVertices.end(); itVertex++)
	{
		if (itVertex==listVertices.begin())
			itVertexPrev = --listVertices.end();
		else {
			itVertexPrev = itVertex;
			itVertexPrev--;
		}

		itVertexNext = itVertex;
		itVertexNext++;
		if (itVertexNext==listVertices.end())
			itVertexNext = listVertices.begin();

		piPos = (CCouple<int>)itVertex->pfPos;

		// Velocity vector at current vertex: centered finite difference approximation
		// of curve first-order derivative
		// Could also use backward finite difference : itVertex->pfPos - itVertexPrev->pfPos ?
		vfDerivative1 = (itVertexNext->pfPos-itVertexPrev->pfPos)*0.5f;

		fInnerRegionArea += vfDerivative1.y*itVertex->pfPos.x - vfDerivative1.x*itVertex->pfPos.y;

		if (bUseImageIntegrals==true)
			// Arrays arrayImageIntegralX and arrayImageIntegralY implements function 2Q and -2P, respectively,
			// defined in equation (13), so next instruction implements equation (14) of section 2.3 in [Mille09]:
			fInnerSum += vfDerivative1.y*arrayImageIntegralX.Element(piPos) - vfDerivative1.x*arrayImageIntegralY.Element(piPos);
		else {
			// If image integrals have not been computed, apply "brutal" discretization
			// of Green's theorem (see first equation of section 5.6 in [Mille09])
			fSumX = 0.0f;
			piIntegral.y = piPos.y;
			for (piIntegral.x=0; piIntegral.x<=piPos.x; piIntegral.x++)
				fSumX += arrayImage.Element(piIntegral);

			fSumY = 0.0f;
			piIntegral.x = piPos.x;
			for (piIntegral.y=0; piIntegral.y<=piPos.y; piIntegral.y++)
				fSumY += arrayImage.Element(piIntegral);

			fInnerSum += vfDerivative1.y*fSumX - vfDerivative1.x*fSumY;
		}
	}

	fInnerRegionArea*=0.5f;
	fOuterRegionArea = (float)(iWidth*iHeight)-fInnerRegionArea;

	fInnerSum*=0.5f;
	fOuterSum = fImageSum - fInnerSum;
	fInnerMean = fInnerSum/fInnerRegionArea;
	fOuterMean = fOuterSum/fOuterRegionArea;
}

void CActiveContourGrayscale::ComputeBandMeans()
{
	list<CVertex2D>::const_iterator itVertex, itVertexPrev, itVertexNext;
	CCouple<float> vfDerivative1, vfDerivative2;
	float fThickness, fBandThickness = (float)iBandThickness;
	float fLengthElement, fLengthElementParallelCurve, fCurvature;
	float fIntensity;

	if (listVertices.size()<3)
		return;

	fInnerNarrowBandArea = 0.0f;
	fOuterNarrowBandArea = 0.0f;

	fInnerNarrowBandMean = 0.0f;
	fOuterNarrowBandMean = 0.0f;

	// This loop implements the computation of average intensities on the narrow bands,
	// according to the template expression in equation (51) in [Mille09]
	for (itVertex=listVertices.begin(); itVertex!=listVertices.end(); itVertex++)
	{
		if (itVertex==listVertices.begin())
			itVertexPrev = --listVertices.end();
		else {
			itVertexPrev = itVertex;
			itVertexPrev--;
		}

		itVertexNext = itVertex;
		itVertexNext++;
		if (itVertexNext==listVertices.end())
			itVertexNext = listVertices.begin();

		// Velocity vector at current vertex: centered finite difference approximation
		// of curve first-order derivative
		// Could also use backward finite difference : itVertex->pfPos - itVertexPrev->pfPos ?
		vfDerivative1 = (itVertexNext->pfPos-itVertexPrev->pfPos)*0.5f;

		// Finite difference approximation of curve second-order derivative
		vfDerivative2 = itVertexPrev->pfPos + itVertexNext->pfPos - 2.0f*itVertex->pfPos;

		// Discrete length element, denoted \ell_i in [Mille09] (continuous definition is in equation (9) )
		fLengthElement = vfDerivative1.L2Norm();

		// Discrete curvature, denoted \kappa_i in [Mille09] (continuous definition is given shortly after length element)
		fCurvature = (vfDerivative1.x * vfDerivative2.y - vfDerivative1.y * vfDerivative2.x)/(fLengthElement*fLengthElement*fLengthElement);

		for (fThickness=0; fThickness<fBandThickness; fThickness+=1.0f)
		{
			fLengthElementParallelCurve = fLengthElement * (1.0f - fThickness*fCurvature);

			// The following test is related to regularity condition (12) in [Mille09]
			// If it is false, the thickness exceeds the radius of curvature, yielding a singularity on the
			// inner parallel curve. In this case, it seems reasonable not to include the resulting pixel
			// in the inner average color
			if (fLengthElementParallelCurve>0.0f)
			{
				fIntensity = arrayImage.GetElementInterpolate(itVertex->pfPos + fThickness*itVertex->vfNormal);
				fInnerNarrowBandMean += fIntensity*fLengthElementParallelCurve;
				fInnerNarrowBandArea += fLengthElementParallelCurve;
			}
		}

		for (fThickness=1.0f; fThickness<=fBandThickness; fThickness+=1.0f)
		{
			fLengthElementParallelCurve = fLengthElement * (1.0f + fThickness*fCurvature);

			// The following test is related to regularity condition (12) in [Mille09]
			// If it is false, the thickness exceeds the radius of curvature, yielding a singularity on the
			// outer parallel curve. In this case, it seems reasonable not to include the resulting pixel
			// in the outer average color
			if (fLengthElementParallelCurve>0.0f)
			{
				fIntensity = arrayImage.GetElementInterpolate(itVertex->pfPos - fThickness*itVertex->vfNormal);
				fOuterNarrowBandMean += fIntensity*fLengthElementParallelCurve;
				fOuterNarrowBandArea += fLengthElementParallelCurve;
			}
		}
	}

	fInnerNarrowBandMean /= fInnerNarrowBandArea;
	fOuterNarrowBandMean /= fOuterNarrowBandArea;
}

CCouple<float> CActiveContourGrayscale::RegionForce(const CVertex2D *pVertexPrev, const CVertex2D *pVertex, const CVertex2D *pVertexNext) const
{
	// Region terms
	float fDiffRegion, fDiffRegionInside, fDiffRegionOutside;

	// Band thickness stored as a real number (make the conversion once)
	float fBandThickness = (float)iBandThickness;

	// Intensity at current vertex
	float fIntensity;

	// Inner and outer intensity region descriptors
	float fInnerMeanCurrent=0.0f, fOuterMeanCurrent=0.0f;

	// For computing the bias force
	float fCoefBias, fForceBias, fDiffInsideOutside;

	fIntensity = arrayImage.GetElementInterpolate(pVertex->pfPos);

	// Choose appropriate inner region descriptor depending on iInsideMode
	if (iInsideMode==REGION)
		fInnerMeanCurrent = fInnerMean;
	else if (iInsideMode==REGION_INIT)
		fInnerMeanCurrent = fInnerInitialMean;
	else if (iInsideMode==BAND)
		fInnerMeanCurrent = fInnerNarrowBandMean;

	// Choose appropriate outer region descriptor depending on iOutsideMode
	if (iOutsideMode==REGION)
		fOuterMeanCurrent = fOuterMean;
	else if (iOutsideMode==BAND)
		fOuterMeanCurrent = fOuterNarrowBandMean;
	else if (iOutsideMode==LINE)
	{
		float fThickness;
		float fLengthElement, fWeightCurvature, fCurvature, fSumWeights;
		CCouple<float> vfDerivative1, vfDerivative2;

		// Velocity vector at current vertex: centered finite difference approximation
		// of curve first-order derivative
		// Could also use backward finite difference : itVertex->pfPos - itVertexPrev->pfPos ?
		vfDerivative1 = (pVertexNext->pfPos-pVertexPrev->pfPos)*0.5f;

		// Finite difference approximation of curve second-order derivative
		vfDerivative2 = pVertexPrev->pfPos + pVertexNext->pfPos - 2.0f*pVertex->pfPos;

		// Discrete length element, denoted \ell_i in [Mille09] (continuous definition is in equation (9) )
		fLengthElement = vfDerivative1.L2Norm();

		// Discrete curvature, denoted \kappa_i in [Mille09] (continuous definition is given shortly after length element)
		fCurvature = (vfDerivative1.x * vfDerivative2.y - vfDerivative1.y * vfDerivative2.x)/(fLengthElement*fLengthElement*fLengthElement);

		// Average weighted intensity along the outward normal line segment of length equal to band thickness
		// Implements the computation of \mu_NL appearing just after equation (54) in [Mille09]
		fOuterMeanCurrent = 0.0f;
		fSumWeights = 0.0f;
		for (fThickness=1.0f; fThickness<=fBandThickness; fThickness+=1.0f)
		{
			fWeightCurvature = 1.0f + fThickness*fCurvature; // Implements (1+b\kappa_i)

			// The following test is related to regularity condition (12) in [Mille09]
			// If it is false, the thickness exceeds the radius of curvature, yielding a singularity on the
			// outer parallel curve. In this case, it seems reasonable not to include the resulting pixel
			if (fWeightCurvature>0.0f)
			{
				fOuterMeanCurrent += arrayImage.GetElementInterpolate(pVertex->pfPos - fThickness*pVertex->vfNormal)*fWeightCurvature;
				fSumWeights += fWeightCurvature;
			}
		}

		if (fSumWeights==0.0f)
		{
			// This happens if the curve is excessively concave at the current vertex
			// Return a null force, so that only regularization is effective on this particular vertex
			return CCouple<float>(0.0f, 0.0f);
		}
		else
			fOuterMeanCurrent /= fSumWeights;
	}

	// Unlike in [Mille09], deviations with respect to inner and outer descriptors
	// are computed using the L1 norm
	// This yields better behaviour with respect to local intensity differences
	fDiffRegionInside = fabs(fInnerMeanCurrent-fIntensity);
	fDiffRegionOutside = fabs(fOuterMeanCurrent-fIntensity);

	// The narrow band region energies introduced in [Mille09] are symmetric, i.e. inner and outer region terms
	// are identically weighted However, an asymmetric configuration is allowed here, in order to favor
	// minimization of intensity deviation inside or outside. This enables to implement the parametric equivalent
	// of the Chan-Vese model (which is asymmetric) for comparison purpose.
	// The initial configuration is symmetric, since the default value of fWeightRegionInOverOut is 0.5
	fDiffRegion = fWeightRegionInOverOut * fDiffRegionInside - (1.0f-fWeightRegionInOverOut) * fDiffRegionOutside;

	// If bias is enabled, a balloon-like force is added to the region force
	if (bRegionEnergyBias==true)
	{
		// This is slightly different from the bias force described in section 5.5 in [Mille09],
		// as the magnitude of the bias is set by the balloon weight (which should be negative)
		fForceBias = fWeightBalloon + fabs(fInnerMeanCurrent-fIntensity);

		// Compute bias coefficient, denoted \gamma and introduced in equation (56) in [Mille09]
		// It decreases exponentially with respect to the difference between inner and outer region descriptors
		fDiffInsideOutside = fabs(fInnerMeanCurrent-fOuterMeanCurrent);
		fCoefBias = (1.0f-fDiffInsideOutside)/(1.0f+50.0f*fDiffInsideOutside);

		// Linear combination of bias and region forces
		// See second part of equation (56)
		fDiffRegion = fCoefBias*fForceBias + (1.0f-fCoefBias)*fDiffRegion;
	}

	return pVertex->vfNormal*fDiffRegion;
}

void CActiveContourGrayscale::UpdateAfterInitialization()
{
	if (fWeightRegion!=0.0f)
	{
		if (iInsideMode==REGION || iInsideMode==REGION_INIT ||iOutsideMode==REGION)
		{
			ComputeRegionMeansGreenTheorem();
			if (iInsideMode==REGION_INIT)
				fInnerInitialMean = fInnerMean;
		}

		if (iInsideMode==BAND || iOutsideMode==BAND)
			ComputeBandMeans();
	}
}



/**********************************************************************
*                      CActiveContourColor                            *
**********************************************************************/

CActiveContourColor::CActiveContourColor():CActiveContourBase()
{
	fcolInnerMean.Set(0.0f, 0.0f, 0.0f);
	fcolOuterMean.Set(0.0f, 0.0f, 0.0f);

	fcolInnerNarrowBandMean.Set(0.0f, 0.0f, 0.0f);
	fcolOuterNarrowBandMean.Set(0.0f, 0.0f, 0.0f);

	// Default color space is RGB
	iColorSpace = RGB;
	bIgnoreBrightnessComponent = false;
}

void CActiveContourColor::AttachImage(const cv::Mat *pImage)
{
	// int iBitsPerPixel;

	if (pImage->type()!=CV_8UC3)
	{
		cerr<<"ERROR in CActiveContourColor::AttachImage(): instances of CActiveContourColor can be attached to RGB images only"<<endl;
		pInputImage = NULL;
		return;
	}

	pInputImage = pImage;

	if (iColorSpace==RGB)
		ConvertCvMatToArray2DTripletFloatRGB(arrayImage, *pImage);
	else if (iColorSpace==YUV)
		ConvertCvMatToArray2DTripletFloatYUV(arrayImage, *pImage);
	else if (iColorSpace==LAB)
		ConvertCvMatToArray2DTripletFloatLAB(arrayImage, *pImage);

	// If chosen color space separates brightness from color information (i.e. not RGB)
	// and brightness component should be ignored for illumination invariance purpose,
	// then set brightness component to zero everywhere
	if (iColorSpace!=RGB && bIgnoreBrightnessComponent==true)
	{
		CTriplet<float> *pColor;
		int i,iSize;

		iSize = arrayImage.GetWidth()*arrayImage.GetHeight();
		pColor = arrayImage.GetBuffer();
		for (i=0; i<iSize; i++)
		{
			pColor->x = 0.0f;
			pColor++;
		}
	}

	// If edge information is used, then precompute vector field which will be used for gradient force
	if (fWeightGradient!=0.0f)
	{
		CArray2D<CTriplet<float> > arrayDiffX, arrayDiffY;
		CArray2D<float> arrayGaussian, arrayGradientNorm;
		CTriplet<float> *pDiffX, *pDiffY;
		float *pNorm;
		float fScale;
		int i, iSize;

		// Gaussian mask's half size should be at least 3
		fScale = 1.0f;
		if (fScale<1.0f)
			arrayGaussian.SetGaussianKernel(fScale, 3);
		else
			arrayGaussian.SetGaussianKernel(fScale);

		// Convolve the image with x and y-derivatives of gaussian
		arrayDiffX = arrayImage.Convolve(arrayGaussian.DerivativeX(1, ARRAYNDFLOAT_CENTERED));
		arrayDiffY = arrayImage.Convolve(arrayGaussian.DerivativeY(1, ARRAYNDFLOAT_CENTERED));

		// Compute the magnitude of the obtained "smooth gradient" for every pixel
		arrayGradientNorm.Init(arrayImage.GetSize());
		pDiffX = arrayDiffX.GetBuffer();
		pDiffY = arrayDiffY.GetBuffer();
		pNorm = arrayGradientNorm.GetBuffer();
		iSize = arrayGradientNorm.GetWidth()*arrayGradientNorm.GetHeight();
		for (i=0; i<iSize; i++)
		{
			*pNorm = sqrt(pDiffX->L2Norm2() + pDiffY->L2Norm2());
			pDiffX++;
			pDiffY++;
			pNorm++;
		}

		// Final vector field is taken as the gradient of the smooth gradient magnitude
		arrayGradientField = arrayGradientNorm.Gradient();
	}

	if (fWeightRegion!=0.0f)
	{
		if (bUseImageIntegrals==true)
			ComputeImageIntegrals();
		else {
			// Compute sum of image colors
			// Useful to compute average color outside the contour if needed
			int x, y, iWidth, iHeight;
			CTriplet<float> fPixel;

			iWidth = arrayImage.GetWidth();
			iHeight = arrayImage.GetHeight();

			fcolImageSum.Set(0.0f, 0.0f, 0.0f);

			for (y=0;y<iHeight;y++)
				for (x=0;x<iWidth;x++)
					fcolImageSum += arrayImage.Element(x,y);
		}
	}
}

void CActiveContourColor::ComputeImageIntegrals()
{
	int x, y, width, height;

	width = arrayImage.GetWidth();
	height = arrayImage.GetHeight();

	arrayImageIntegralX.Init(width, height);
	arrayImageIntegralY.Init(width, height);

	// Compute cumulative colors in x-direction
	for (y=0;y<height;y++)
	{
		arrayImageIntegralX.Element(0,y) = arrayImage.Element(0,y);
		for (x=1;x<width;x++)
			arrayImageIntegralX.Element(x,y) = arrayImageIntegralX.Element(x-1,y)+arrayImage.Element(x,y);
	}

	// Compute cumulative colors in y-direction
	for (x=0;x<width;x++)
	{
		arrayImageIntegralY.Element(x,0) = arrayImage.Element(x,0);
		for (y=1;y<height;y++)
			arrayImageIntegralY.Element(x,y) = arrayImageIntegralY.Element(x,y-1)+arrayImage.Element(x,y);
	}

	// Compute sum of image colors
	fcolImageSum.Set(0.0f, 0.0f, 0.0f);
	for (y=0;y<height;y++)
		fcolImageSum += arrayImageIntegralX.Element(width-1,y);
}

void CActiveContourColor::ComputeRegionMeansGreenTheorem()
{
	list<CVertex2D>::const_iterator itVertex, itVertexPrev, itVertexNext;
	int iWidth, iHeight;
	CCouple<float> vfDerivative1;
	CCouple<int> piPos, piIntegral;
	CTriplet<float> fPixel, fcolInnerSum, fcolOuterSum;
	CTriplet<float> fcolSumX, fcolSumY;

	if (listVertices.size()<3)
		return;

	iWidth = arrayImage.GetWidth();
	iHeight = arrayImage.GetHeight();

	fInnerRegionArea = 0.0f;
	fcolInnerSum.Set(0.0f, 0.0f, 0.0f);

	for (itVertex=listVertices.begin(); itVertex!=listVertices.end(); itVertex++)
	{
		if (itVertex==listVertices.begin())
			itVertexPrev = --listVertices.end();
		else {
			itVertexPrev = itVertex;
			itVertexPrev--;
		}

		itVertexNext = itVertex;
		itVertexNext++;
		if (itVertexNext==listVertices.end())
			itVertexNext = listVertices.begin();

		piPos = (CCouple<int>)itVertex->pfPos;

		// Velocity vector at current vertex: centered finite difference approximation
		// of curve first-order derivative
		// Could also use backward finite difference : itVertex->pfPos - itVertexPrev->pfPos ?
		vfDerivative1 = (itVertexNext->pfPos-itVertexPrev->pfPos)*0.5f;

		fInnerRegionArea += vfDerivative1.y*itVertex->pfPos.x - vfDerivative1.x*itVertex->pfPos.y;

		if (bUseImageIntegrals==true)
			// Arrays arrayImageIntegralX and arrayImageIntegralY implements function 2Q and -2P, respectively,
			// defined in equation (13), so next instruction implements equation (14) of section 2.3 in [Mille09]:
			fcolInnerSum += vfDerivative1.y*arrayImageIntegralX.Element(piPos) - vfDerivative1.x*arrayImageIntegralY.Element(piPos);
		else {
			// If image integrals have not been computed, apply "brutal" discretization
			// of Green's theorem (see first equation of section 5.6 in [Mille09] for the grayscale equivalent)
			fcolSumX.Set(0.0f, 0.0f, 0.0f);
			piIntegral.y = piPos.y;
			for (piIntegral.x=0; piIntegral.x<=piPos.x; piIntegral.x++)
				fcolSumX += arrayImage.Element(piIntegral);

			fcolSumY.Set(0.0f, 0.0f, 0.0f);
			piIntegral.x = piPos.x;
			for (piIntegral.y=0; piIntegral.y<=piPos.y; piIntegral.y++)
				fcolSumY += arrayImage.Element(piIntegral);

			fcolInnerSum += vfDerivative1.y*fcolSumX - vfDerivative1.x*fcolSumY;
		}
	}

	fInnerRegionArea*=0.5f;
	fOuterRegionArea = (float)(iWidth*iHeight)-fInnerRegionArea;

	fcolInnerSum*=0.5f;
	fcolOuterSum = fcolImageSum - fcolInnerSum;
	fcolInnerMean = fcolInnerSum/fInnerRegionArea;
	fcolOuterMean = fcolOuterSum/fOuterRegionArea;
}

void CActiveContourColor::ComputeBandMeans()
{
	list<CVertex2D>::const_iterator itVertex, itVertexPrev, itVertexNext;
	CCouple<float> vfDerivative1, vfDerivative2;
	float fThickness, fBandThickness = (float)iBandThickness;
	float fLengthElement, fLengthElementParallelCurve, fCurvature;
	CTriplet<float> fPixel;

	if (listVertices.size()<3)
		return;

	fInnerNarrowBandArea = 0.0f;
	fOuterNarrowBandArea = 0.0f;

	fcolInnerNarrowBandMean.Set(0.0f, 0.0f, 0.0f);
	fcolOuterNarrowBandMean.Set(0.0f, 0.0f, 0.0f);

	// This loop implements the computation of average colors on the narrow bands,
	// according to the template expression in equation (51) in [Mille09]
	for (itVertex=listVertices.begin(); itVertex!=listVertices.end(); itVertex++)
	{
		if (itVertex==listVertices.begin())
			itVertexPrev = --listVertices.end();
		else {
			itVertexPrev = itVertex;
			itVertexPrev--;
		}

		itVertexNext = itVertex;
		itVertexNext++;
		if (itVertexNext==listVertices.end())
			itVertexNext = listVertices.begin();

		// Velocity vector at current vertex: centered finite difference approximation
		// of curve first-order derivative
		// Could also use backward finite difference : itVertex->pfPos - itVertexPrev->pfPos ?
		vfDerivative1 = (itVertexNext->pfPos-itVertexPrev->pfPos)*0.5f;

		// Finite difference approximation of curve second-order derivative
		vfDerivative2 = itVertexPrev->pfPos + itVertexNext->pfPos - 2.0f*itVertex->pfPos;

		// Discrete length element, denoted \ell_i in [Mille09] (continuous definition is in equation (9) )
		fLengthElement = vfDerivative1.L2Norm();

		// Discrete curvature, denoted \kappa_i in [Mille09] (continuous definition is given shortly after length element)
		fCurvature = (vfDerivative1.x * vfDerivative2.y - vfDerivative1.y * vfDerivative2.x)/(fLengthElement*fLengthElement*fLengthElement);

		for (fThickness=0; fThickness<fBandThickness; fThickness+=1.0f)
		{
			fLengthElementParallelCurve = fLengthElement * (1.0f - fThickness*fCurvature);

			// The following test is related to regularity condition (12) in [Mille09]
			// If it is false, the thickness exceeds the radius of curvature, yielding a singularity on the
			// inner parallel curve. In this case, it seems reasonable not to include the resulting pixel
			// in the inner average color
			if (fLengthElementParallelCurve>0.0f)
			{
				fPixel = arrayImage.GetElementInterpolate(itVertex->pfPos + fThickness*itVertex->vfNormal);
				fcolInnerNarrowBandMean += fPixel*fLengthElementParallelCurve;
				fInnerNarrowBandArea += fLengthElementParallelCurve;
			}
		}

		for (fThickness=1.0f; fThickness<=fBandThickness; fThickness+=1.0f)
		{
			fLengthElementParallelCurve = fLengthElement * (1.0f + fThickness*fCurvature);

			// The following test is related to regularity condition (12) in [Mille09]
			// If it is false, the thickness exceeds the radius of curvature, yielding a singularity on the
			// outer parallel curve. In this case, it seems reasonable not to include the resulting pixel
			// in the outer average color
			if (fLengthElementParallelCurve>0.0f)
			{
				fPixel = arrayImage.GetElementInterpolate(itVertex->pfPos - fThickness*itVertex->vfNormal);
				fcolOuterNarrowBandMean += fPixel*fLengthElementParallelCurve;
				fOuterNarrowBandArea += fLengthElementParallelCurve;
			}
		}
	}

	fcolInnerNarrowBandMean /= fInnerNarrowBandArea;
	fcolOuterNarrowBandMean /= fOuterNarrowBandArea;
}

CCouple<float> CActiveContourColor::RegionForce(const CVertex2D *pVertexPrev, const CVertex2D *pVertex, const CVertex2D *pVertexNext) const
{
	// Region terms
	float fDiffRegion, fDiffRegionInside, fDiffRegionOutside;

	// Band thickness stored as a real number (make the conversion once)
	float fBandThickness = (float)iBandThickness;

	// Color at current vertex
	CTriplet<float> fPixel;

	// Inner and outer color region descriptors
	CTriplet<float> fcolInnerMeanCurrent(0.0f, 0.0f, 0.0f), fcolOuterMeanCurrent(0.0f, 0.0f, 0.0f);

	// For computing the bias force
	float fCoefBias, fForceBias, fDiffInsideOutside;

	fPixel = arrayImage.GetElementInterpolate(pVertex->pfPos);

	// Choose appropriate inner region descriptor depending on iInsideMode
	if (iInsideMode==REGION)
		fcolInnerMeanCurrent = fcolInnerMean;
	else if (iInsideMode==REGION_INIT)
		fcolInnerMeanCurrent = fcolInnerInitialMean;
	else if (iInsideMode==BAND)
		fcolInnerMeanCurrent = fcolInnerNarrowBandMean;

	// Choose appropriate outer region descriptor depending on iOutsideMode
	if (iOutsideMode==REGION)
		fcolOuterMeanCurrent = fcolOuterMean;
	else if (iOutsideMode==BAND)
		fcolOuterMeanCurrent = fcolOuterNarrowBandMean;
	else if (iOutsideMode==LINE)
	{
		float fThickness;
		float fLengthElement, fWeightCurvature, fCurvature, fSumWeights;
		CCouple<float> vfDerivative1, vfDerivative2;

		// Velocity vector at current vertex: centered finite difference approximation
		// of curve first-order derivative
		// Could also use backward finite difference : itVertex->pfPos - itVertexPrev->pfPos ?
		vfDerivative1 = (pVertexNext->pfPos-pVertexPrev->pfPos)*0.5f;

		// Finite difference approximation of curve second-order derivative
		vfDerivative2 = pVertexPrev->pfPos + pVertexNext->pfPos - 2.0f*pVertex->pfPos;

		// Discrete length element, denoted \ell_i in [Mille09] (continuous definition is in equation (9) )
		fLengthElement = vfDerivative1.L2Norm();

		// Discrete curvature, denoted \kappa_i in [Mille09] (continuous definition is given shortly after length element)
		fCurvature = (vfDerivative1.x * vfDerivative2.y - vfDerivative1.y * vfDerivative2.x)/(fLengthElement*fLengthElement*fLengthElement);

		// Average weighted color along the outward normal line segment of length equal to band thickness
		// Implements the computation of \mu_NL appearing just after equation (54) in [Mille09]
		fcolOuterMeanCurrent.Set(0.0f, 0.0f, 0.0f);
		fSumWeights = 0.0f;
		for (fThickness=1.0f; fThickness<=fBandThickness; fThickness+=1.0f)
		{
			fWeightCurvature = 1.0f + fThickness*fCurvature; // Implements (1+b\kappa_i)

			// The following test is related to regularity condition (12) in [Mille09]
			// If it is false, the thickness exceeds the radius of curvature, yielding a singularity on the
			// outer parallel curve. In this case, it seems reasonable not to include the resulting pixel
			if (fWeightCurvature>0.0f)
			{
				fcolOuterMeanCurrent += arrayImage.GetElementInterpolate(pVertex->pfPos - fThickness*pVertex->vfNormal)*fWeightCurvature;
				fSumWeights += fWeightCurvature;
			}
		}

		if (fSumWeights==0.0f)
		{
			// This happens if the curve is excessively concave at the current vertex
			// Return a null force, so that only regularization is effective on this particular vertex
			return CCouple<float>(0.0f, 0.0f);
		}
		else
			fcolOuterMeanCurrent /= fSumWeights;
	}

	// Unlike in [Mille09], deviations with respect to inner and outer descriptors
	// are computed using the L2 norm (not the squared one)
	// This yields better behaviour with respect to local color differences
	fDiffRegionInside = (fcolInnerMeanCurrent-fPixel).L2Norm();
	fDiffRegionOutside = (fcolOuterMeanCurrent-fPixel).L2Norm();

	// The narrow band region energies introduced in [Mille09] are symmetric, i.e. inner and outer region terms
	// are identically weighted However, an asymmetric configuration is allowed here, in order to favor
	// minimization of color deviation inside or outside. This enables to implement the parametric equivalent
	// of the Chan-Vese model (which is asymmetric) for comparison purpose.
	// The initial configuration is symmetric, since the default value of fWeightRegionInOverOut is 0.5
	fDiffRegion = fWeightRegionInOverOut * fDiffRegionInside - (1.0f-fWeightRegionInOverOut) * fDiffRegionOutside;

	// If bias is enabled, a balloon-like force is added to the region force
	if (bRegionEnergyBias==true)
	{
		// This is slightly different from the bias force described in section 5.5 in [Mille09],
		// as the magnitude of the bias is set by the balloon weight (which should be negative)
		fForceBias = fWeightBalloon+(fcolInnerMeanCurrent-fPixel).L2Norm();

		// Compute bias coefficient, denoted \gamma and introduced in equation (56) in [Mille09]
		// It decreases exponentially with respect to the difference between inner and outer region descriptors
		fDiffInsideOutside = (fcolInnerMeanCurrent-fcolOuterMeanCurrent).L2Norm();
		fCoefBias = (1.0f-fDiffInsideOutside)/(1.0f+50.0f*fDiffInsideOutside);

		// Linear combination of bias and region forces
		// See second part of equation (56)
		fDiffRegion = fCoefBias*fForceBias + (1.0f-fCoefBias)*fDiffRegion;
	}

	return pVertex->vfNormal*fDiffRegion;
}

void CActiveContourColor::UpdateAfterInitialization()
{
	if (fWeightRegion!=0.0f)
	{
		if (iInsideMode==REGION || iInsideMode==REGION_INIT ||iOutsideMode==REGION)
		{
			ComputeRegionMeansGreenTheorem();
			if (iInsideMode==REGION_INIT)
				fcolInnerInitialMean = fcolInnerMean;
		}

		if (iInsideMode==BAND || iOutsideMode==BAND)
			ComputeBandMeans();
	}
}
