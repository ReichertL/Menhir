#include "osm_interface.hpp"



/**
 * @brief This file contains functions for parallelized access to multiple Oblivious Sorted Muli-Map (OSM). Implementation wise, each OSM is an AVLTree where each Tree is stored in a separate ORAM. 
 * For volume pattern sanitation purposes, for each attribute one or multiple histograms have to be maintained. 
 * These histograms cover the data domain of the corresponding column and contain information for which value, how many data points exist.
 * 
 * To test the naive approach, the OSM can also be a LinearDB. 
 * Parallelization improves the runtime of queries.
 * 
 */


/**
 * @brief Construct a new OSMInterface::OSMInterface object based on global variables.
 * First computes how many Oblivious sorted multi map (AVL Trees) are required to store all data. Then generate volume sanitizers. 
 * INPUT_DATA is split and the new OSMs are created in parallel.
 * 
 */
OSMInterface::OSMInterface(){

    this->maxPerTree=pow(2,ORAM_LOG_CAPACITY)-1;
    LOG_PARAMETER(this->maxPerTree);
    this->numOSMs=ceil((double)NUM_DATAPOINTS/(double)this->maxPerTree );
    LOG_PARAMETER(this->numOSMs);
    this->USE_ORAM=USE_ORAM;
    
    size_t numGenerate=NUM_ATTRIBUTES;
    if(!USE_GAMMA){
        numGenerate*=this->numOSMs;
    }
    for(size_t i=0;i<numGenerate;i++){
        LOG(INFO, boost::wformat(L"Generating sanitizers for  %d/%d. (USE GAMMA %d)") %(i+1) %numGenerate %USE_GAMMA);
        generateVolumeSanitizer();
    }    

    //This is to ensure that incase data is read from file, different OSMs are created depending on the inital seed. Only for measuring purposes.
    shuffle(INPUT_DATA.begin(),INPUT_DATA.end(),GENERATOR);

    int end=NUM_DATAPOINTS-1;
    vector<vector<vector<db_t>>> inputDataSplits;
    for(size_t osmIndex=0;osmIndex<this->numOSMs;osmIndex++){
        LOG(INFO, boost::wformat(L"Creating OSM data split %d/%d.") %(osmIndex+1) %this->numOSMs );       
        size_t thisNumAtStart=this->maxPerTree;
        if(osmIndex==(this->numOSMs-1)){
            thisNumAtStart=INPUT_DATA.size()%(this->maxPerTree+1);
        }        
        int start=end-thisNumAtStart+1;
        LOG(INFO, boost::wformat(L"start %d, end %d, num %d") %start %end %thisNumAtStart);

        vector<vector<db_t>>  inputSplit(INPUT_DATA.begin()+start, INPUT_DATA.begin()+end);
        inputDataSplits.push_back(inputSplit);
        end=start-1;
        for(size_t i=0;i<thisNumAtStart;i++){
            INPUT_DATA.pop_back();
        }
    }
   
	thread threads[this->numOSMs];	
	promise<tuple<void*,vector<hist_t>>> promises[this->numOSMs];
	future<tuple<void*,vector<hist_t>>> futures[this->numOSMs];
    for (size_t osmIndex = 0; osmIndex <this->numOSMs; osmIndex++){
		vector<vector<db_t>> inputSplit=inputDataSplits[osmIndex];
        size_t thisSize=inputSplit.size();
            if(USE_ORAM){
            LOG(INFO, boost::wformat(L"Creating AVLTree for OSM  %d/%d with %d datapoints") %(osmIndex+1) %this->numOSMs %thisSize);
        }else{
            LOG(INFO, boost::wformat(L"Creating LinearOblivDB for OSM  %d/%d with %d datapoints") %(osmIndex+1) %this->numOSMs %thisSize);
        }
        futures[osmIndex] = promises[osmIndex].get_future();
		threads[osmIndex] = thread(
			&OSMInterface::createNewTreeWithDataParallel,
            this,
            osmIndex,
            inputSplit,
            thisSize,
			&promises[osmIndex]);
	}

    for(size_t osmIndex=0; osmIndex<this->numOSMs;osmIndex++){
		void * ptr;
        vector<hist_t> osmHistos;
        tie(ptr, osmHistos)=futures[osmIndex].get();
		threads[osmIndex].join();
        if(USE_ORAM){
            DOSM::AVLTree *oblivTree=(DOSM::AVLTree *) ptr;
            this->trees.push_back(oblivTree);
        }else{
            LinearDB::LinearOblivDB *oblivList =(LinearDB::LinearOblivDB *) ptr;
            this->lists.push_back(oblivList);
        }
        this->histograms.push_back(osmHistos);

    }

}


