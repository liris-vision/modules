/*
	binaryregion.h

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

	The current source file corresponds to a binary version of the level set active
	contour model. See sections 2 and 6 in [Mille09] for the continuous variational
	model and its implicit implementation.
*/

#include "binaryregion.h"
#include "cvmat2array.h"
#include <string.h>
#include <float.h>
#include <iostream>
#include <list>
#include <vector>

using namespace std;

/**********************************************************************
*                      CBinaryRegionBase                              *
**********************************************************************/

CArray1D<CCouple<int> > CBinaryRegionBase::vectNeighborhood;
CArray1D<CCouple<int> > CBinaryRegionBase::vectNeighborhoodDistance;
CArray1D<float> CBinaryRegionBase::vectNeighborhoodWeightsDistance;
const int CBinaryRegionBase::iNeighborhoodDistanceHalfWidth = 2;

CBinaryRegionBase::CBinaryRegionBase():CDeformableModelBase()
{
	// Region is empty
	iNbPixels = 0;

	// Draw parameters
	fDisplayOpacity = 0.5f;

	bDisplayRegionPixels = true;
	bDisplayBand = true;
	bDisplayBoundary = true;
	bDisplayNormals = false;
	rgbRegionColor.Set(255,0,0);
	rgbBandColor.Set(0,0,255);

	// Maximum number of previous states of the region to be stored
	iNbPreviousRegionsMax = 10;
}

void CBinaryRegionBase::InitNeighborhood()
{
	CCouple<int> piNeighbor;
	int iNeighbor, iNeighborhoodDistanceSize;

	// Initialize 4-connexity relative neighborhood
	vectNeighborhood.Init(4);
	vectNeighborhood[0].Set(-1,0);
	vectNeighborhood[1].Set(1,0);
	vectNeighborhood[2].Set(0,-1);
	vectNeighborhood[3].Set(0,1);

	// Initialize relative neighborhood for signed euclidean distance approximation
	iNeighborhoodDistanceSize = (2*iNeighborhoodDistanceHalfWidth+1)*(2*iNeighborhoodDistanceHalfWidth+1);

	vectNeighborhoodDistance.Init(iNeighborhoodDistanceSize);
	vectNeighborhoodWeightsDistance.Init(iNeighborhoodDistanceSize);

	iNeighbor = 0;
	for (piNeighbor.y=-iNeighborhoodDistanceHalfWidth; piNeighbor.y<=iNeighborhoodDistanceHalfWidth; piNeighbor.y++)
	{
		for (piNeighbor.x=-iNeighborhoodDistanceHalfWidth; piNeighbor.x<=iNeighborhoodDistanceHalfWidth; piNeighbor.x++)
		{
			vectNeighborhoodDistance[iNeighbor] = piNeighbor;
			vectNeighborhoodWeightsDistance[iNeighbor] = sqrt((float)piNeighbor.L2Norm2());
			iNeighbor++;
		}
	}
}

void CBinaryRegionBase::InitNeighborhoodOffsets()
{
	int iNeighbor;

	// Initialize set of offsets of 4-connexity relative neighborhood
	vectNeighborhoodOffsets.Init(vectNeighborhood.GetSize());
	for (iNeighbor=0; iNeighbor<vectNeighborhood.GetSize(); iNeighbor++)
		vectNeighborhoodOffsets[iNeighbor] = GetOffset(vectNeighborhood[iNeighbor]);

	// Initialize set of offsets of relative neighborhood for signed euclidean distance approximation
	vectNeighborhoodOffsetsDistance.Init(vectNeighborhoodDistance.GetSize());
	for (iNeighbor=0; iNeighbor<vectNeighborhoodDistance.GetSize(); iNeighbor++)
		vectNeighborhoodOffsetsDistance[iNeighbor] = GetOffset(vectNeighborhoodDistance[iNeighbor]);
}

// Initialize band neighborhood: circular relative neighborhood of radius iBandThickness and corresponding offsets
void CBinaryRegionBase::InitNeighborhoodsSmoothingBand()
{
	// Static variables : neighborhoods, offsets and previous band thickness
	list<CCouple<int> > listNeighborhoodBand;
	list<CCouple<int> >::const_iterator itNeighbor;
	CCouple<int> piNeighbor;
	unsigned int i;

	// Smoothing neighborhood
	if (fStdDeviationGaussianFilter!=0.0f)
	{
		CArray2D<float> arrayGaussian;
		CCouple<int> *pNeighbor;
		float *pGaussian, *pWeight;
		int *pOffset;

		// Generate 2D array holding gaussian weights
		arrayGaussian.SetGaussianKernel(fStdDeviationGaussianFilter);

		// Get relative neighbors, offsets and weights in vectors
		vectNeighborhoodSmoothing.Init(arrayGaussian.GetWidth()*arrayGaussian.GetHeight());
		vectNeighborhoodOffsetsSmoothing.Init(vectNeighborhoodSmoothing.GetSize());
		vectNeighborhoodWeightsSmoothing.Init(vectNeighborhoodSmoothing.GetSize());

		pNeighbor = vectNeighborhoodSmoothing.GetBuffer();
		pGaussian = arrayGaussian.GetBuffer();
		pOffset = vectNeighborhoodOffsetsSmoothing.GetBuffer();
		pWeight = vectNeighborhoodWeightsSmoothing.GetBuffer();

		for (piNeighbor.y=-arrayGaussian.GetHeight()/2; piNeighbor.y<=arrayGaussian.GetHeight()/2; piNeighbor.y++)
		{
			for (piNeighbor.x=-arrayGaussian.GetWidth()/2; piNeighbor.x<=arrayGaussian.GetWidth()/2; piNeighbor.x++)
			{
				*pNeighbor = piNeighbor;
				*pWeight = *pGaussian;
				*pOffset = GetOffset(*pNeighbor);
				pNeighbor++;
				pGaussian++;
				pOffset++;
				pWeight++;
			}
		}
	}

	// Band neighborhood
	// Fill list with points in circular relative neighborhood of radius iBandThickness
	for (piNeighbor.y=-iBandThickness;piNeighbor.y<=iBandThickness;piNeighbor.y++)
		for (piNeighbor.x=-iBandThickness;piNeighbor.x<=iBandThickness;piNeighbor.x++)
			if (piNeighbor.L2Norm2()<=iBandThickness*iBandThickness)
				listNeighborhoodBand.push_back(piNeighbor);

	// Convert list to vector and compute offsets
	vectNeighborhoodBand.Init(listNeighborhoodBand.size());
	vectNeighborhoodOffsetsBand.Init(listNeighborhoodBand.size());
	for (i=0, itNeighbor=listNeighborhoodBand.begin(); i<listNeighborhoodBand.size(); i++, itNeighbor++)
	{
		vectNeighborhoodBand[i] = *itNeighbor;
		vectNeighborhoodOffsetsBand[i] = GetOffset(*itNeighbor);
	}
}

