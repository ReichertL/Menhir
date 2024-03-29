#include "avl_loadtree.hpp"
#include "utility.hpp"
#include <stdexcept>
#include "definitions.h"

namespace DOSM{

/**
 * @brief Creates a valid binary tree from the passed input data in a non-oblivious manner. 
 * First, AVLTreeNodes are created for each data points. Then, using a sorted list of nodes, the pointers of each node are updated so it stores information about child nodes.
 *  
 * @param inputData 
 * @param num : size of inputData
 * @param valueSize 
 * @param columnFormat 
 * @param oramBlockSize 
 * @return tuple<vector<pair<number, bytes>>, vector<ulong>> 
 */
tuple<vector<pair<number, bytes>>, vector<ulong>>  createTreeStructureNonObliv(vector<vector<db_t>> *inputData, size_t num, size_t valueSize, vector<AType> columnFormat, size_t oramBlockSize){
    LOG(INFO, boost::wformat(L"Creating Tree Structure in non-oblivious manner from sorted lists.") );
    vector<AVLTreeNode> allNodes;
    allNodes.reserve(num);
    vector<ulong> nodePtrs;
    vector<ulong> thisRoots(columnFormat.size(), 1);
    bytes value=vector<uchar>(valueSize, 0);

    for (size_t i = 0; i < inputData->size(); i++){
        vector<db_t> key=(*inputData)[i];//->back();
        //inputData->pop_back();
        ulong nodeID=(ulong) i+1;

        std::ostringstream oss;
        for (size_t j = 0; j < key.size(); j++){
            oss<<DBT::toString(key[j])<<",";
        }

        allNodes.push_back(AVLTreeNode(key,nodeID,value,columnFormat,nodeID));
    }

	auto cmpNodes=[](AVLTreeNode n1, AVLTreeNode n2 )->bool{
			if(n1.nodeID<n2.nodeID){
				return true;
			}
			return false;
	};

    sort(allNodes.begin(),allNodes.end(),cmpNodes);
    (*inputData).clear();

    //updating the child pointers of all nodes for all attributes
    LOG(INFO, boost::wformat(L"starting to build Tree") );
    #pragma omp parallel for num_threads(16)
    for(size_t i=0;i<columnFormat.size();i++){
        LOG(INFO, boost::wformat(L"building Tree from sorted list for Colum %d") %i );
        auto[rootI, thisNodes]=buildTreeFromSortedList(allNodes,i);
        thisRoots[i]=rootI;
        sort(thisNodes.begin(),thisNodes.end(),cmpNodes);
        for(size_t j=0;j<allNodes.size();j++){
            allNodes[j].ptrRightChild[i]=thisNodes[j].ptrRightChild[i];
            allNodes[j].ptrLeftChild[i]=thisNodes[j].ptrLeftChild[i];
            allNodes[j].rHeight[i]=thisNodes[j].rHeight[i];
            allNodes[j].lHeight[i]=thisNodes[j].lHeight[i];
            allNodes[j].next[i]=thisNodes[j].next[i];
        }


        LOG(INFO, boost::wformat(L"Finished building tree from sorted list for Colum %d") %i );
    }
    //unordered_map<ulong,ulong> nodeIDs;

    vector<pair<number, bytes>> data;
    for(size_t i=0;i<allNodes.size();i++){
        AVLTreeNode n=allNodes[i];
        bytes nodeBytes=n.padToBlockSize(oramBlockSize);
        data.push_back(make_pair(n.nodeID+1,nodeBytes));

    }
    LOG(INFO, boost::wformat(L"data size %d") %data.size() );
  
    
    return make_tuple(data,thisRoots);
}


/**
 * @brief Sorts the passed list of nodes for sortIndex, then calls a function which updates all childe nodes for sortIndex
 * Note: Right subtree will be bigger if both subtrees do not have same number of nodes.
 * @param allNodes 
 * @param sortIndex 
 * @return tuple<ulong,vector<AVLTreeNode>>
 */
tuple<ulong,vector<AVLTreeNode>> buildTreeFromSortedList(vector<AVLTreeNode>  allNodes, int sortIndex){
	auto cmpNodes=[sortIndex](AVLTreeNode n1, AVLTreeNode n2 )->bool{
			if(n1.key[sortIndex]<n2.key[sortIndex]){
				return true;
			}
            if(n1.key[sortIndex]==n2.key[sortIndex] and n1.nodeHash<n2.nodeHash){
                return true;
            }
			return false;
	};

    sort(allNodes.begin(),allNodes.end(),cmpNodes);


    //skip last entry because it needs to remain 0
    for(size_t i=0;i<(allNodes).size()-1;i++){
        (allNodes)[i].next[sortIndex]=(allNodes)[i+1].nodeID;
    }

    ulong root;
    int height;
    tie(root, height)= updateTreeConstruction(&allNodes,0,(allNodes).size(),sortIndex);
    


    if(CURRENT_LEVEL==DEBUG and allNodes.size()<1000){
        for(size_t i=0;i<allNodes.size();i++){
            LOG( DEBUG,boost::wformat( L"%s") %toWString((allNodes)[i].toStringFull(1)));
        }
    }
    
    return make_tuple(root,allNodes);
    
}

/**
 * @brief Recursive function which based on a sorted vector of AVLTreeNodes adapts the 
 * pointers to Left and Right Children.  
 * The function returns if only only one or two nodes are left.
 * Note: The right subtree will always be bigger if the number of nodes is not equal 
 * in both subtrees->
 * 
 * @param nodes 
 * @param index 
 * @return tuple<ulong,vector<AVLTreeNode>> 
 */
tuple<ulong,int> updateTreeConstruction(vector<AVLTreeNode> *nodes,int start,int sizeSubArray, int index){
    LOG(TRACE, boost::wformat(L"start %d, sizeSubArray %d , index %d\n") %start %sizeSubArray %index);

    if(sizeSubArray==1){
        (*nodes)[start].ptrLeftChild[index]=0;
        (*nodes)[start].ptrRightChild[index]=0;
        return make_tuple((*nodes)[start].nodeID,1);
    }if(sizeSubArray==2){
        (*nodes)[start].ptrRightChild[index]=(*nodes)[start+1].nodeID;
        (*nodes)[start].rHeight[index]=1;
        return make_tuple((*nodes)[start].nodeID, (*nodes)[start].height(index));
    }else{
        int index_mid, size_lo, size_hi;
        if(sizeSubArray%2==0){
            //even number
            size_lo=sizeSubArray/2;
            index_mid=(start+size_lo);
            size_hi=size_lo-1;
        }else{
            //uneven number
            size_lo=floor(sizeSubArray/2);
            index_mid=start+size_lo;
            size_hi=size_lo;
        }
        int start_hi=index_mid+1;
        auto[lC,  lH]=updateTreeConstruction(nodes, start, size_lo,index);
        auto[rC, rH]=updateTreeConstruction(nodes, start_hi,size_hi,index );


        (*nodes)[index_mid].ptrLeftChild[index]=lC;
        (*nodes)[index_mid].ptrRightChild[index]=rC;
        (*nodes)[index_mid].lHeight[index]=lH;
        (*nodes)[index_mid].rHeight[index]=rH;

        return make_tuple((*nodes)[index_mid].nodeID, (*nodes)[index_mid].height(index));
        
    }
}

/**
 * @brief Oblivious right rotation in AVL tree. Only for use in avl_loadtree.cpp.
 * 
 * @param node 
 * @param nodePtr 
 * @param L 
 * @param column 
 */
void rightRotate(AVLTreeNode *node,ulong nodePtr, AVLTreeNode *L, ulong column){

    node->ptrLeftChild[column]=L->ptrRightChild[column];
    L->ptrRightChild[column]=nodePtr;
    node->lHeight[column]=L->rHeight[column];
    L->rHeight[column]=node->height(column);


}

/**
 * @brief Oblivious left rotate operation in AVL tree. Only for use in avl_loadtree.cpp.
 * 
 * @param node 
 * @param nodePtr 
 * @param R 
 * @param column 
 */
void leftRotate(AVLTreeNode *node,ulong nodePtr, AVLTreeNode *R, ulong column){

    node->ptrRightChild[column]=R->ptrLeftChild[column];
    R->ptrLeftChild[column]=nodePtr;
    node->rHeight[column]=R->lHeight[column];
    R->lHeight[column]=node->height(column);
        

}

/**
 * @brief Non-oblivious balance operation for AVL tree. Only for use in avl_loadtree.cpp.
 * 
 * @param allNodes 
 * @param tsize 
 * @param node 
 * @param nodePtr 
 * @param column 
 * @return tuple<AVLTreeNode ,ulong,uint> 
 */
tuple<AVLTreeNode ,ulong,uint> balanceNonObliv(vector<AVLTreeNode> *allNodes, number tsize, AVLTreeNode node, ulong nodePtr, ulong column){
    //if(CURRENT_LEVEL<=TRACE)
    LOG(TRACE, boost::wformat(L"start balance on %d----------------------------------------------------------------------")%nodePtr);
    if(tsize<=2){
        //no balance needed if there are only two nodes in the tree
        tuple<AVLTreeNode ,ulong,uint>  result= make_tuple(node,nodePtr,node.height(column));
        //if(CURRENT_LEVEL<=TRACE)
        LOG(TRACE, boost::wformat(L"end balance (No balance needed)---- ptr:%d height: %d--------") % nodePtr  % node.height(column));
        return result;
    }
    //if(CURRENT_LEVEL<=TRACE)
    LOG(TRACE, L"BalanceNode: "+MENHIR::toWString(node.toString(true,column)));

    ulong l_ptr=node.ptrLeftChild[column];
    ulong r_ptr=node.ptrRightChild[column];
    AVLTreeNode L=(*allNodes)[node.ptrLeftChild[column]];
    AVLTreeNode R=(*allNodes)[node.ptrRightChild[column]];

    int n_balanceFactor= node.balanceFactor(column);
    int l_balanceFactor= L.balanceFactor(column);
    int r_balanceFactor= R.balanceFactor(column);

    if(n_balanceFactor>= 2 and l_balanceFactor>=1){
        if(CURRENT_LEVEL<=TRACE)
            LOG(TRACE, L"Right Rotate");
        //right rotate
        rightRotate(&node, nodePtr,&L, column);
        (*allNodes)[nodePtr]=node;
        (*allNodes)[l_ptr]=L;

        
        if(CURRENT_LEVEL<=TRACE)
            LOG(TRACE, boost::wformat(L"end balance ---- ptr:%d height: %d--------") % nodePtr  % node.height(column));
        return make_tuple(L, l_ptr, L.height(column));

    }else if(n_balanceFactor>=2){

        if(CURRENT_LEVEL<=TRACE)
            LOG(TRACE, L"Left Right Rotate");
    
        ulong lr_ptr=L.ptrRightChild[column];
        AVLTreeNode LR=(*allNodes)[L.ptrRightChild[column]];

        //left rotate in left subtree
        leftRotate(&L,l_ptr,&LR,column);

        //change node : leftsubtree
        node.ptrLeftChild[column]=lr_ptr;
        node.lHeight[column]=LR.height();

        //right rotate around node
        rightRotate(&node,nodePtr,&LR,column);

        (*allNodes)[nodePtr]=node;
        (*allNodes)[l_ptr]=L;
        (*allNodes)[lr_ptr]=LR;

        if(CURRENT_LEVEL<=TRACE)
            LOG(TRACE, boost::wformat(L"end balance ---- ptr:%d  height: %d--------") % nodePtr % node.height(column));
        return make_tuple(LR, lr_ptr, LR.height(column));
        
    }else if(n_balanceFactor<=-2 and r_balanceFactor<=-1){
        if(CURRENT_LEVEL<=TRACE)
            LOG(TRACE,L"Left Rotate");

        //left rotate
        leftRotate(&node, nodePtr,&R,column);
        
        (*allNodes)[nodePtr]=node;
        (*allNodes)[r_ptr]=R;

        if(CURRENT_LEVEL<=TRACE)
            LOG(TRACE, boost::wformat(L"end balance ---- ptr:%d height: %d--------") % nodePtr % node.height(column));
        return make_tuple(R, r_ptr, R.height(column));

        
    }else if(n_balanceFactor<=-2){
        if(CURRENT_LEVEL<=TRACE)
            LOG(TRACE,L"Right Left Rotate");
        ulong rl_ptr=R.ptrLeftChild[column];
        AVLTreeNode RL=(*allNodes)[R.ptrLeftChild[column]];

        //left rotate in left subtree
        rightRotate(&R,r_ptr,&RL,column);

        //change node : leftsubtree
        node.ptrRightChild[column]=rl_ptr;
        node.rHeight[column]=RL.height();


        //right rotate around node
        leftRotate(&node,nodePtr,&RL,column);
        (*allNodes)[nodePtr]=node;
        (*allNodes)[r_ptr]=R;
        (*allNodes)[rl_ptr]=RL;

        if(CURRENT_LEVEL<=TRACE)
            LOG(TRACE, boost::wformat(L"end balance ---- ptr:%d  height: %d--------") % nodePtr  % node.height(column));
        return make_tuple(RL, rl_ptr, RL.height(column));        
    }
    
    return make_tuple(node,nodePtr,node.height(column));

}


}