/**
 * @brief Creates  a new Oblivious Sorted Multi-map ("OSM",AVL Trees) using a given data. 
 * Additionally, this function creates a histogram for each attribute over the domain from the given data. Ths is later used for volume sanitation. 
 * 
 * @param osmIndex : index of the Oblivious Sorted Multi-map ("OSM", AVL Trees) to be created
 * @param inputSplit : split of INPUT_DATA to be used for creating this OSM
 * @param thisSize : How many data points from inputSplit are to be used for creating the OSM
 * @param promise : For parallelization, after execution this contains a pointer to the OSM and  the histograms.
 */
void OSMInterface::createNewTreeWithDataParallel(size_t osmIndex, vector<vector<db_t>> inputSplit, size_t thisSize,promise<tuple<void *,vector<hist_t>>> *promise){
    LOG(INFO, boost::wformat(L"Creating Histograms for OSM  %d.") %(osmIndex+1) );

    vector<hist_t> osmHistos=createNewHistogram();
    for(size_t j=0;j<thisSize;j++){
        vector<db_t> element=inputSplit[j];
        for(size_t att=0;att<NUM_ATTRIBUTES;att++){
            size_t histoIndex=(size_t) abs(DBT::toDouble(element[att])-DBT::toDouble(MIN_VALUE[att]))/DBT::toDouble(DATA_RESOLUTION[att]);
            osmHistos[att][histoIndex].second+=1ull;
        }
    }

    void * ptr;
    if(USE_ORAM){
        DOSM::AVLTree *oblivTree=new DOSM::AVLTree(COLUMN_FORMAT, VALUE_SIZE, ORAM_LOG_CAPACITY, ORAM_Z, STASH_FACTOR, BATCH_SIZE, &inputSplit, thisSize, USE_ORAM);
        ptr=(void *) oblivTree;
    }else{
        LinearDB::LinearOblivDB *oblivList=new LinearDB::LinearOblivDB(COLUMN_FORMAT,&inputSplit, thisSize);
        ptr=(void *) oblivList;
    }      
    promise->set_value(make_tuple(ptr,osmHistos));
}

/**
 * @brief Creates  a new Oblivious Sorted Multi-map ("OSM",AVL Trees) without any data. 
 * The new OSM is added to the corresponding data structure in OSMInterface, either this->trees or this->lists.
 * 
 */
void OSMInterface::createNewTree(){
    if(USE_ORAM){
        vector<vector<db_t>> emptyVec;
        DOSM::AVLTree *oblivTree=new DOSM::AVLTree(COLUMN_FORMAT, VALUE_SIZE, ORAM_LOG_CAPACITY, ORAM_Z, STASH_FACTOR, BATCH_SIZE, &emptyVec, 0, USE_ORAM);
        this->trees.push_back(oblivTree);
    }else{
		LinearDB::LinearOblivDB* oblivList=new LinearDB::LinearOblivDB(COLUMN_FORMAT);
        this->lists.push_back(oblivList);
    }
}

/**
 * @brief Creates a new histogram data structure without any data.
 * 
 * @return vector<hist_t> 
 */