bool CBinaryRegionBase::EnergyGradientDescent(int iNbPasses)
{
	if (iNbPixels==0)
		return false;

	CArray2DIterator<CBinaryRegionPixel> itPixel;
	CBinaryRegionPixel *pPixel;
	CCouple<int> piCurrent, piNeighbor, piMin, piMax, piMinScan, piMaxScan;
	static vector<CCouple<int> > listPixelsToAdd, listPixelsToRemove;
	static bool bFirstCall = true;
	int iPass, iNeighbor;
	unsigned int iPixel;
	float fSpeed, fSpeedGradient, fSpeedSmoothness, fSpeedRegion;
	bool bEvolve;

	// Make room for pixels candidate to addition or removal
	if (bFirstCall==true)
	{
		listPixelsToAdd.reserve(iWidth*iHeight);
		listPixelsToRemove.reserve(iWidth*iHeight);
	}

	// For now, no pixel should be marked for smoothing
	if (bUseGaussianFilter==true && fStdDeviationGaussianFilter!=0.0f)
	{
		for (itPixel = GetIterator(); !itPixel.End(); itPixel++)
			itPixel.Element().UnmarkForSmoothing();
	}

    // If stopping criterion is enabled, the current state of the region should be stored
    if (bStoppingCriterion==true)
    {
        cv::Mat imgMaskCurrent;

        MakeBinaryMask(imgMaskCurrent);
        listPreviousRegions.push_back(imgMaskCurrent.clone());
        if (listPreviousRegions.size()>iNbPreviousRegionsMax)
            listPreviousRegions.pop_front();
    }

	bFirstCall = false;
	bEvolve = true;

	piMin.Set(0, 0);
	piMax.Set(iWidth-1, iHeight-1);

	for (iPass=0; iPass<iNbPasses && bEvolve==true; iPass++)
	{
		// Define the rectangle area in which candidate pixels will be looked for
		piMinScan = coupleMax(piMin, piRegionMin-CCouple<int>(1,1));
		piMaxScan = coupleMin(piMax, piRegionMax+CCouple<int>(1,1));

		for (itPixel = GetIterator(piMinScan, piMaxScan); !itPixel.End(); itPixel++)
		{
			piCurrent = itPixel.GetPosition();

			// Region evolves with respect to the narrow band technique, with a 1-pixel wide band
			// both sides apart from the contour, so that candidate pixels should be located on the
			// inner or outer boundary of the region
			if (IsCandidatePixel(itPixel.ElementPtr())==true)
			{
				// Current pixel is candidate to be added to or removed from the region,
				// so compute its speed
				fSpeed = 0.0f;

				// Smoothness
				if (fWeightSmoothness!=0.0f && bUseGaussianFilter==false)
				{
					// Laplacian smooth
					if (itPixel.ElementPtr()->IsInGrowSafeArea())
					{
						float fAverage = 0.0f;

						pPixel = itPixel.ElementPtr();

						if (pPixel[-1].IsInRegion())
							fAverage+=0.25f;
						if (pPixel[1].IsInRegion())
							fAverage+=0.25f;
						if (pPixel[-iWidth].IsInRegion())
							fAverage+=0.25f;
						if (pPixel[iWidth].IsInRegion())
							fAverage+=0.25f;

						fSpeedSmoothness = 1.0f-2.0f*fAverage;
					}
					else fSpeedSmoothness = 0.0f;

					fSpeed += fWeightSmoothness*fSpeedSmoothness;
				}

				// Region
				if (fWeightRegion!=0.0f)
				{
					fSpeedRegion = RegionSpeed(piCurrent);
					fSpeed += fWeightRegion*fSpeedRegion;
				}

				// Gradient
				if (fWeightGradient!=0.0f)
				{
					fSpeedGradient = arrayGradientNorm.Element(piCurrent);
					if (fWeightBalloon!=0.0f)
						// If balloon speed is enabled, edge speed should oppose it
						fSpeed += -fWeightBalloon/fabs(fWeightBalloon)*fWeightGradient*fSpeedGradient;
					else
						fSpeed += fWeightGradient*fSpeedGradient;
				}

				// Balloon
				if (fWeightBalloon!=0.0f && (fWeightRegion==0.0f || bRegionEnergyBias==false))
					fSpeed += fWeightBalloon;

				// Pixel is candidate to addition or removal regarding on the sign of its speed
				// Unlike in the evolution equation (57) in [Mille09], speed magnitude is not important here
				if (itPixel.Element().IsInRegion()==false && fSpeed<0.0f)
					listPixelsToAdd.push_back(piCurrent);
				else if (itPixel.Element().IsInRegion()==true && fSpeed>0.0f)
					listPixelsToRemove.push_back(piCurrent);
			}
		}

		if (listPixelsToAdd.size()!=0 || listPixelsToRemove.size()!=0)
		{
			for (iPixel=0; iPixel<listPixelsToAdd.size(); iPixel++)
				AddPixel(listPixelsToAdd[iPixel]);

			for (iPixel=0; iPixel<listPixelsToRemove.size(); iPixel++)
				RemovePixel(listPixelsToRemove[iPixel]);

			UpdateAfterEvolution();

			if (bUseGaussianFilter==true && fStdDeviationGaussianFilter!=0.0f)
			{
				// Scan neighbors of new and former pixels and mark them for smoothing
				for (iPixel=0; iPixel<listPixelsToAdd.size(); iPixel++)
				{
					pPixel = GetBuffer() + GetOffset(listPixelsToAdd[iPixel]);
					if (pPixel->IsInSmoothingSafeArea())
					{
						// Scan neighborhood without checking
						for (iNeighbor=0; iNeighbor<vectNeighborhoodSmoothing.GetSize(); iNeighbor++)
							pPixel[vectNeighborhoodOffsetsSmoothing[iNeighbor]].MarkForSmoothing();
					}
					else {
						// Scan neighborhood by checking if neighbors are within image domain
						for (iNeighbor=0; iNeighbor<vectNeighborhoodSmoothing.GetSize(); iNeighbor++)
						{
							piNeighbor = listPixelsToAdd[iPixel]+vectNeighborhoodSmoothing[iNeighbor];
							if (piNeighbor.IsInRange(piMin, piMax))
								pPixel[vectNeighborhoodOffsetsSmoothing[iNeighbor]].MarkForSmoothing();
						}
					}
				}

				for (iPixel=0; iPixel<listPixelsToRemove.size(); iPixel++)
				{
					pPixel = GetBuffer() + GetOffset(listPixelsToRemove[iPixel]);
					if (pPixel->IsInSmoothingSafeArea())
					{
						// Scan smoothing neighborhood without checking
						for (iNeighbor=0; iNeighbor<vectNeighborhoodSmoothing.GetSize(); iNeighbor++)
							pPixel[vectNeighborhoodOffsetsSmoothing[iNeighbor]].MarkForSmoothing();
					}
					else {
						// Scan smoothing neighborhood by checking if neighbors are within image domain
						for (iNeighbor=0; iNeighbor<vectNeighborhoodSmoothing.GetSize(); iNeighbor++)
						{
							piNeighbor = listPixelsToRemove[iPixel] + vectNeighborhoodSmoothing[iNeighbor];
							if (piNeighbor.IsInRange(piMin, piMax))
								pPixel[vectNeighborhoodOffsetsSmoothing[iNeighbor]].MarkForSmoothing();
						}
					}
				}
			}

			listPixelsToAdd.clear();
			listPixelsToRemove.clear();
		}
		else
			bEvolve = false;
	}

	if (bUseGaussianFilter==true && fStdDeviationGaussianFilter!=0.0f)
		GaussianSmoothMarked();

    if (bStoppingCriterion==true)
    {
        // To detect that the binary region has reached stability, we compute the minimal difference area
        // between the current region and the previous ones.
        // In other words, this is the area of the intersection of all symmetric difference sets, where the i^th
        // difference set is the difference between the current region and the i^th previous region
        // Hence, a pixel x in the current region (resp. background) contributes to the difference only if it
        // was not in any previous region (resp. background).

        // Doing so, the detection of convergence is robust against the oscillations that may arise around
        // the local minimum, particularly when the number of passes per iteration is high
        list<cv::Mat>::const_iterator itPreviousRegion;
        cv::Mat imgDiff, imgDiffMin, imgMask;

        int x, y, iNbDiff;

        imgDiffMin.create(iHeight, iWidth, CV_8UC1);
        imgDiffMin.setTo(255);

        MakeBinaryMask(imgMask);
        for (itPreviousRegion=listPreviousRegions.begin(); itPreviousRegion!=listPreviousRegions.end(); itPreviousRegion++)
        {
            cv::absdiff(imgMask, *itPreviousRegion, imgDiff);
            //imgDiff = imgMask.AbsDiff(*itPreviousRegion);
            // imgDiffMin = imgDiffMin.Min(imgDiff);
            imgDiffMin = cv::min(imgDiffMin, imgDiff);
        }

        iNbDiff = 0;
        for (y=0; y<iHeight; y++)
            for (x=0; x<iWidth; x++)
                if (imgDiffMin.at<unsigned char>(y,x)==255)
                    iNbDiff++;

        if (iNbDiff==0)
            return false;
    }

    return true;
}

void CBinaryRegionBase::GaussianSmooth()
{
	unsigned int i;
	vector<CCouple<int> > listPixelsToRemove, listPixelsToAdd;
	CArray2DIterator<CBinaryRegionPixel> itPixel;
	CBinaryRegionPixel *pPixel, *pPixelNeighbor;
	CCouple<int> p, pNeighbor, pMin, pMax;
	float fSumWeights;
	int iNeighbor;

	if (iNbPixels==0)
		return;

	listPixelsToRemove.reserve(iNbPixels);
	listPixelsToAdd.reserve(iNbPixels);

	pMin.Set(0,0);
	pMax.Set(iWidth-1, iHeight-1);

	for (itPixel = GetIterator(piRegionMin, piRegionMax); !itPixel.End(); itPixel++)
	{
		pPixel = itPixel.ElementPtr();
		p = itPixel.GetPosition();

		fSumWeights = 0.0f;

		if (pPixel->IsInSmoothingSafeArea())
		{
			// Safety area -> do not check neighbors
			for (iNeighbor=0; iNeighbor<vectNeighborhoodSmoothing.GetSize(); iNeighbor++)
			{
				if ((pPixel+vectNeighborhoodOffsetsSmoothing[iNeighbor])->IsInRegion()==true)
					fSumWeights += vectNeighborhoodWeightsSmoothing[iNeighbor];
			}
		}
		else {
			// Not in safety area -> check if neighbors fall outside the image domain
			for (iNeighbor=0; iNeighbor<vectNeighborhoodSmoothing.GetSize(); iNeighbor++)
			{
				pNeighbor = (p+vectNeighborhoodSmoothing[iNeighbor]);
				pNeighbor.LimitWithinRange(pMin, pMax);

				pPixelNeighbor = pElements + GetOffset(pNeighbor);
				if (pPixelNeighbor->IsInRegion()==true)
					fSumWeights += vectNeighborhoodWeightsSmoothing[iNeighbor];
			}
		}

		if (pPixel->IsInRegion()==true && fSumWeights<0.5f)
			listPixelsToRemove.push_back(p);
		else if (pPixel->IsInRegion()==false && fSumWeights>=0.5f)
			listPixelsToAdd.push_back(p);
	}

	if (listPixelsToRemove.size()!=0 || listPixelsToAdd.size()!=0)
	{
		for (i=0; i<listPixelsToRemove.size(); i++)
			RemovePixel(listPixelsToRemove[i]);

		for (i=0; i<listPixelsToAdd.size(); i++)
			AddPixel(listPixelsToAdd[i]);

		UpdateAfterEvolution();
	}
}

