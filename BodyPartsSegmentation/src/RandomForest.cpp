#include "RandomForest.h"
#include "support_class.h"
#include <iostream>
#include <stdio.h>
#include <locale.h>

extern int PART_SIZE;
extern int FIXED_INF;

int TerminalNode::totalTerminalNodeCount = 0;
int TerminalNode::idCounter = 0;

using namespace std;

//------------------------------------------------------------
/** Functions in the class of Terminal **/

TerminalNode::TerminalNode(ClassLabelHistogram* nodeClassLabel, unsigned int nbOfTree, unsigned int treeLevel, int child, list<FeatureVector*> *featureVecList) {
	if (nodeClassLabel!=0)
		classDistribution = *nodeClassLabel;
		
	parent = 0;
	tree = nbOfTree;
	score = 1.0;
	level = treeLevel;
	child_id = child; 

	idCounter++;
	id = idCounter;
	totalTerminalNodeCount++;
}


TerminalNode::~TerminalNode ( ) {
	totalTerminalNodeCount--; 
}


TerminalNode* TerminalNode::classify ( FeatureVector* featureVec ) {
	return this;
}


std::map<int, float>  TerminalNode::getClassDistribution(){
	return classDistribution.getDensity();
}


void TerminalNode::writeToFile( std::ostream &out )
{
	float* tmp = new float [PART_SIZE];
	for ( int i=0; i<PART_SIZE; i++ )
		tmp[i] = 0; 
	
	map<int, float> dis = getClassDistribution();
	for ( map<int, float>::iterator p=dis.begin(); p!=dis.end(); p++ )
		tmp[p->first] += p->second;

	out << "TerminalNode;" << tree << ";" << level << ";" << child_id << ";";  
	for ( int i=0; i<PART_SIZE; i++ )
		out << tmp[i] << ";";
	out << endl; 
		
	delete[] tmp; 
}


//------------------------------------------------
/* functions in the class of Decision node */

unsigned int DecisionNode::totalDecisionNodeCount = 0;


DecisionNode::DecisionNode( int id, int type, CvPoint u, CvPoint v, float splitThreshold, TreeNode* nodeAttributeSmaller, TreeNode* nodeAttributeGE, double scoreVal, unsigned int nbOfTree, unsigned int treeLevel, int child ) 
{
	feat_id = id; 
	feat_type = type; 
	parent = 0;
	offset_1 = u;
	offset_2 = v;
	threshold = splitThreshold;
	score = scoreVal;
	tree = nbOfTree;
	level = treeLevel;
	child_id = child; 
	nodeAttSmaller = nodeAttributeSmaller;
	nodeAttGE = nodeAttributeGE;
	totalDecisionNodeCount++;
}


DecisionNode::~DecisionNode ( ) 
{
	if (nodeAttSmaller != 0) 
		delete nodeAttSmaller;
	if (nodeAttGE != 0)
		delete nodeAttGE;
	totalDecisionNodeCount--;
}


TerminalNode* DecisionNode::classify ( FeatureVector* featureVec )
{
	featureVec->reset_feature( feat_id, feat_type, offset_1, offset_2 );
	float value = featureVec->get_feature( feat_id, feat_type );
	if ( value  < threshold )
		return nodeAttSmaller->classify( featureVec );
	else	
		return nodeAttGE->classify( featureVec );
}


void DecisionNode::writeToFile(std::ostream &out)
{
	out << "DecisionNode;" << tree << ";" << level << ";" << child_id << ";" << feat_id << ";" << feat_type << ";" << offset_1.x << ";" << offset_1.y << ";" << offset_2.x << ";" << offset_2.y << ";" << threshold << ";" << score << ";" << endl; 

	DecisionNode* dnode = dynamic_cast <DecisionNode *> (nodeAttSmaller); 
	TerminalNode* tnode = NULL; 
	if( dnode ) {
		dnode->writeToFile( out ); 		
	} else{ 
		tnode = dynamic_cast <TerminalNode *> (nodeAttSmaller);
		if ( tnode )
			tnode->writeToFile( out ); 
	}

	dnode = dynamic_cast <DecisionNode *> (nodeAttGE); 
	if( dnode ) {
		dnode->writeToFile( out ); 			
	} else { 
		tnode = dynamic_cast <TerminalNode *> (nodeAttGE);
		if ( tnode )
			tnode->writeToFile( out ); 
	}

}

//---------------------------------------------------------------
/* functions of RandomForest */

RandomForest::RandomForest() {}


RandomForest::~RandomForest()
{
	for(list<TreeNode*>::iterator p=rootNodeList.begin(); p!=rootNodeList.end(); ++p)
	{
		TreeNode* rootNode = *p;
		if (rootNode != 0) {
			delete rootNode; // this will delete the whole decision tree;
		}
	}
}


TerminalNode* RandomForest::testFeatPerTree( FeatureVector* feat, TreeNode* root, float* mat, int row )
{
	TerminalNode* leafNode = 0; 
	leafNode = root->classify( feat ); 
	if ( mat )
	{
		for ( int i=0; i<row; i++ )
			mat[i] = 0;
		map<int, float> temp = leafNode->getClassDistribution();
		for ( map<int, float>::iterator p=temp.begin(); p!=temp.end(); p++ )
			mat[p->first] += p->second;
	}
	return leafNode; 
}


