#include <iostream>
#include "support_class.h"
#include "cv.h"

using namespace std;

//------------- define the feature type and id -------------
enum { DEPT, EDGE_MAG, EDGE_ORI };	/* feature_id */
enum { DIFF, SUM, BOTH };               /* feature_type */

//-----------------------------------------------------
/* feature class */ 

Feature::Feature() {}


Feature::Feature( int id, CvPoint point, float** matrix, int M, int N, int flag )
{
	feat_id = id; 
	center = point;
	data = matrix; 
	row = M; 
	col = N; 
	label = flag; 
	value = 0;
}


Feature::~Feature()  {}


void Feature::set_offset ( CvPoint u, CvPoint v )
{
	offset_1 = u;
	offset_2 = v;
}


void Feature::set_type ( int type )
{
	feat_type = type; 
} 


void Feature::computeFeature()
{
	if ( data==0 )
		cout << " can not find the matrix of the image " << endl;

	float depth = data[center.x][center.y];
	int nora_x, nora_y; 
	if ( feat_id!=DEPT ) {
		nora_x = offset_1.x ; // /(depth+0.01)
		nora_y = offset_1.y;	
	} else {
		if ( depth !=0 )
		{	nora_x = offset_1.x / depth;
			nora_y = offset_1.y / depth;
		} else {
			nora_x = offset_1.x;
			nora_y = offset_1.y;
		}
	}
	int left_x = center.x + nora_x;
	int left_y = center.y + nora_y;
	if ( left_x<0 )
		left_x = 0;
	if ( left_x>row-1 )
		left_x = row - 1;
	if ( left_y<0 )
		left_y = 0;
	if ( left_y>col-1 )
		left_y = col - 1;

	float left_depth = data[ left_x ][ left_y ];
	//cout << depth << " " <<  offset_1.x << " " << offset_1.y << " " << left_x << " " << left_y << " " << left_depth << " "; 

	if ( feat_id!=DEPT ) {
		nora_x = offset_2.x;  // /(depth+0.01)
		nora_y = offset_2.y;	
	} else {
		if ( depth !=0 )
		{	nora_x = offset_2.x / depth;
			nora_y = offset_2.y / depth;
		} else {
			nora_x = offset_2.x;
			nora_y = offset_2.y;
		}
	}
	left_x = center.x + nora_x;
	left_y = center.y + nora_y;
	if ( left_x<0 )
		left_x = 0;
	if ( left_x>row-1 )
		left_x = row - 1;
	if ( left_y<0 )
		left_y = 0;
	if ( left_y>col-1 )
		left_y = col - 1;

	float right_depth = data[ left_x ][ left_y ];
	//cout << offset_2.x << " " << offset_2.y << " " << left_x << " " << left_y << " " << right_depth << " ";

	switch( feat_type )
	{	case DIFF: 
			this->value = left_depth - right_depth; break;
		case SUM:
			this->value = left_depth + right_depth; break; 
	}
		
}


float Feature::get_center_depth()  { return data[center.x][center.y]; }


CvPoint Feature::get_center_pos () { return center; }


CvPoint Feature::get_offset_1()  { return offset_1; }


CvPoint Feature::get_offset_2()  { return offset_2; }


//---------------------------------------------------------------------------------
/* FeatureVector functions */

FeatureVector::FeatureVector ( int flag ): label(flag) { }


FeatureVector::FeatureVector( Feature* feat, int flag )
{
	features.push_back( feat ); 
	label = flag; 
}


FeatureVector::~FeatureVector()
{
	for ( list<Feature*>::iterator it=features.begin(); it!=features.end(); it++ )
	{	if ( *it )
			delete *it; 
	}
}


float FeatureVector::get_feature ( int id, int type )
{
	int i = 0; 
	float value = 0; 
	for ( list<Feature*>::iterator it=features.begin(); it!=features.end(); it++, i++ )
	{	
		if ( i==id )
		{	if ( type != (*it)->feat_type )
			{	cout << "feature type is not consistent, please check the set of feature type " << endl; 
				exit(0);
			} else 
			{  value = (*it)->value; break; 	 }
		} else 
			continue;
	}

/*
	// for circular condition for the orienataion, have no effect on the undirect orientation [0 PI]
	if ( id==2 && type==0 )
	{
		if ( value>=0 )
			value = ( (value<=(36-value)) ? value : (36-value) ); 
		else 
			value = ( (value>=(-36-value)) ? value : (-36-value) );
		}
	}
*/
	return value; 
}