void CBinaryRegionBase::GaussianSmoothMarked()
{
	unsigned int i;
	vector<CCouple<int> > listPixelsToRemove, listPixelsToAdd;
	CArray2DIterator<CBinaryRegionPixel> itPixel;
	CBinaryRegionPixel *pPixel, *pPixelNeighbor;
	CCouple<int> p, pNeighbor, pMin, pMax;
	float fSumWeights;
	int iNeighbor;

	if (iNbPixels==0)
		return;

	listPixelsToRemove.reserve(iNbPixels);
	listPixelsToAdd.reserve(iNbPixels);

	pMin.Set(0,0);
	pMax.Set(iWidth-1, iHeight-1);

	for (itPixel = GetIterator(piRegionMin, piRegionMax); !itPixel.End(); itPixel++)
	{
		pPixel = itPixel.ElementPtr();

		if (pPixel->IsMarkedForSmoothing())
		{
			// Current pixel is marked, so scan its smoothing neighborhood
			p = itPixel.GetPosition();

			fSumWeights = 0.0f;

			if (pPixel->IsInSmoothingSafeArea())
			{
				// Safety area -> do not check neighbors
				for (iNeighbor=0; iNeighbor<vectNeighborhoodSmoothing.GetSize(); iNeighbor++)
				{
					if ((pPixel+vectNeighborhoodOffsetsSmoothing[iNeighbor])->IsInRegion()==true)
						fSumWeights += vectNeighborhoodWeightsSmoothing[iNeighbor];
				}
			}
			else {
				// Not in safety area -> check if neighbors fall outside the image domain
				for (iNeighbor=0; iNeighbor<vectNeighborhoodSmoothing.GetSize(); iNeighbor++)
				{
					pNeighbor = (p+vectNeighborhoodSmoothing[iNeighbor]);
					pNeighbor.LimitWithinRange(pMin, pMax);

					pPixelNeighbor = pElements + GetOffset(pNeighbor);
					if (pPixelNeighbor->IsInRegion()==true)
						fSumWeights += vectNeighborhoodWeightsSmoothing[iNeighbor];
				}
			}

			if (pPixel->IsInRegion()==true && fSumWeights<0.5f)
				listPixelsToRemove.push_back(p);
			else if (pPixel->IsInRegion()==false && fSumWeights>=0.5f)
				listPixelsToAdd.push_back(p);
		}
	}

	if (listPixelsToRemove.size()!=0 || listPixelsToAdd.size()!=0)
	{
		for (i=0; i<listPixelsToRemove.size(); i++)
			RemovePixel(listPixelsToRemove[i]);

		for (i=0; i<listPixelsToAdd.size(); i++)
			AddPixel(listPixelsToAdd[i]);

		UpdateAfterEvolution();
	}
}

void CBinaryRegionBase::GetNormalAndCurvature(const CCouple<int> &p, CCouple<float> &vfNormal, float &fCurvature) const
{
	float *pDistance, fNormalNorm;
	float fDerX, fDerY, fDerX2, fDerY2, fDerXY, fCurvatureNum, fCurvatureDen;

	// Approximate first and second-order derivatives by centered finite differences
	if (Element(p).IsInGrowSafeArea())
	{
		// Pointer to distance value at current point
		pDistance = arraySignedDistance.GetBuffer() + arraySignedDistance.GetOffset(p);

		fDerX = (pDistance[1]-pDistance[-1])*0.5f;
		fDerY = (pDistance[iWidth]-pDistance[-iWidth])*0.5f;
		fDerX2 = pDistance[1]-2.0f*pDistance[0]+pDistance[-1];
		fDerY2 =  pDistance[iWidth]-2.0f*pDistance[0]+pDistance[-iWidth];
		fDerXY = (pDistance[iWidth+1] - pDistance[-iWidth+1]
			- pDistance[iWidth-1] + pDistance[-iWidth-1])*0.25f;

		// Compute numerator and denominator of curvature quotient
		fCurvatureNum = fDerX2*(fDerY*fDerY) + fDerY2*(fDerX*fDerX) - 2.0f*fDerX*fDerY*fDerXY;
		fCurvatureDen = (fDerX*fDerX + fDerY*fDerY);
	}
	else {
		// If current pixel is located on image border, just set its normal and curvature to zero
		fDerX = 0.0f;
		fDerY = 0.0f;
		fCurvatureNum = 0.0f;
		fCurvatureDen = 0.0f;
	}

	// Euclidean distance function is negative inside and positive outside,
	// so the inward normal is given by the negative gradient
	vfNormal.x =-fDerX;
	vfNormal.y =-fDerY;

	fNormalNorm = vfNormal.L2Norm();
	if (fNormalNorm!=0.0f)
		vfNormal/=fNormalNorm;

	if (fCurvatureDen!=0.0f)
		fCurvature = fCurvatureNum/fCurvatureDen;
	else
		fCurvature = 0.0f;
}

void CBinaryRegionBase::InitCircle(const CCouple<float> &pfCenter, float fRadius)
{
	CCouple<int> piCenter, p, pmin, pmax;
	int iRadius, y2;

	Empty();

	if (pInputImage==NULL)
	{
		cerr<<"ERROR in CBinaryRegionBase::InitCircle(...): cannot initialize the region: not attached to image. Call AttachImage() before"<<endl;
		return;
	}

	piCenter = (CCouple<int>)pfCenter;
	iRadius = (int)fRadius;

	if (piCenter.y<iRadius)
		pmin.y = -piCenter.y;
	else pmin.y = -iRadius;

	if (piCenter.y+iRadius>=iHeight)
		pmax.y = iHeight-piCenter.y-1;
	else pmax.y = iRadius;

	// Fill circle
	for (p.y=pmin.y; p.y<=pmax.y; p.y++)
	{
		y2 = (int)sqrt((float)(iRadius*iRadius-p.y*p.y));
		if (piCenter.x<y2)
			pmin.x = -piCenter.x;
		else pmin.x = -y2;

		if (piCenter.x+y2>=iWidth)
			pmax.x = iWidth-piCenter.x-1;
		else pmax.x = y2;

		for (p.x=pmin.x; p.x<=pmax.x; p.x++)
			AddPixel(piCenter + p);
	}

	// Check if region is not empty
	if (iNbPixels==0)
		cerr<<"WARNING in CBinaryRegionBase::InitCircle(...): bad center or radius. Yields empty region."<<endl;

	UpdateAfterInitialization();
}

void CBinaryRegionBase::InitEllipse(const CCouple<float> &pfCenter, float fRadiusX, float fRadiusY)
{
	CCouple<int> piCenter, p, pmin, pmax;
	int iRadiusX, iRadiusY, y2;

	piCenter = (CCouple<int>)pfCenter;
	iRadiusX = (int)fRadiusX;
	iRadiusY = (int)fRadiusY;

	Empty();

	if (pInputImage==NULL)
	{
		cerr<<"ERROR in CBinaryRegionBase::InitEllipse(...): cannot initialize the region: not attached to image. Call AttachImage() before"<<endl;
		return;
	}

	if (piCenter.y<iRadiusY)
		pmin.y = -piCenter.y;
	else pmin.y = -iRadiusY;

	if (piCenter.y+iRadiusY>=iHeight)
		pmax.y = iHeight-piCenter.y-1;
	else pmax.y = iRadiusY;

	// Fill circle
	for (p.y=pmin.y; p.y<=pmax.y; p.y++)
	{
		y2 = (int)sqrt((float)((iRadiusY*iRadiusY-p.y*p.y)*iRadiusX)/(float)iRadiusY);

		if (piCenter.x<y2)
			pmin.x = -piCenter.x;
		else pmin.x = -y2;

		if (piCenter.x+y2>=iWidth)
			pmax.x = iWidth-piCenter.x-1;
		else pmax.x = y2;

		for (p.x=pmin.x; p.x<=pmax.x; p.x++)
			AddPixel(piCenter + p);
	}

	// Check if region is not empty
	if (iNbPixels==0)
		cerr<<"WARNING in CBinaryRegionBase::InitEllipse(...): bad center or radii. Yields empty region."<<endl;

	UpdateAfterInitialization();
}