list<TerminalNode*> RandomForest::testFeatInForest( FeatureVector* testFeatVec, float* mat, int row )
{
	float* tmp = NULL; 
	if ( mat )
	{	for ( int i=0; i<row; i++ )
			mat[i] = 0;
		tmp = new float [row];  
	}

	TerminalNode* leafNode = 0;
	list<TerminalNode*> leafNodeList;
	for( list<TreeNode*>::const_iterator it=rootNodeList.begin(); it!=rootNodeList.end(); it++ ) {
		 leafNode = testFeatPerTree( testFeatVec, *it, tmp, row ); 
		 if ( mat )	
		 {	for ( int i=0; i<row; i++ )
				mat[i] += tmp[i];
		 }	
		 leafNodeList.push_back( leafNode );
	}
	
	if ( mat )
	{	for ( int i=0; i<row; i++ )	
			mat[i] /= (int)rootNodeList.size();
		delete[] tmp; 
	}

	return leafNodeList;
}


void RandomForest::writeSubTree(std::ostream &out, int id)
{
	int i=0; 
	for ( list<TreeNode*>::iterator it=rootNodeList.begin(); it!=rootNodeList.end(); it++, i++ )
	{	
		if ( i==id )
		{	DecisionNode* root = dynamic_cast <DecisionNode *>(*it);		 
			root->writeToFile(out);
		} else 
			continue;  
	}
}


void RandomForest::writeForest( std::ostream &out )
{	
	int k=0; 
	for ( list<TreeNode*>::iterator it=rootNodeList.begin(); it!=rootNodeList.end(); it++, k++ )
	{	
		out << "TREE_ID;" << k << ";" << endl; 
		DecisionNode* root = dynamic_cast <DecisionNode *>(*it);		 
		root->writeToFile(out); 
	}
}


TreeNode* RandomForest::readSubTree (istream &in)
{
	// check locale
	assert( strcmp( setlocale( LC_NUMERIC, NULL), "C") == 0 );

	if (! in.eof() )
	{
		string s;
		getline(in,s);
		//cout << s ; 

		// split lines into values
		if (s.find("DecisionNode;") == 0)
		{
			unsigned int tree=0;
			unsigned int child_id=0; 
			unsigned int level=0;
			int feat_id=-1;
			int feat_type=-1; 
			int offset1_x=-1;
			int offset1_y=-1;
			int offset2_x=-1;
			int offset2_y=-1; 
			float threshold=0;
			double score=0; 
			if ( sscanf( s.c_str(), "DecisionNode;%u;%u;%u;%d;%d;%d;%d;%d;%d;%f;%lf;", &tree, &level, &child_id, &feat_id, &feat_type, &offset1_x, &offset1_y, &offset2_x, &offset2_y, &threshold, &score) == 11 )
			{
				//cout << "decision " << endl;  
				TreeNode *nodeAttSmaller = readSubTree(in);
				TreeNode *nodeAttGE = readSubTree(in);
				CvPoint offset1 = cvPoint(offset1_x, offset1_y);
				CvPoint offset2 = cvPoint(offset2_x, offset2_y);
				DecisionNode *dnode = new DecisionNode(feat_id, feat_type, offset1, offset2, threshold, nodeAttSmaller, nodeAttGE, score, tree, level, child_id);
				dnlist.push_back(dnode);
				nodeAttSmaller->parent = dnode;
				nodeAttGE->parent = dnode;
				return dnode;
			}
		} 
		if (s.find("TerminalNode;") == 0)
		{
			unsigned int tree=0;
			unsigned int level=0;
			unsigned int child=0;
			double* tmp = new double [PART_SIZE]; 
			if ( sscanf( s.c_str(), "TerminalNode;%u;%u;%u;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;", &tree, &level, &child, tmp, (tmp+1), (tmp+2), (tmp+3), (tmp+4), (tmp+5), (tmp+6), (tmp+7), (tmp+8), (tmp+9), (tmp+10) ) == 14 )
			{
				//cout << "terminal " << endl; 
				ClassLabelHistogram *nodeClassLabel = new ClassLabelHistogram(PART_SIZE); 
				for ( int m=0; m<PART_SIZE; m++ )
					nodeClassLabel->setScaleValue( m, (float)tmp[m] );  
				TerminalNode *tnode = new TerminalNode(nodeClassLabel, tree, level, child);
				tnlist.push_back(tnode);
				return tnode;
			}
			delete[] tmp; 
		}
	} // !eof
	return 0;
}


void RandomForest::readForeset( std::istream &in )
{
	string s;
	while ( getline(in,s) )
	{	
		int id; 
		sscanf(s.c_str(), "TREE_ID;%d;", &id ); 
		//cout << "tree id: " << id << endl;

		TreeNode* root = readSubTree (in); 
		if ( root )
			rootNodeList.push_back(root); 
	}
}