vector<hist_t> OSMInterface::createNewHistogram(){
    vector<hist_t> histsForNewOSM;
    for(size_t i=0;i<NUM_ATTRIBUTES;i++){
        hist_t thisHisto;
        size_t numBuckets=(size_t)ceil((DBT::toDouble(MAX_VALUE[i])-DBT::toDouble(MIN_VALUE[i])+1.0)/DBT::toDouble(DATA_RESOLUTION[i]));
        db_t val=MIN_VALUE[i];
        for(size_t j=0;j<numBuckets;j++){
            thisHisto.push_back(make_pair(val,0ull));
            val=val+DATA_RESOLUTION[i];

        }
        histsForNewOSM.push_back(thisHisto);
    }
    return histsForNewOSM;
}

/**
 * @brief Inserts a new data point into the database (without providing a value). 
 * The new data point is inserted into the last OSM. The corresponding histogram is updated accordingly.
 * 
 * @param key : data point to be inserted 
 * @return size_t : Hash of the corresponding entry.
 */
size_t OSMInterface::insert(vector<db_t> key){
    if(this->trees.back()->size()==maxPerTree){
        createNewTree();
        vector<hist_t> osmHistos =createNewHistogram();
        this->histograms.push_back(osmHistos);
        this->numOSMs=this->numOSMs+1;
    }
    size_t hash=0;
    if(USE_ORAM){   
        hash=this->trees.back()->insert(key);
    }else{
        hash=this->lists.back()->insert(key);
    }
    return hash;
}

#ifndef NDEBUG
/**
 * @brief Inserts a new data point into the database (without providing a value). The hash is already fixed.
 * The new data point is inserted into the last OSM. The corresponding histogram is updated accordingly.
 * 
 * 
 * @param key : data point to be inserted 
 * @param nodeHash : Value to be used as hash of the corresponding entry
 */
void OSMInterface::insert(vector<db_t> key, size_t nodeHash){

    if(this->trees.back()->size()==maxPerTree){
        createNewTree();
        createNewHistogram();
        this->numOSMs=this->numOSMs+1;       
    }
    if(USE_ORAM){   
        this->trees.back()->insert(key,nodeHash);
    }else{
        this->lists.back()->insert(key,nodeHash);
    }
}
#endif

/**
 * @brief Delete entry from the database. For this purpose, for each OSM the delete operation is called. Only in one OSM actually data is deleted, in the others dummy operations are conducted.
 * 
 * TODO: Also delete from Histograms! Maybe store information about which OSM was used in the hash.
 * @param key : Key in the passed column of the node to be deleted
 * @param nodeHash : Hash of the node to be deleted
 * @param column : column for the key 
 */
void OSMInterface::deleteEntry(db_t key, size_t nodeHash, ulong column){
    #pragma omp for
    for(size_t i=0;i<this->numOSMs;i++){
        this->trees[i]->deleteEntry(key,nodeHash,column);
    }
}  

/**
 * @brief Get the Total number of data points in each ODB for a query by scanning all histograms. Runtime depends on the domain size of the input data. This algorithm is oblivious.
 * 
 * @param from : start point of the interval
 * @param to : end point of the interval
 * @param column : column for which data is queried
 * @return tuple<size_t,vector<size_t>> 
 */
tuple<number,vector<number>> OSMInterface::getTotalFromHistograms(db_t from, db_t to, size_t column){
    vector<number> counts;
    number total=0;
    for(size_t osmIndex=0; osmIndex<this->numOSMs;osmIndex++){
        auto histogram=this->histograms[osmIndex][column];
        number count=0;
        for(size_t i=0;i<histogram.size();i++){
            pair<db_t,number> entry=histogram[i]; //getting the value and its frequency
            bool inInterval= (entry.first>= from and entry.first<=to);
            count+=_IF_THEN(inInterval, entry.second, (number)0);
        }
        counts.push_back(count);
        total+=count;
    }
    return make_tuple(total,counts);
}