void CBinaryRegionBase::DrawInImageRGB(cv::Mat &im, int iZoom) const
{
	const CBinaryRegionPixel *pPixel;
	// CImage2DFloatRGBPixel
	CTriplet<float> frgbRegion, frgbBand, frgbImage, frgbMix;
	CCouple<int> p, p2;
	// int i, wdec;
	// int iBytesPerPixel = im.GetBitsPerPixel()/8;
	// unsigned char *lp_src, *lp_src2;
	// CCouple<int> pMin, pMax;
	int iOffsetEndRow;

	if (im.cols!=pInputImage->cols*iZoom || im.rows!=pInputImage->rows*iZoom)
	{
		cerr<<"ERROR in CBinaryRegionBase::DrawInImageRGB(...): sizes of region and image are different"<<endl;
		return;
	}

	if (im.type()!=CV_8UC3)
	{
		cerr<<"ERROR in CBinaryRegionBase::DrawInImageRGB(...): image is not RGB"<<endl;
		return;
	}

	// wdec = im.width_step;

    //
    // CTriplet<unsigned char> rgbRegionColor, rgbBandColor;

    cv::Vec3b vecRegionColor, vecOuterBoundaryColor, vecImageColor, vecMixColor;

    vecRegionColor.val[0] = rgbRegionColor.z;
    vecRegionColor.val[1] = rgbRegionColor.y;
    vecRegionColor.val[2] = rgbRegionColor.x;

    vecOuterBoundaryColor.val[0] = 255;
    vecOuterBoundaryColor.val[1] = 255;
    vecOuterBoundaryColor.val[2] = 0;

    // imgInput.at<cv::Vec3b>(p.y, p.x) = pixelVec;

	if (bDisplayRegionPixels==true)
	{
		frgbRegion = (CTriplet<float>)rgbRegionColor;
		iOffsetEndRow = iWidth-1-piRegionMax.x+piRegionMin.x;

		pPixel = pElements + GetOffset(piRegionMin);
		// lp_src = im.GetBits() + piRegionMin.y*wdec*iZoom;
		for (p.y=piRegionMin.y; p.y<=piRegionMax.y; p.y++)
		{
			// lp_src2 = lp_src + piRegionMin.x*iBytesPerPixel*iZoom;
			for (p.x=piRegionMin.x; p.x<=piRegionMax.x; p.x++)
			{
				if (pPixel->IsInRegion())
				{
					if (bDisplayBoundary==true && pPixel->IsInInnerBoundary())
                        im.at<cv::Vec3b>(p.y, p.x) = vecRegionColor;
						// *((CImage2DByteRGBPixel *)lp_src2) = rgbRegionColor;
					else if (bDisplayBoundary==true && pPixel->IsInOuterBoundary())
						im.at<cv::Vec3b>(p.y, p.x) = vecOuterBoundaryColor;
						//*((CImage2DByteRGBPixel *)lp_src2) = rgbOuterBoundaryColor;
					else {
					    vecImageColor = im.at<cv::Vec3b>(p.y, p.x);
					    frgbImage.Set(vecImageColor.val[2], vecImageColor.val[1], vecImageColor.val[0]);
						// frgbImage.Set((float)(lp_src2[2]), (float)(lp_src2[1]), (float)(lp_src2[0]));
						frgbMix = fDisplayOpacity*frgbRegion + (1.0f-fDisplayOpacity)*frgbImage;

						vecMixColor.val[0] = frgbMix.z;
                        vecMixColor.val[1] = frgbMix.y;
                        vecMixColor.val[2] = frgbMix.x;

						im.at<cv::Vec3b>(p.y, p.x) = vecMixColor;
					}
				}
				pPixel++;
				// lp_src2+=iBytesPerPixel*iZoom;
			}

			// Copy current row to the (iZoom-1) next rows
			// for (i=1;i<iZoom;i++)
			//	memcpy(lp_src+wdec*i+piRegionMin.x*iBytesPerPixel*iZoom, lp_src+piRegionMin.x*iBytesPerPixel*iZoom, iBytesPerPixel*(piRegionMax.x-piRegionMin.x+1)*iZoom);

			pPixel += iOffsetEndRow;
			// lp_src+=wdec*iZoom;
		}
	}

    /*
	if (bDisplayBand==true && (iInsideMode==BAND || iOutsideMode==BAND))
	{
		CCouple<int> piMinBand, piMaxBand;
		CTriplet<float> fRgbInnerBand(0.0f, 255.0f, 0.0f), fRgbOuterBand(0.0f, 0.0f, 255.0f);

		piMinBand = coupleMax(CCouple<int>(0,0), piRegionMin - CCouple<int>(iBandThickness, iBandThickness));
		piMaxBand = coupleMin(CCouple<int>(iWidth-1,iHeight-1), piRegionMax + CCouple<int>(iBandThickness, iBandThickness));
		iOffsetEndRow = iWidth-1-piMaxBand.x+piMinBand.x;

		pPixel = pElements + GetOffset(piMinBand);
		lp_src = im.GetBits() + piMinBand.y*wdec*iZoom;

		frgbBand = (CImage2DFloatRGBPixel)rgbBandColor;

		for (p.y=piMinBand.y; p.y<=piMaxBand.y; p.y++)
		{
			lp_src2 = lp_src + piMinBand.x*iBytesPerPixel*iZoom;
			for (p.x=piMinBand.x; p.x<=piMaxBand.x; p.x++)
			{
				if (pPixel->IsInOuterNarrowBand())
				{
					frgbImage.Set((float)(lp_src2[2]), (float)(lp_src2[1]), (float)(lp_src2[0]));
					frgbMix = fDisplayOpacity*frgbBand + (1.0f-fDisplayOpacity)*frgbImage;
					*((CImage2DByteRGBPixel *)lp_src2) = frgbMix.ToByteRGBPixel();

					for (i=1;i<iZoom;i++)
						memcpy(lp_src2+i*iBytesPerPixel, lp_src2, iBytesPerPixel);
				}
				pPixel++;
				lp_src2+=iBytesPerPixel*iZoom;
			}

			// Copy current row to the (iZoom-1) next rows
			for (i=1;i<iZoom;i++)
				memcpy(lp_src+wdec*i+piMinBand.x*iBytesPerPixel*iZoom, lp_src+piMinBand.x*iBytesPerPixel*iZoom, iBytesPerPixel*(piMaxBand.x-piMinBand.x+1)*iZoom);

			pPixel += iOffsetEndRow;
			lp_src+=wdec*iZoom;
		}
	}
    */

    /*
	if (bDisplayNormals==true)
	{
		pPixel = pElements + GetOffset(piRegionMin);
		iOffsetEndRow = iWidth-1-piRegionMax.x+piRegionMin.x;

		for (p.y=piRegionMin.y; p.y<=piRegionMax.y; p.y++)
		{
			for (p.x=piRegionMin.x; p.x<=piRegionMax.x; p.x++)
			{
				if (pPixel->IsInInnerBoundary())
				{
					CCouple<float> vfNormal;
					CCouple<float> pfParallel;
					float fCurvature;

					GetNormalAndCurvature(p, vfNormal, fCurvature);

					pfParallel = CCouple<float>((float)p.x, (float)p.y)+ vfNormal*(float)iBandThickness;

					im.DrawLineRGB(
						CCouple<int>(p.x*iZoom+iZoom/2, p.y*iZoom+iZoom/2),
						CCouple<int>((int)(pfParallel.x*iZoom) + iZoom/2, (int)(pfParallel.y*iZoom) + iZoom/2),
						CImage2DByteRGBPixel(255,0,255)
						);
				}
				else if (IsOuterBoundaryPixel(pPixel))
				{
					CCouple<float> vfNormal;
					CCouple<float> pfParallel;
					float fCurvature;

					GetNormalAndCurvature(p, vfNormal, fCurvature);

					pfParallel = CCouple<float>((float)p.x, (float)p.y)+ vfNormal*(float)iBandThickness;

					im.DrawLineRGB(
						CCouple<int>(p.x*iZoom+iZoom/2, p.y*iZoom+iZoom/2),
						CCouple<int>((int)(pfParallel.x*iZoom) + iZoom/2, (int)(pfParallel.y*iZoom) + iZoom/2),
						CImage2DByteRGBPixel(100,100,0)
						);
				}
				pPixel++;
			}
			pPixel += iOffsetEndRow;
		}
	}
	*/
}


void CBinaryRegionBase::MakeBinaryMask(cv::Mat &imgMask) const
{
	CCouple<int> p;
	unsigned char *pBits;
	const CBinaryRegionPixel *pPixel;

	if (imgMask.rows!=iHeight || imgMask.cols!=iWidth || imgMask.type()!=CV_8UC1)
		imgMask.create(iHeight, iWidth, CV_8UC1);

	pPixel = pElements;
	for (p.y=0; p.y<iHeight; p.y++)
	{
	    pBits = imgMask.ptr(p.y);
		for (p.x=0; p.x<iWidth; p.x++)
		{
			if (pPixel->IsInRegion())
				*pBits = 255;
			else
				*pBits = 0;
            pPixel++;
			pBits++;
		}
	}
}