void FeatureVector::reset_feature( int id, int type, CvPoint u, CvPoint v )
{
	int i = 0; 
	for( list<Feature*>::iterator it=features.begin(); it!=features.end(); it++, i++ )
	{
		if ( i==id )
		{	(*it)->set_offset( u, v );
			(*it)->set_type( type ); 
			(*it)->computeFeature();
			break;	
		} else
			continue; 
	}
}


CvPoint FeatureVector::get_center( )
{	
	CvPoint a; 
	for ( list<Feature*>::iterator it=features.begin(); it!=features.end(); it++ )
	{	if( (*it) ){
			a = (*it)->center; 
			break; 
		} else 
			continue; 
	}
	return a; 
}


CvPoint FeatureVector::get_offset_1( int id )
{
	int i = 0; 
	CvPoint value; 
	for( list<Feature*>::iterator it=features.begin(); it!=features.end(); it++, i++ )
	{
		if ( i==id )
		{	value = (*it)->get_offset_1(); break;	  }
		else
			continue; 
	}
	return value; 
}


CvPoint FeatureVector::get_offset_2( int id )
{
	int i = 0; 
	CvPoint value; 
	for( list<Feature*>::iterator it=features.begin(); it!=features.end(); it++, i++ )
	{
		if ( i==id )
		{	value = (*it)->get_offset_2(); break;	  }
		else
			continue; 
	}
	return value; 
}


//--------------------------------------------------------------
/* ClassLabelHistogram functions */

ClassLabelHistogram::ClassLabelHistogram()
{
  memberCount = 0;
}


ClassLabelHistogram::ClassLabelHistogram( int totalNum )
{
	memberCount = 0;
	for ( int i=0; i<totalNum; i++ ) {
		countHistogram[i] = 0;	
		scaleHistogram[i] = 0;
	}
}


ClassLabelHistogram::~ClassLabelHistogram() {
	countHistogram.clear(); 
	scaleHistogram.clear(); 
	//cout << " explicitly call this function " << endl; 
} 


//**************************************************************************
//                         public Methods
//**************************************************************************
void ClassLabelHistogram::reset ()
{
  countHistogram.clear();
  scaleHistogram.clear();
  memberCount = 0;
}


unsigned int ClassLabelHistogram::getTotalCount()
{
  return memberCount;
}


int ClassLabelHistogram::getBinCount()
{
	return (int)countHistogram.size();
}


void ClassLabelHistogram::getListOfClassLabels (list<int>* classLabelList)
{
  classLabelList->clear();
  for (map<int, int>::iterator p=countHistogram.begin(); (p!=countHistogram.end()); ++p)  {
    int l = (*p).first;
    classLabelList->push_back(l);
  }
}


int ClassLabelHistogram::getCountOfLabel (int label)
{
  map<int, int>::iterator found = countHistogram.find(label);
  if(found!=countHistogram.end())
  {
  	return found->second;
  }else{
        return 0;}
}


std::map<int, int> ClassLabelHistogram::getHistogram()
{
	return countHistogram;
}


std::map<int, float> ClassLabelHistogram::getDensity() 
{
	return scaleHistogram;
}


void ClassLabelHistogram::increaseCountOfLabel (int label)
{
	memberCount++;
	if (countHistogram.find(label) != countHistogram.end()) {
		countHistogram[label] = countHistogram[label] + 1;
	} else {
		countHistogram[label] = 1;
	}
	//cout << "problem 1" << endl;
	for ( map<int, int>::iterator p=countHistogram.begin(); (p!=countHistogram.end()); ++p )
			scaleHistogram[p->first] = (float)p->second / (float)memberCount;
	//cout << "problem 2" << endl;
}


void ClassLabelHistogram::setScaleValue( int label, float scale ) {
	scaleHistogram[label] = scale;
}


void ClassLabelHistogram::setLabelCount( int label, int num) {
	memberCount = memberCount-countHistogram[label]+num;
	countHistogram[label] = num;
}


float ClassLabelHistogram::getScaleOfLabel ( int label )
{
  return scaleHistogram[label];
}

