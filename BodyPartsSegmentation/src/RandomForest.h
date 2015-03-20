#ifndef RANDOMFOREST_H
#define RANDOMFOREST_H

#include "support_class.h"
#include <ostream>

class TerminalNode;

//----------------------------------------------------------------
/* virtual treenode class */

class TreeNode {

public:
  
  virtual ~TreeNode() {};
  virtual TerminalNode* classify ( FeatureVector* featureVec ) = 0;
  virtual void writeToFile (std::ostream &out) = 0;
  
  TreeNode *parent;
  unsigned int level;
  unsigned int tree;
  unsigned int child_id;  // 0 is left child, 1 is the right child
  double score;

};

//----------------------------------------------------------------
/* Terminal node */

class TerminalNode : public virtual TreeNode {

public:
	TerminalNode(ClassLabelHistogram *nodeClassLabel, unsigned int nbOfTree, unsigned int treeLevel, int child, std::list<FeatureVector*> *featureVecList = 0);
	virtual ~TerminalNode();

	virtual TerminalNode* classify ( FeatureVector* featureVec );
	virtual void writeToFile(std::ostream &out);	
	int getID() {return id;};
	void setID(int newID) {id=newID;};
	unsigned int getOfTree() { return tree;};
	std::map<int,float> getClassDistribution();
  
	static int getMaxID() {return idCounter;};
	static int totalTerminalNodeCount;
	ClassLabelHistogram classDistribution;
  
private:
	int id;
	static int idCounter;
  
};

//------------------------------------------------------------------
/* decision node */

class DecisionNode : public virtual TreeNode {
public:  
	DecisionNode( int id, int type, CvPoint u, CvPoint v, float splitThreshold, TreeNode* nodeAttributeSmaller, TreeNode* nodeAttributeGE, double scoreVal, unsigned int nbOfTree, unsigned int treeLevel, int child );
	virtual ~DecisionNode();     // will delete whole subtree!!!

	virtual TerminalNode* classify ( FeatureVector* featureVec );
	virtual void writeToFile(std::ostream &out);
 
  	int feat_id;
	int feat_type;  
	CvPoint offset_1;
	CvPoint offset_2;
	float threshold;
	TreeNode* nodeAttSmaller;
 	TreeNode* nodeAttGE;
  
	static unsigned int totalDecisionNodeCount;

private:
  //none
};


//-------------------------------------------------------------------
/* Random forest class */

class RandomForest {
	
public:

	RandomForest ();
	~RandomForest();
 
	TerminalNode* testFeatPerTree( FeatureVector* feat, TreeNode* root, float* mat=NULL, int row=0 );
	std::list<TerminalNode*> testFeatInForest ( FeatureVector* testFeatVec, float* mat=NULL, int row=0 );

	void writeSubTree(std::ostream &out, int id);
	TreeNode* readSubTree (std::istream &in); 

	void writeForest( std::ostream &out ); 
	void readForeset( std::istream &in );

	std::list<TreeNode*> rootNodeList;
        std::list<DecisionNode*> dnlist;       // list with all decision nodes, for pruning purposes
        std::list<TerminalNode*> tnlist;       // list with all terminal nodes, for external use
        
private:

};

#endif