void CBinaryRegionBase::Empty()
{
	CBinaryRegionPixel *pPixel;
	int i;

	// Set all pixels out of region
	pPixel = pElements;
	for (i=0; i<iSize; i++)
	{
		pPixel->RemoveFromRegion();
		pPixel++;
	}

	// Reset number of pixels and bounds
	iNbPixels = 0;
	piRegionMin.Set(INT_MAX, INT_MAX);
	piRegionMax.Set(0,0);
}

/*
bool CBinaryRegionBase::IsInnerBoundaryPixel(CBinaryRegionPixel *pPixel)
{
	bool bIsBoundary = false;
	int v;

	if (pPixel->IsInRegion()==true)
	{
		v = 0;
		while (v<vectNeighborhoodOffsets.GetSize() && bIsBoundary==false)
		{
			if ((pPixel+vectNeighborhoodOffsets[v])->IsInRegion()==false)
				bIsBoundary = true;
			v++;
		}
	}
	return bIsBoundary;
}

bool CBinaryRegionBase::IsOuterBoundaryPixel(CBinaryRegionPixel *pPixel)
{
	bool bIsBoundary = false;
	int v;

	if (pPixel->IsInRegion()==false)
	{
		v = 0;
		while (v<vectNeighborhoodOffsets.GetSize() && bIsBoundary==false)
		{
			if ((pPixel+vectNeighborhoodOffsets[v])->IsInRegion()==true)
				bIsBoundary = true;
			v++;
		}
	}
	return bIsBoundary;
}

bool CBinaryRegionBase::IsCandidatePixel(CBinaryRegionPixel *pPixel)
{
	bool bIsBoundary = false;
	int v;

	v = 0;
	while (v<vectNeighborhoodOffsets.GetSize() && bIsBoundary==false)
	{
		if ((pPixel+vectNeighborhoodOffsets[v])->region!=pPixel->region)
			bIsBoundary = true;
		v++;
	}
	return bIsBoundary;
}
*/
void CBinaryRegionBase::UpdateSafeAreas()
{
	CCouple<int> piMin, piMax;
	CArray2DIterator<CBinaryRegionPixel> itPixel;
	CCouple<int> piSmoothHalfWidth;

	// Clear status of all pixels
	for (itPixel = GetIterator(); !itPixel.End(); itPixel++)
		itPixel.Element().ClearStatusByte();

	// Update safe areas
	// In these areas, pixels may be scanned without checking if neighbors fall outside the image domain

	// Add pixels into grow-safe area
	piMin.Set(1, 1);
	piMax.Set(iWidth-2, iHeight-2);

	for (itPixel = GetIterator(piMin,piMax); !itPixel.End(); itPixel++)
		itPixel.Element().AddIntoGrowSafeArea();

	// Add pixels into smoothing-safe area
	piSmoothHalfWidth = (CCouple<int>)max(1, (int)(3.0f * fStdDeviationGaussianFilter));

	piMin = piSmoothHalfWidth;
	piMax = GetSize()-CCouple<int>(1,1)-piSmoothHalfWidth;

	for (itPixel = GetIterator(piMin,piMax); !itPixel.End(); itPixel++)
		itPixel.Element().AddIntoSmoothingSafeArea();

	// Add pixels into band-safe area
	if (iOutsideMode==LINE)
	{
		piMin.Set(iBandThickness+2, iBandThickness+2);
		piMax.Set(iWidth-3-iBandThickness, iHeight-3-iBandThickness);
	}
	else {
		piMin.Set(iBandThickness, iBandThickness);
		piMax.Set(iWidth-1-iBandThickness, iHeight-1-iBandThickness);
	}

	for (itPixel = GetIterator(piMin,piMax); !itPixel.End(); itPixel++)
		itPixel.Element().AddIntoBandSafeArea();

	// Add pixels into distance-safe area
	piMin.Set(iNeighborhoodDistanceHalfWidth, iNeighborhoodDistanceHalfWidth);
	piMax.Set(iWidth-1-iNeighborhoodDistanceHalfWidth, iHeight-1-iNeighborhoodDistanceHalfWidth);

	for (itPixel = GetIterator(piMin,piMax); !itPixel.End(); itPixel++)
		itPixel.Element().AddIntoDistanceSafeArea();
}

void CBinaryRegionBase::UpdateBoundary()
{
	CBinaryRegionPixel *pPixel;
	CCouple<int> p, piMin, piMax;
	int iPixel, iSize, iOffsetEndRow, iOffsetEndColumn;
	bool bInRegion;

	// Remove all pixels from narrow band
	iSize = iWidth*iHeight;
	pPixel = pElements;

	for (iPixel=0; iPixel<iSize; iPixel++)
	{
		pPixel->RemoveFromBoundary();
		pPixel++;
	}

	piMin = coupleMax(CCouple<int>(0,0), piRegionMin - CCouple<int>(1,1));
	piMax = coupleMin(CCouple<int>(iWidth-1,iHeight-1), piRegionMax + CCouple<int>(1,1));

	// Detect horizontal edges
	pPixel = pElements + GetOffset(piMin);
	iOffsetEndRow = iWidth-1-piMax.x+piMin.x;

	for (p.y=piMin.y; p.y<=piMax.y; p.y++)
	{
		bInRegion = false;

		if (pPixel->IsInRegion()==true)
        {
            bInRegion = true;
            pPixel->AddIntoBoundary();
            if (piMin.x>0)
                (pPixel-1)->AddIntoBoundary();
        }
        pPixel++;

		for (p.x=piMin.x+1; p.x<=piMax.x; p.x++)
		{
			if (pPixel->IsInRegion()==true && bInRegion==false)
			{
				bInRegion = true;
				pPixel->AddIntoBoundary();
				(pPixel-1)->AddIntoBoundary();
			}
			else if (pPixel->IsInRegion()==false && bInRegion==true)
			{
				bInRegion = false;
				pPixel->AddIntoBoundary();
				(pPixel-1)->AddIntoBoundary();
			}
			pPixel++;
		}
		pPixel+=iOffsetEndRow;
	}

	// Detect vertical edges
	pPixel = pElements + GetOffset(piMin);
	iOffsetEndColumn = (piMin.y-piMax.y-1)*iWidth+1;

	for (p.x=piMin.x; p.x<=piMax.x; p.x++)
	{
		//pPixel = pElements + GetOffset(p.x, piMin.y);
		bInRegion = false;

		if (pPixel->IsInRegion()==true)
        {
            bInRegion = true;
            pPixel->AddIntoBoundary();
            if (piMin.y>0)
                (pPixel-iWidth)->AddIntoBoundary();
        }
        pPixel += iWidth;

		for (p.y=piMin.y+1; p.y<=piMax.y; p.y++)
		{
			if (pPixel->IsInRegion()==true && bInRegion==false)
			{
				bInRegion = true;
				pPixel->AddIntoBoundary();
				(pPixel-iWidth)->AddIntoBoundary();
			}
			else if (pPixel->IsInRegion()==false && bInRegion==true)
			{
				bInRegion = false;
				pPixel->AddIntoBoundary();
				(pPixel-iWidth)->AddIntoBoundary();
			}
			pPixel += iWidth;
		}
		pPixel += iOffsetEndColumn;
	}
}

void CBinaryRegionBase::UpdateNarrowBand()
{
	CBinaryRegionPixel *pPixel;
	CCouple<int> p, piMin, piMax, piNeighborMin, piNeighborMax;
	int v, iPixel, iOffsetEndRow, iSize;

	if (iNbPixels==0)
		return;

	// Remove all pixels from narrow band
	iSize = iWidth*iHeight;
	pPixel = pElements;

	for (iPixel=0; iPixel<iSize; iPixel++)
	{
		pPixel->RemoveFromNarrowBand();
		pPixel++;
	}

	piMin = piRegionMin;
	piMax = piRegionMax;
	iOffsetEndRow = iWidth-1-piMax.x+piMin.x;
	pPixel = pElements + GetOffset(piMin);

	for (p.y=piMin.y; p.y<=piMax.y; p.y++)
	{
		for (p.x=piMin.x; p.x<=piMax.x; p.x++)
		{
			if (pPixel->IsInInnerBoundary())
			{
				if (pPixel->IsInBandSafeArea())
				{
					for (v=0; v<vectNeighborhoodBand.GetSize(); v++)
						(pPixel + vectNeighborhoodOffsetsBand[v])->AddIntoNarrowBand();
				}
				else {
					piNeighborMin.Set(-p.x, -p.y);
					piNeighborMax.Set(iWidth-1-p.x, iHeight-1-p.y);

					for (v=0; v<vectNeighborhoodBand.GetSize(); v++)
					{
						if (vectNeighborhoodBand[v].IsInRange(piNeighborMin, piNeighborMax))
							(pPixel + vectNeighborhoodOffsetsBand[v])->AddIntoNarrowBand();
					}
				}
			}
			pPixel++;
		}
		pPixel += iOffsetEndRow;
	}
}

