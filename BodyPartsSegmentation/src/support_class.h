#ifndef SUPPORTCLASS_H
#define SUPPORTCLASS_H

#include <map>
#include <list>
#include "cv.h"

class Feature {
	public:
		Feature();
		Feature( int id, CvPoint point, float** matrix, int M, int N, int flag );
		~Feature();
		
		int feat_id;            
		CvPoint center;         // the position of center pixel
		CvPoint offset_1;	// the offset 1
		CvPoint offset_2;	// the offset 2
		int label;
		float value;
		int feat_type; 

		void set_offset ( CvPoint u, CvPoint v );
		void set_type ( int type ); 
		void computeFeature( );

		float get_center_depth ( );
		CvPoint get_center_pos ();
		CvPoint get_offset_1();
		CvPoint get_offset_2();
		int get_label();

	private:
		float** data;
		int row;	// the row of data 
		int col;	// the col of data
};


class FeatureVector {
	public: 
		FeatureVector( int flag=0 );
		FeatureVector( Feature* feat, int flag ); 
		~FeatureVector();
		
		std::list<Feature*>	features;
		int label; 
		
		void add_feature( Feature* feat ) { features.push_back(feat); }
		float get_feature( int id, int type );
		void reset_feature( int id, int type, CvPoint u, CvPoint v );
		CvPoint get_center ( ); 
		CvPoint get_offset_1( int id );
		CvPoint get_offset_2( int id ); 
	private: 
		; 
}; 


class ClassLabelHistogram {

public:
	ClassLabelHistogram();
	ClassLabelHistogram( int totalNum );

	~ClassLabelHistogram();
	void reset ();
	unsigned int getTotalCount();
	int getBinCount();
	void getListOfClassLabels (std::list<int>* classLabelList);
	int getCountOfLabel (int label);
	std::map<int, int> getHistogram();
	std::map<int, float> getDensity();
	void increaseCountOfLabel (int label);	
	void setScaleValue( int label, float scale );
	void setLabelCount( int label, int num);
	float getScaleOfLabel( int label );
private:
	unsigned int memberCount;
	std::map<int, int> countHistogram;
	std::map<int, float> scaleHistogram;
};

#endif
