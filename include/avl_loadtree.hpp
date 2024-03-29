//#include "database_type.hpp"
#include "avl_treenode.hpp"
#include "globals.hpp"
//#include "avl_multiset.hpp"    

namespace DOSM{
using namespace MENHIR;
tuple<vector<pair<number, bytes>>, vector<ulong>>  createTreeStructureNonObliv(vector<vector<db_t>> *inputData, size_t num, size_t valueSize, vector<AType> columnFormat, size_t oramBlockSize);
//void loadTreeBulkNonObliv(size_t num, AVLTree *avl_tree);
tuple<ulong,vector<AVLTreeNode>> buildTreeFromSortedList(vector<AVLTreeNode>  allNodes, int sortIndex);
//ulong buildTreeFromSortedList(vector<AVLTreeNode>  allNodes, int sortIndex);
tuple<ulong,int> updateTreeConstruction(vector<AVLTreeNode> *nodes, int start,int sizeSubArray, int index);

//void createTreeNonObliv(vector<vector<db_t>> records,AVLTree *avl_tree);


tuple<AVLTreeNode ,ulong,uint> balanceNonObliv(vector<AVLTreeNode> *ALLnodes, number tsize, AVLTreeNode node, ulong nodePtr, ulong column);


}