void CBinaryRegionBase::UpdateSignedDistance()
{
	CBinaryRegionPixel *pPixel, *pPixelNeighbor;
	float *pDistance, *pDistanceNeighbor;
	CCouple<int> p, piMin, piMax, piNeighbor, piNeighborMin, piNeighborMax;
	int v, iOffsetEndRow;

	if (iNbPixels==0)
		return;

	// Set distance values to -infinity inside and +infinity outside
	// in a narrow band both sides apart from the inner boundary
	// The width of this narrow band is iNeighborhoodDistanceHalfWidth
	piMin = coupleMax(CCouple<int>(0,0), piRegionMin -CCouple<int>(iNeighborhoodDistanceHalfWidth, iNeighborhoodDistanceHalfWidth));
	piMax = coupleMin(CCouple<int>(iWidth-1,iHeight-1), piRegionMax+CCouple<int>(iNeighborhoodDistanceHalfWidth, iNeighborhoodDistanceHalfWidth));
	iOffsetEndRow = iWidth-1-piMax.x+piMin.x;

	pPixel = pElements + GetOffset(piMin);
	pDistance = arraySignedDistance.GetBuffer() + arraySignedDistance.GetOffset(piMin);

	for (p.y=piMin.y; p.y<=piMax.y; p.y++)
	{
		for (p.x=piMin.x; p.x<=piMax.x; p.x++)
		{
			if (pPixel->IsInRegion())
				*pDistance = -FLT_MAX;
			else
				*pDistance = FLT_MAX;
			pPixel++;
			pDistance++;
		}
		pPixel += iOffsetEndRow;
		pDistance += iOffsetEndRow;
	}

	piMin = piRegionMin;
	piMax = piRegionMax;
	iOffsetEndRow = iWidth-1-piMax.x+piMin.x;

	pPixel = pElements + GetOffset(piMin);
	pDistance = arraySignedDistance.GetBuffer() + arraySignedDistance.GetOffset(piMin);

	for (p.y=piMin.y; p.y<=piMax.y; p.y++)
	{
		for (p.x=piMin.x; p.x<=piMax.x; p.x++)
		{
			if (pPixel->IsInInnerBoundary())
			{
				if (pPixel->IsInDistanceSafeArea())
				{
					for (v=0; v<vectNeighborhoodDistance.GetSize(); v++)
					{
						pPixelNeighbor = pPixel + vectNeighborhoodOffsetsDistance[v];
						pDistanceNeighbor = pDistance + vectNeighborhoodOffsetsDistance[v];
						if (pPixelNeighbor->IsInRegion())
							*pDistanceNeighbor = max(*pDistanceNeighbor, -vectNeighborhoodWeightsDistance[v]);
						else
							*pDistanceNeighbor = min(*pDistanceNeighbor, vectNeighborhoodWeightsDistance[v]);
					}
				}
				else {
					piNeighborMin.Set(-p.x, -p.y);
					piNeighborMax.Set(iWidth-1-p.x, iHeight-1-p.y);

					for (v=0; v<vectNeighborhoodDistance.GetSize(); v++)
					{
						if (vectNeighborhoodDistance[v].IsInRange(piNeighborMin, piNeighborMax))
						{
							pPixelNeighbor = pPixel + vectNeighborhoodOffsetsDistance[v];
							pDistanceNeighbor = pDistance + vectNeighborhoodOffsetsDistance[v];
							if (pPixelNeighbor->IsInRegion())
								*pDistanceNeighbor = max(*pDistanceNeighbor, -vectNeighborhoodWeightsDistance[v]);
							else
								*pDistanceNeighbor = min(*pDistanceNeighbor, vectNeighborhoodWeightsDistance[v]);
						}
					}
				}
			}
			pPixel++;
			pDistance++;
		}
		pPixel += iOffsetEndRow;
		pDistance += iOffsetEndRow;
	}
}

void CBinaryRegionBase::UpdateAfterEvolution()
{
	UpdateBoundary();

	// Average intensities/colors over narrow bands need to be updated
	// only if weight of region speed is non-zero
	if (iInsideMode==BAND || iOutsideMode==BAND)
	{
		UpdateNarrowBand();
		if (fWeightRegion!=0.0f)
			ComputeBandMeans();
	}

	// Update signed euclidean distance if local outer data term is used
	if (fWeightRegion!=0.0f && iOutsideMode==LINE)
		UpdateSignedDistance();
}

void CBinaryRegionBase::UpdateAfterInitialization()
{
	UpdateBoundary();

	// Average intensities/colors over narrow bands need to be updated
	// only if weight of region speed is non-zero
	if (iInsideMode==BAND || iOutsideMode==BAND)
	{
		UpdateNarrowBand();
		if (fWeightRegion!=0.0f)
			ComputeBandMeans();
	}

	// Update signed euclidean distance if local outer data term is used
	if (fWeightRegion!=0.0f && iOutsideMode==LINE)
		UpdateSignedDistance();
}

/**********************************************************************
*                      CBinaryRegionGrayscale                         *
**********************************************************************/

CBinaryRegionGrayscale::CBinaryRegionGrayscale():CBinaryRegionBase()
{
	fInnerSum = 0.0f;
	fOuterSum = 0.0f;

	fInnerMean = 0.0f;
	fOuterMean = 0.0f;

	fInnerNarrowBandMean = 0.0f;
	fOuterNarrowBandMean = 0.0f;
}

void CBinaryRegionGrayscale::AttachImage(const cv::Mat *pImage)
{
	if (pImage->type()!=CV_8UC1)
	{
		cerr<<"ERROR in CBinaryRegionGrayscale::AttachImage(): instances of CBinaryRegionGrayscale can be attached to 8-bit images only"<<endl;
		pInputImage = NULL;
		return;
	}

	pInputImage = pImage;
	ConvertCvMatToArray2DFloat(arrayImage, *pImage);

	// If edge information is used, then precompute gradient magnitude
	if (fWeightGradient!=0.0f)
	{
		CArray2D<float> arrayGaussian, arrayDiffX, arrayDiffY;
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
	}

	// Init binary pixel array
	Init(pInputImage->cols, pInputImage->rows);
	Empty();

	InitNeighborhoodOffsets();
	InitNeighborhoodsSmoothingBand();
	UpdateSafeAreas();

	// Init distance map if the local outer data term is used
	if (iOutsideMode==LINE)
		arraySignedDistance.Init(pInputImage->cols, pInputImage->rows);
}

void CBinaryRegionGrayscale::Empty()
{
	int x, y, iWidth, iHeight;

	CBinaryRegionBase::Empty();

	fInnerSum = 0.0f;
	fOuterSum = 0.0f;
	fInnerMean = 0.0f;
	fInnerNarrowBandMean = 0.0f;
	fOuterNarrowBandMean = 0.0f;

	// Compute sum of image intensities
	iWidth = arrayImage.GetWidth();
	iHeight = arrayImage.GetHeight();

	for (y=0;y<iHeight;y++)
		for (x=0;x<iWidth;x++)
			fOuterSum += arrayImage.Element(x,y);

	fOuterMean = fOuterSum/(float)(iWidth*iHeight);
}

void CBinaryRegionGrayscale::AddPixel(const CCouple<int> &p)
{
	CBinaryRegionPixel *pPixel;
	float fIntensity;

	pPixel = pElements + GetOffset(p);

	// If pixel is already inside the region, nothing to do
	if (pPixel->IsInRegion()==true)
		return;

	if (iNbPixels==0)
	{
		piRegionMin = p;
		piRegionMax = p;
	}
	else {
		piRegionMin = coupleMin(piRegionMin, p);
		piRegionMax = coupleMax(piRegionMax, p);
	}

	pPixel->AddIntoRegion();
	fIntensity = arrayImage.Element(p);
	fInnerSum += fIntensity;
	fOuterSum -= fIntensity;
	iNbPixels++;
}

void CBinaryRegionGrayscale::RemovePixel(const CCouple<int> &p)
{
	CBinaryRegionPixel *pPixel;
	float fIntensity;

	pPixel = pElements + GetOffset(p);

	// If pixel is already outside the region, nothing to do
	if (pPixel->IsInRegion()==false)
		return;

	pPixel->RemoveFromRegion();
	fIntensity = arrayImage.Element(p);
	fInnerSum -= fIntensity;
	fOuterSum += fIntensity;
	iNbPixels--;
}