/**
 * @brief Queries an OSM to find all elements that fall into the queried interval. 
 * This function is a wrapper for parallelization, called by findInterval().
 * 
 * @param osmIndex : Index of the OSM to be queried
 * @param startKey : Start of the interval
 * @param endKey : end of the interval
 * @param column : Column to be queried
 * @param estimate : number of data points to retrieve for volume sanitation from this OSM
 * @param promise : for parallelization, is set after execution. Then it contains a list of records and information wether these are dummies or not.
 */
void OSMInterface::findIntervalParallel(size_t osmIndex, db_t startKey, db_t endKey, ushort column, number estimate,promise<dbResponse> *promise){
    dbResponse  records;
    //LOG(INFO,boost::wformat(L"Parallelized accessing osm %d") %osmIndex);

    if(USE_ORAM){
        records= this->trees[osmIndex]->findIntervalMenhir(startKey, endKey, column, estimate);
    }else{
        records=this->lists[osmIndex]->findInterval(startKey, endKey, column, estimate);
    } 
    promise->set_value(records);
}


/**
 * @brief Query all OSMs in parallel to find all elements that fall into the queried interval. 
 * 
 * @param startKey : Start of the interval
 * @param endKey : end of the interval
 * @param column : Column to be queried
 * @param estimates  : vector of containing information for each OSM, how many data points have to be retrieved for volume sanitation
 * @return dbResponse : Contains a list of records and information wether these are dummies or not.
 */
dbResponse OSMInterface::findInterval(db_t startKey, db_t endKey, ushort column, vector<number> estimates){

   
	thread threads[this->numOSMs];	
	promise<dbResponse> promises[this->numOSMs];
	future<dbResponse> futures[this->numOSMs];

	timestampBeforeORAMs = chrono::steady_clock::now();

    for (size_t i = 0; i <this->numOSMs; i++){
        LOG(INFO, boost::wformat(L"estimate %d  for dosm %i") % estimates[i] %i);
		futures[i] = promises[i].get_future();
		threads[i] = thread(
			&OSMInterface::findIntervalParallel,
            this,
            i,
            startKey, 
            endKey, 
            column, 
            estimates[i],
			&promises[i]);
	}


    dbResponse allRecords;
    for(size_t i=0; i<this->numOSMs;i++){
		dbResponse records= futures[i].get();
		threads[i].join();
        LOG(INFO, boost::wformat(L"got %d datapoints from osm %d ") % records.size() %i);
        if(RETRIEVE_EXACTLY.size()!=0 and i!=0 ){
            continue;
        }
        allRecords.insert(end(allRecords), begin(records), end(records));
    }
    timestampAfterORAMs = chrono::steady_clock::now();


    return allRecords;
}



/**
 * @brief This function calculates the noise required to mitigate volume pattern leakage from queries. 
 * Depending on the range and resolution of each column, a volume sanitizer data structure is calculated. 
 * 'dp_level' of the volume sanitizer is a tree covering the potential domain of the data. Each node at the leaf level is one resolution, nodes further up the tree cover intervals. 
 * The root node covers the whole domain. 
 * For each node, differentially private noise is generated with either the Laplace function (as used by Epsolute) or with a Truncated Laplace function (Menhir).
 * 
 * The volume sanitizer data structure will later be used when computing how many dummy nodes are retrieved for each query.
 * Code adapted from Epsolute (https://github.com/epsolute/Epsolute).
 * 
 *
 */