void CBinaryRegionGrayscale::ComputeBandMeans()
{
	CBinaryRegionPixel *pPixel;
	CCouple<int> p, piMin, piMax;
	float fInnerNarrowBandSum, fOuterNarrowBandSum;
	int iNbPixelsInnerNarrowBand, iNbPixelsOuterNarrowBand;
	int iOffsetEndRow;

	if (iNbPixels==0)
		return;

	iNbPixelsInnerNarrowBand = 0;
	iNbPixelsOuterNarrowBand = 0;

	fInnerNarrowBandSum = 0.0f;
	fOuterNarrowBandSum = 0.0f;

	piMin = coupleMax(piRegionMin-CCouple<int>(iBandThickness, iBandThickness), CCouple<int>(0,0));
	piMax = coupleMin(piRegionMax+CCouple<int>(iBandThickness, iBandThickness), CCouple<int>(iWidth-1, iHeight-1));

	pPixel = pElements + GetOffset(piMin);
	iOffsetEndRow = iWidth-1-piMax.x+piMin.x;
	for (p.y=piMin.y;p.y<=piMax.y;p.y++)
	{
		for (p.x=piMin.x;p.x<=piMax.x;p.x++)
		{
			if (pPixel->IsInInnerNarrowBand() && iInsideMode==BAND)
			{
				iNbPixelsInnerNarrowBand++;
				fInnerNarrowBandSum += arrayImage.Element(p);
			}
			else if (pPixel->IsInOuterNarrowBand() && iOutsideMode==BAND)
			{
				iNbPixelsOuterNarrowBand++;
				fOuterNarrowBandSum += arrayImage.Element(p);
			}
			pPixel++;
		}
		pPixel+=iOffsetEndRow;
	}

	fInnerNarrowBandMean = fInnerNarrowBandSum/(float)iNbPixelsInnerNarrowBand;
	fOuterNarrowBandMean = fOuterNarrowBandSum/(float)iNbPixelsOuterNarrowBand;
}

void CBinaryRegionGrayscale::UpdateAfterEvolution()
{
	CBinaryRegionBase::UpdateAfterEvolution();

	if (fWeightRegion!=0.0f && (iInsideMode==REGION || iOutsideMode==REGION))
	{
		fInnerMean = fInnerSum/(float)iNbPixels;
		fOuterMean = fOuterSum/(float)(iWidth*iHeight-iNbPixels);
	}
}

void CBinaryRegionGrayscale::UpdateAfterInitialization()
{
	CBinaryRegionBase::UpdateAfterInitialization();

	if (fWeightRegion!=0.0f && (iInsideMode==REGION || iInsideMode==REGION_INIT
		|| iOutsideMode==REGION))
	{
		fInnerMean = fInnerSum/(float)iNbPixels;
		fOuterMean = fOuterSum/(float)(iWidth*iHeight-iNbPixels);

		if (iInsideMode==REGION_INIT)
			fInnerInitialMean = fInnerMean;
	}
}

float CBinaryRegionGrayscale::RegionSpeed(const CCouple<int> &p) const
{
	// Region terms
	float fDiffRegion, fDiffRegionInside, fDiffRegionOutside;

	// Intensity at current point
	float fIntensity;

	// Inner and outer intensity region descriptors
	float fInnerMeanCurrent=0.0f, fOuterMeanCurrent=0.0f;

	// For computing the bias force
	float fCoefBias, fForceBias, fDiffInsideOutside;

	fIntensity = arrayImage.Element(p);

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
		// Convert band thickness to real value once (loop will be slightly faster)
		float fBandThickness = (float)iBandThickness;

		float fThickness;
		float fWeightCurvature, fCurvature, fSumWeights;
		CCouple<float> vfNormal, pfPos;

		GetNormalAndCurvature(p, vfNormal, fCurvature);
		pfPos = (CCouple<float>)p;

		// Average weighted intensity along the outward normal line segment of length equal to band thickness
		// Implements a discretization of h_out(x) appearing in section 6 in [Mille09]
		fOuterMeanCurrent = 0.0f;
		fSumWeights = 0.0f;

		// Scan the normal line segment
		// For every point located on this line segment, a test related to regularity condition (12) in [Mille09]
		// is performed. If it fails, the thickness exceeds the radius of curvature, yielding a singularity on the
		// outer parallel curve. In this case, it seems reasonable not to include the resulting pixel
		if (Element(p).IsInBandSafeArea())
		{
			// Points on the normal line segment can be scanned without checking if they are within the image domain
			for (fThickness=1.0f; fThickness<=fBandThickness; fThickness+=1.0f)
			{
				fWeightCurvature = 1.0f + fThickness*fCurvature; // Implements (1+b\kappa_i)
				if (fWeightCurvature>0.0f)
				{
					fOuterMeanCurrent += arrayImage.GetElementInterpolate(pfPos - fThickness*vfNormal)*fWeightCurvature;
					fSumWeights += fWeightCurvature;
				}
			}
		}
		else {
			CCouple<float> pfPosNormal, pfMin, pfMax;

			pfMin.Set(0.0f, 0.0f);
			pfMax.Set((float)(iWidth-2), (float)(iHeight-2));

			// Points on the normal line segment need to be checked
			for (fThickness=1.0f; fThickness<=fBandThickness; fThickness+=1.0f)
			{
				fWeightCurvature = 1.0f + fThickness*fCurvature; // Implements (1+b\kappa_i)
				if (fWeightCurvature>0.0f)
				{
					pfPosNormal = pfPos - fThickness*vfNormal;
					pfPosNormal.LimitWithinRange(pfMin, pfMax);

					fOuterMeanCurrent += arrayImage.GetElementInterpolate(pfPosNormal)*fWeightCurvature;
					fSumWeights += fWeightCurvature;
				}
			}
		}

		if (fSumWeights==0.0f)
		{
			// This happens if the region boundary is excessively concave at the current vertex
			// Return a null speed, so that only regularization is effective on this particular point
			return 0.0f;
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
	// minimization of intensity deviation inside or outside. This enables to implement the data term of the
	// Chan-Vese model (which is asymmetric) for comparison purpose.
	// The initial configuration is symmetric, since the default value of fWeightRegionInOverOut is 0.5
	fDiffRegion = fWeightRegionInOverOut * fDiffRegionInside - (1.0f-fWeightRegionInOverOut) * fDiffRegionOutside;

	// If bias is enabled, a balloon-like speed is added to the region speed
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

	return fDiffRegion;
}



/**********************************************************************
*                        CBinaryRegionColor                           *
**********************************************************************/

CBinaryRegionColor::CBinaryRegionColor():CBinaryRegionBase()
{
	fcolInnerSum.Set(0.0f, 0.0f, 0.0f);
	fcolOuterSum.Set(0.0f, 0.0f, 0.0f);

	fcolInnerMean.Set(0.0f, 0.0f, 0.0f);
	fcolOuterMean.Set(0.0f, 0.0f, 0.0f);

	fcolInnerNarrowBandMean.Set(0.0f, 0.0f, 0.0f);
	fcolOuterNarrowBandMean.Set(0.0f, 0.0f, 0.0f);

	// Default color space is RGB
	iColorSpace = RGB;
	bIgnoreBrightnessComponent = false;
}

void CBinaryRegionColor::AttachImage(const cv::Mat *pImage)
{
	if (pImage->type()!=CV_8UC3)
	{
		cerr<<"ERROR in CBinaryRegionColor::AttachImage(): instances of CBinaryRegionColor can be attached to RGB images only"<<endl;
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

	// If edge information is used, then precompute gradient magnitude
	if (fWeightGradient!=0.0f)
	{
		CArray2D<CTriplet<float> > arrayDiffX, arrayDiffY;
		CArray2D<float> arrayGaussian;
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
	}

	// Init binary pixel array
	Init(pInputImage->cols, pInputImage->rows);
	Empty();

	InitNeighborhoodOffsets();
	InitNeighborhoodsSmoothingBand();
	UpdateSafeAreas();

	// Init distance map if the local outer data term is used
	if (fWeightRegion!=0.0f && iOutsideMode==LINE)
		arraySignedDistance.Init(pInputImage->cols, pInputImage->rows);
}

void CBinaryRegionColor::Empty()
{
	int x, y, iWidth, iHeight;

	CBinaryRegionBase::Empty();

	fcolInnerSum.Set(0.0f, 0.0f, 0.0f);
	fcolOuterSum.Set(0.0f, 0.0f, 0.0f);
	fcolInnerMean.Set(0.0f, 0.0f, 0.0f);
	fcolInnerNarrowBandMean.Set(0.0f, 0.0f, 0.0f);
	fcolOuterNarrowBandMean.Set(0.0f, 0.0f, 0.0f);

	// Compute sum of image intensities
	iWidth = arrayImage.GetWidth();
	iHeight = arrayImage.GetHeight();

	for (y=0;y<iHeight;y++)
		for (x=0;x<iWidth;x++)
			fcolOuterSum += arrayImage.Element(x,y);

	fcolOuterMean = fcolOuterSum/(float)(iWidth*iHeight);
}

void CBinaryRegionColor::AddPixel(const CCouple<int> &p)
{
	CBinaryRegionPixel *pPixel;
	CTriplet<float> fPixel;

	pPixel = pElements + GetOffset(p);

	// If pixel is already in inner region, nothing to do
	if (pPixel->IsInRegion()==true)
		return;

	if (iNbPixels==0)
	{
		piRegionMin = p;
		piRegionMax = p;
	}
	else {
		piRegionMin = coupleMin(piRegionMin, p);
		piRegionMax = coupleMax(piRegionMax, p);
	}

	pPixel->AddIntoRegion();
	fPixel = arrayImage.Element(p);
	fcolInnerSum += fPixel;
	fcolOuterSum -= fPixel;
	iNbPixels++;
}

void CBinaryRegionColor::RemovePixel(const CCouple<int> &p)
{
	CBinaryRegionPixel *pPixel;
	CTriplet<float> fPixel;

	pPixel = pElements + GetOffset(p);

	// If pixel is already outside the region, nothing to do
	if (pPixel->IsInRegion()==false)
		return;

	pPixel->RemoveFromRegion();
	fPixel = arrayImage.Element(p);
	fcolInnerSum -= fPixel;
	fcolOuterSum += fPixel;
	iNbPixels--;
}

void CBinaryRegionColor::ComputeBandMeans()
{
	CBinaryRegionPixel *pPixel;
	CCouple<int> p, piMin, piMax;
	CTriplet<float> fcolInnerNarrowBandSum, fcolOuterNarrowBandSum;
	int iNbPixelsInnerNarrowBand, iNbPixelsOuterNarrowBand;
	int iOffsetEndRow;

	if (iNbPixels==0)
		return;

	iNbPixelsInnerNarrowBand = 0;
	iNbPixelsOuterNarrowBand = 0;

	fcolInnerNarrowBandSum.Set(0.0f, 0.0f, 0.0f);
	fcolOuterNarrowBandSum.Set(0.0f, 0.0f, 0.0f);

	piMin = coupleMax(piRegionMin-CCouple<int>(iBandThickness, iBandThickness), CCouple<int>(0,0));
	piMax = coupleMin(piRegionMax+CCouple<int>(iBandThickness, iBandThickness), CCouple<int>(iWidth-1, iHeight-1));

	pPixel = pElements + GetOffset(piMin);
	iOffsetEndRow = iWidth-1-piMax.x+piMin.x;
	for (p.y=piMin.y;p.y<=piMax.y;p.y++)
	{
		for (p.x=piMin.x;p.x<=piMax.x;p.x++)
		{
			if (pPixel->IsInInnerNarrowBand() && iInsideMode==BAND)
			{
				iNbPixelsInnerNarrowBand++;
				fcolInnerNarrowBandSum += arrayImage.Element(p);
			}
			else if (pPixel->IsInOuterNarrowBand() && iOutsideMode==BAND)
			{
				iNbPixelsOuterNarrowBand++;
				fcolOuterNarrowBandSum += arrayImage.Element(p);
			}
			pPixel++;
		}
		pPixel += iOffsetEndRow;
	}

	fcolInnerNarrowBandMean = fcolInnerNarrowBandSum/(float)iNbPixelsInnerNarrowBand;
	fcolOuterNarrowBandMean = fcolOuterNarrowBandSum/(float)iNbPixelsOuterNarrowBand;
}


void CBinaryRegionColor::UpdateAfterEvolution()
{
	CBinaryRegionBase::UpdateAfterEvolution();

	if (fWeightRegion!=0.0f && (iInsideMode==REGION || iOutsideMode==REGION))
	{
		fcolInnerMean = fcolInnerSum/(float)iNbPixels;
		fcolOuterMean = fcolOuterSum/(float)(iWidth*iHeight-iNbPixels);
	}
}

void CBinaryRegionColor::UpdateAfterInitialization()
{
	CBinaryRegionBase::UpdateAfterInitialization();

	if (fWeightRegion!=0.0f && (iInsideMode==REGION || iInsideMode==REGION_INIT
		|| iOutsideMode==REGION))
	{
		fcolInnerMean = fcolInnerSum/(float)iNbPixels;
		fcolOuterMean = fcolOuterSum/(float)(iWidth*iHeight-iNbPixels);

		if (iInsideMode==REGION_INIT)
			fcolInnerInitialMean = fcolInnerMean;
	}
}

float CBinaryRegionColor::RegionSpeed(const CCouple<int> &p) const
{
	// Region terms
	float fDiffRegion, fDiffRegionInside, fDiffRegionOutside;

	// Color at current point
	CTriplet<float> fPixel;

	// Inner and outer intensity region descriptors
	CTriplet<float> fcolInnerMeanCurrent(0.0f, 0.0f, 0.0f), fcolOuterMeanCurrent(0.0f, 0.0f, 0.0f);

	// For computing the bias force
	float fCoefBias, fForceBias, fDiffInsideOutside;

	fPixel = arrayImage.Element(p);

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
		// Convert band thickness to real value once (loop will be slightly faster)
		float fBandThickness = (float)iBandThickness;

		float fThickness;
		float fWeightCurvature, fCurvature, fSumWeights;
		CCouple<float> vfNormal, pfPos;

		GetNormalAndCurvature(p, vfNormal, fCurvature);
		pfPos = (CCouple<float>)p;

		// Average weighted color along the outward normal line segment of length equal to band thickness
		// Implements a discretization of h_out(x) appearing in section 6 in [Mille09]
		fcolOuterMeanCurrent = 0.0f;
		fSumWeights = 0.0f;

		// Scan the normal line segment
		// For every point located on this line segment, a test related to regularity condition (12) in [Mille09]
		// is performed. If it fails, the thickness exceeds the radius of curvature, yielding a singularity on the
		// outer parallel curve. In this case, it seems reasonable not to include the resulting pixel
		if (Element(p).IsInBandSafeArea())
		{
			// Points on the normal line segment can be scanned without checking if they are within the image domain
			for (fThickness=1.0f; fThickness<=fBandThickness; fThickness+=1.0f)
			{
				fWeightCurvature = 1.0f + fThickness*fCurvature; // Implements (1+b\kappa_i)
				if (fWeightCurvature>0.0f)
				{
					fcolOuterMeanCurrent += arrayImage.GetElementInterpolate(pfPos - fThickness*vfNormal)*fWeightCurvature;
					fSumWeights += fWeightCurvature;
				}
			}
		}
		else {
			CCouple<float> pfPosNormal, pfMin, pfMax;

			pfMin.Set(0.0f, 0.0f);
			pfMax.Set((float)(iWidth-2), (float)(iHeight-2));

			// Points on the normal line segment need to be checked
			for (fThickness=1.0f; fThickness<=fBandThickness; fThickness+=1.0f)
			{
				fWeightCurvature = 1.0f + fThickness*fCurvature; // Implements (1+b\kappa_i)
				if (fWeightCurvature>0.0f)
				{
					pfPosNormal = pfPos - fThickness*vfNormal;
					pfPosNormal.LimitWithinRange(pfMin, pfMax);

					fcolOuterMeanCurrent += arrayImage.GetElementInterpolate(pfPosNormal)*fWeightCurvature;
					fSumWeights += fWeightCurvature;
				}
			}
		}

		if (fSumWeights==0.0f)
		{
			// This happens if the region boundary is excessively concave at the current vertex
			// Return a null speed, so that only regularization is effective on this particular point
			return 0.0f;
		}
		else
			fcolOuterMeanCurrent /= fSumWeights;
	}

	// Unlike in [Mille09], deviations with respect to inner and outer descriptors
	// are computed using the L2 norm
	// This yields better behaviour with respect to local color differences
	fDiffRegionInside = (fcolInnerMeanCurrent-fPixel).L2Norm();
	fDiffRegionOutside = (fcolOuterMeanCurrent-fPixel).L2Norm();

	// The narrow band region energies introduced in [Mille09] are symmetric, i.e. inner and outer region terms
	// are identically weighted However, an asymmetric configuration is allowed here, in order to favor
	// minimization of color deviation inside or outside. This enables to implement the data term of the
	// Chan-Vese model (which is asymmetric) for comparison purpose.
	// The initial configuration is symmetric, since the default value of fWeightRegionInOverOut is 0.5
	fDiffRegion = fWeightRegionInOverOut * fDiffRegionInside - (1.0f-fWeightRegionInOverOut) * fDiffRegionOutside;

	// If bias is enabled, a balloon-like speed is added to the region speed
	if (bRegionEnergyBias==true)
	{
		// This is slightly different from the bias force described in section 5.5 in [Mille09],
		// as the magnitude of the bias is set by the balloon weight (which should be negative)
		fForceBias = fWeightBalloon + (fcolInnerMeanCurrent-fPixel).L2Norm();

		// Compute bias coefficient, denoted \gamma and introduced in equation (56) in [Mille09]
		// It decreases exponentially with respect to the difference between inner and outer region descriptors
		fDiffInsideOutside = (fcolInnerMeanCurrent-fcolOuterMeanCurrent).L2Norm();
		fCoefBias = (1.0f-fDiffInsideOutside)/(1.0f+50.0f*fDiffInsideOutside);

		// Linear combination of bias and region speed
		// See second part of equation (56)
		fDiffRegion = fCoefBias*fForceBias + (1.0f-fCoefBias)*fDiffRegion;
	}

	return fDiffRegion;
}