void OSMInterface::generateVolumeSanitizer(){
	double beta=1.0 / (1 << DP_BETA);

    //For each column a new volume sanitizer data structure is generated
	for(int i=0;i< (int) NUM_ATTRIBUTES;i++){
		double range=DBT::toDouble(MAX_VALUE[i]-MIN_VALUE[i]); 
		double resolution= DBT::toDouble(DATA_RESOLUTION[i]);
		auto this_DP_domain = ceil(range/resolution);
		number this_dp_buckets=DP_BUCKETS;
		number this_dp_levels=DP_LEVELS;
		auto this_dp_alpha=0;

		//this is for columns containing binary data. Here, range queries do not make sense, so we set the default for point queries.
		if(this_DP_domain==1 or POINT_QUERIES){
			this_dp_levels=1;
			this_dp_buckets=this_DP_domain; 
			this_dp_alpha = optimalMuForPointQueries(beta, this_dp_buckets, DP_EPSILON);


		}else{
			//Setting the number of buckets for our noise histogram.
			if (this_dp_buckets == 0){
				this_dp_buckets = 1;
    			while (this_dp_buckets * DP_K <= this_DP_domain){
					this_dp_buckets =  this_dp_buckets * DP_K;
				}

					//This is to prevent one very large bucket.
				if(this_dp_buckets==1){
					this_dp_buckets=this_DP_domain; 
				}
			}
			auto this_dp_levels = (number)(log(this_dp_buckets) / log(DP_K));
			this_dp_alpha = optimalMuForRangeQueries(beta, DP_K, this_dp_buckets, DP_EPSILON, this_dp_levels);
				
		}

		VolumeSanitizer np(
			1, this_dp_buckets, this_dp_levels,
			this_DP_domain, this_dp_alpha);
		this->volumeSanitizers.push_back(np);

		wstring s_min=DBT::toWString(MIN_VALUE[i]); 
		wstring s_max= DBT::toWString(MAX_VALUE[i]);
		LOG(INFO, boost::wformat(L"Noise Parameters Column %d - min %s, max %s, range %g")  % i % s_min %s_max  % range) ;
		LOG_PARAMETER(this_DP_domain);
		LOG_PARAMETER(this_dp_buckets);
		LOG_PARAMETER(this_dp_levels);
		LOG_PARAMETER(this_dp_alpha);

	}

	LOG(INFO, L"Generating DP noise tree...");

    // For each column, the dp_level (DP noise tree) of the corresponding volume sanitizer is computed and stored for later use. 
	for(int ii=0; ii< (int) NUM_ATTRIBUTES; ii++){
		VolumeSanitizer* np= &this->volumeSanitizers[ii];
		auto buckets = np->dp_buckets;
		map<pair<number, number>, number> m;

		for (auto l = 0uLL; l < np->dp_levels; l++){
			for (auto j = 0uLL; j < buckets; j++){
				pair<number,number> key= make_pair(l,j);
				//if point query or range =1 then np->dp_levels is 1 resulting in sensitivity 1/epsilon
				//else it will be the same as log_k(N)/epsilon
                int value=0;
                double sensitivity=np->dp_levels; //if one value was different, this is how many nodes in the DP tree would change 
                double lambda= sensitivity/ DP_EPSILON; 
                //retrieve DP noise for this tree node 
                if (USE_TRUNCATED_LAPLACE){
                    double t=sensitivity+sensitivity*log(2/beta)/DP_EPSILON; //cpp default base for log is e
    	            // The expected performance of this function is worse than when computing computing on continuos values. This is especially relevant as the domain between lower and upper truncation edge is very large.
                    //value= (int) sampleTruncatedLaplace_integer(t, lambda,0,this->maxPerTree);
                    value= round(sampleTruncatedLaplace_double(t, lambda,0,this->maxPerTree));
                        
                }else{
                    value = (int) sampleLaplace(np->dp_alpha, lambda);
                }
                m[key] = value;
			}
			buckets /= DP_K;
		}
		np->noises=m;
		// count number of nodes in DP tree
		number dpNodes = np->noises.size();			
		LOG(INFO, boost::wformat(L"For column %1%: DP tree has %2% elements") % ii % dpNodes);
	}
}


