#include "utility.hpp"
#include "output_utility.hpp"
#include "get_data_and_queries.hpp"

#include <boost/filesystem.hpp>

#include <numeric>
#include <string>
#include <thread>
#include <algorithm>
#include <iterator>

/**
 * @brief This file contains functions for generating artificial data an queries. The basis of these can either be real data, specifically generated normal distributions or uniform distributions.
 * The latter case is relevant when a fixed number of elements are supposed to exist for each key. 
 * 
 */

using namespace std;
using namespace DBT;
namespace MENHIR{

	/**
	 * @brief Checks the global setting to either read data from file or generate new data. Data is stored in the corresponding global variables.
	 * 
	 */
	void getDataset(){


		if (DATASOURCE==GENERATED){

			LOG(INFO, L"Constructing synthetic data set and queries...");
			generateArtificial();
			storeInputs(QUERIES, INPUT_DATA);
			//ORAM_CAPACITY =INPUT_DATA.size();
			//LOG_PARAMETER(ORAM_CAPACITY);
		}else if(DATASOURCE==FROM_REAL){
			LOG(INFO, L"Reading data from data set and constructing synthetic queries...");
			generateFromReal();
			storeInputs(QUERIES, INPUT_DATA);
			//ORAM_CAPACITY =INPUT_DATA.size();
			//LOG_PARAMETER(ORAM_CAPACITY);

		}else if(DATASOURCE==FROM_FILE){	
			//Does loads data and QUERIES from File.
			// Is this to connect to an existing database?
			LOG(INFO, L"Reading data and QUERIES from file...");
			//will load NUM_DATAPOINTS datapoints and NUM_QUERY queries if this amout is available
			loadInputs();
			NUM_DATAPOINTS =INPUT_DATA.size();
			NUM_QUERIES =QUERIES.size();
			//ORAM_CAPACITY = INPUT_DATA.size();
			LOG_PARAMETER(NUM_DATAPOINTS);
			LOG_PARAMETER(NUM_QUERIES);
			//LOG_PARAMETER(ORAM_CAPACITY);
			LOG_PARAMETER(NUM_ATTRIBUTES);

		}else{
			LOG(WARNING, L"No dataset loaded as no suitable DATASOURCE was provided. ");

		}
	}





	/**
	 * @brief Create a new data set from a real wold dataset. 
	 * If the passed dataset is larger than the number of required data points, only every nth row is added to the data set.
	 * Based on the data set and on the global settings, queries are generated.
	 * 
	 */
	void generateFromReal(){

		string line = "";
		double numLinesTotal=0;
		ifstream dataFile_count(DATASET);
		if (!dataFile_count.is_open()){
			LOG(CRITICAL, boost::wformat(L"File cannot be opened: %s") % toWString(DATASET));
		}
		while (std::getline(dataFile_count, line)){
        	numLinesTotal++;
		}
		dataFile_count.close();
		int samplingRate=floor(numLinesTotal/(double) NUM_DATAPOINTS);
		LOG(INFO, boost::wformat(L"Sampling Rate for Data File is %d, numLines %d") %samplingRate %numLinesTotal);


		ifstream dataFile(DATASET);
		if (!dataFile.is_open()){
			LOG(CRITICAL, boost::wformat(L"File cannot be opened: %s") % toWString(DATASET));
		}


		line = "";
		size_t numLine=0;
		vector<string> header;
		bool firstLine=true;

		while (getline(dataFile, line) and INPUT_DATA.size()<NUM_DATAPOINTS){
			if(not(numLine==0 or (numLine-1)%samplingRate ==0)){
				numLine=numLine+1;
				continue;
			}
		
			vector<string> vals;	
			boost::algorithm::split(vals, line, boost::is_any_of(","));
			if(numLine==0){
				header=vals;
				for(int i=0; i<(int) NUM_ATTRIBUTES;i++){
					LOG(INFO, boost::wformat(L"Column %d - %s ")  % i %toWString(vals[i])) ;
				}
				numLine=numLine+1;
				continue;
			}
			if (vals.size()<NUM_ATTRIBUTES){
				LOG(ERROR, boost::wformat(L"Column Format requires more columns than the the data file can provide. "\
					"Number of Attributes for expected column format %d. Number of columns read from file %d.") 
					% NUM_ATTRIBUTES %vals.size());
				throw std::invalid_argument( "Column format larger than max num columns in file." );
			}

			vector<db_t> datapoint;
			for(size_t i=0; i< NUM_ATTRIBUTES;i++){
				datapoint.push_back(DBT::fromString(vals[i],COLUMN_FORMAT[i]));
			}
			INPUT_DATA.push_back(datapoint);

			if(firstLine){
				for (size_t i=0;i<NUM_ATTRIBUTES;i++){
					MIN_VALUE.push_back(datapoint[i]);
					MAX_VALUE.push_back(datapoint[i]);
				}
				firstLine=false;

			}else{
				for (size_t i=0;i<NUM_ATTRIBUTES;i++){
					if(MAX_VALUE[i]<datapoint[i]){
						MAX_VALUE[i]=datapoint[i];
					}
					if(MIN_VALUE[i]>datapoint[i]){
						MIN_VALUE[i]=datapoint[i];
					}
				}
			}
			numLine=numLine+1;
		}
		dataFile.close();

		if(INPUT_DATA.size()<NUM_DATAPOINTS){
			LOG(CRITICAL, boost::wformat(L"Not enough datapoints in %s for NUM_DATAPOINTS %d. (read %d datapoints)") %toWString(DATASET) %NUM_DATAPOINTS %INPUT_DATA.size());
		}



		for(size_t i=0; i< NUM_ATTRIBUTES;i++){;

			LOG(INFO, boost::wformat(L"Column %d - min %s, max %s")  % i % DBT::toWString(MIN_VALUE[i]) %DBT::toWString(MAX_VALUE[i])) ;
		}

		if(POINT_QUERIES){
			LOG(INFO, L"generatePointQueries");
			generatePointQueries();
		}else if(CONTROL_SELECTIVITY){
			LOG(INFO, L"Generated Queries aim to provide certain levels of query senstivity. (MANAGE_SENSITIVITY is true.)");
			generateQueriesWithSensitivity();
		}else{
			generateQueries();
		}

	}

	/**
	 * @brief This function creates an artificial data set based on global settings. Results are stored in global variables.
	 * 
	 * If GENERATE_NORMAL_DISTRIBUTED_DATA is set, for each column data is generated following a normal distribution. Following the global settings on wether point queries or range queries.
	 * In the latter case, the selectivity of the newly generated queries  can be managed. This process follows a greedy fashion and might not terminate if parameters are set inconveniently.
	 * 
	 * If data should not follow a normal distribution, it will be uniformly distributed. This makes if simple to control the selectivity of the resulting queries.
	 * 
	 */
	void generateArtificial(){


		vector<float> true_means;
		vector<float> true_stddev;

		if(GENERATE_NORMAL_DISTRIBUTED_DATA){
			createDatasetNormalDistribution(&true_means, &true_stddev);
			if(POINT_QUERIES){
				LOG(INFO, L"Generating point queries. Selectivity can not be managed in this mode and WHERE_INDEX is ignored.");
				generatePointQueries();		
			}else if(CONTROL_SELECTIVITY){
				LOG(INFO, L"Generated Queries aim to provide certain levels of query senstivity. (MANAGE_SENSITIVITY is true.)");
				generateQueriesWithSensitivity();
			}else{
				generateQueries();
			}

		}else{
			if(CONTROL_SELECTIVITY){
				createDatasetandQueriesWithSelectivity(&true_means, &true_stddev);
			}else{
				createDatasetNormalDistribution(&true_means, &true_stddev);
				if(POINT_QUERIES){
					generatePointQueries();		
				}else{
					generateQueries();
				}
			}	
		}
	}


	/**
	 * @brief Generates a new dataset based on  mean and standard deviation (one for each column). Data is stored in the global variable INPUT_DATA.
	 * 
	 * @param ptrue_means : means for all columns
	 * @param ptrue_stddev : standard deviation for all columns
	 */
	void createDatasetNormalDistribution(vector<float> *ptrue_means, vector<float> *ptrue_stddev ){


		for(int i=0; i<(int)NUM_ATTRIBUTES;i++){
			double range= DBT::toDouble(MAX_VALUE[i] -MIN_VALUE[i]); 
			std::uniform_real_distribution<float>  dist_dev1(0, range*SPREAD);
			float stddev=dist_dev1(GENERATOR); 
			int count=500;
			while((DBT::toDouble(MIN_VALUE[i])+2*stddev > DBT::toDouble(MAX_VALUE[i])-2*stddev) and count >0){
				stddev=dist_dev1(GENERATOR); 
				count=count-1;
			}
			if (count==0){
				stddev=range*0.1;
				LOG(WARNING, boost::wformat(L"Column %d - setting std to  %d ")  % i %stddev) ;

			}

			ptrue_stddev->push_back(stddev);
			std::uniform_real_distribution<float>  dist_dev2(DBT::toDouble(MIN_VALUE[i])+2*stddev, DBT::toDouble(MAX_VALUE[i])-2*stddev); //substracting, so no no true min or max right at the edge is possible
			float random=dist_dev2(GENERATOR);
			ptrue_means->push_back(random);
			if((uint)i!=QUERY_INDEX)
				LOG(INFO, boost::wformat(L"Column %d - mean  %s, std %s ")  % i % random %stddev) ;
		}

		thread threads[NUM_THREADS];	
		promise<genType> promises[NUM_THREADS];
		future<genType> futures[NUM_THREADS];

		vector<int> splits;
		if(NUM_THREADS==1){
			splits.push_back(NUM_DATAPOINTS);
		}else{
			int sSize=floor(NUM_DATAPOINTS/NUM_THREADS);
			splits=vector<int>(NUM_THREADS-1,sSize);
			splits.push_back(NUM_DATAPOINTS-sSize*(NUM_THREADS-1)); //last split might be bigger to fit all datapoints
		}
		vector<float> tM=*ptrue_means;
		vector<float> tS=*ptrue_stddev;
		for (int i = 0; i < (int)NUM_THREADS; i++){
			futures[i] = promises[i].get_future();
			threads[i] = thread(
			generateRowsNormalDistribution,
			tM,
			tS,
			splits[i],
			&promises[i]);
	    }
	

		for (int i = 0; i <(int) NUM_THREADS; i++){
			auto[dataD, minC,maxC]= futures[i].get();
			threads[i].join();
			INPUT_DATA.insert(INPUT_DATA.end(), dataD.begin(), dataD.end());
			if(i==0){
				MAX_VALUE=maxC;
				MIN_VALUE=minC;
			}else{
				for(int j=0;j<(int)NUM_ATTRIBUTES;j++){
					if(MAX_VALUE[j]<maxC[j]) MAX_VALUE[j]=maxC[j];
					if(MIN_VALUE[j]>minC[j]) MIN_VALUE[j]=minC[j];
				}
			}
		}

		for(int i=0;i<(int)NUM_ATTRIBUTES;i++){
			wstring s_min=DBT::toWString(MIN_VALUE[i]); 
			wstring s_max= DBT::toWString(MAX_VALUE[i]);
			LOG(INFO, boost::wformat(L"Column %d - min %s, max %s")  % i % s_min %s_max) ;
		}
	}

	/**
	 * @brief Subfunction of createDatasetNormalDistribution for generating n rows of data. This function can be executed in parallel.
	 * 
	 * @param ptrue_means : means for all columns
	 * @param ptrue_stddev : standard deviation for all columns
	 * @param n : how many rows should be generated
	 * @param promise : for parallelization, contains result after execution. In this case, contains data for each column as well as their respective minimum and maximum.
	 */
	void generateRowsNormalDistribution(vector<float> ptrue_means,vector<float> ptrue_stddev, int n,promise<genType> * promise){

		vector<vector<db_t>> data;
		vector<db_t> minC;
		vector<db_t> maxC;


		for(int i=0; i<(int)NUM_ATTRIBUTES;i++){
			AType type=COLUMN_FORMAT[i];
			vector<db_t> data_column;
			float mean= ptrue_means[i];
			float absolute_stddev= ptrue_stddev[i] ;
			normal_distribution<> dist{ mean, absolute_stddev};

			for(int j=0;j<n;j++){
				if(i==0){
					vector<db_t> record;
					record.reserve(NUM_ATTRIBUTES);
					data.push_back(record);
				}
				auto v = dist(GENERATOR);
				db_t value;
				if(type==AType::FLOAT){ 

					value=db_t((float) v);
					if(value<MIN_VALUE[i]){
						value=MIN_VALUE[i];
					}
					if(value>MAX_VALUE[i]){
						value=MAX_VALUE[i];
					}
				
				}else if (type==AType::INT){
					value= db_t((int) round(v));
					if(value<MIN_VALUE[i]){
						value= MIN_VALUE[i];
					}
					if(value>MAX_VALUE[i]){
						value= MAX_VALUE[i];
					}
				}else{
					throw Exception("NOT IMPLEMENTED.");
				}
		
				data_column.push_back(value);
				data[j].push_back(value);
			}
			minC.push_back(min_vector(data_column));
			maxC.push_back(max_vector(data_column));

		}
		promise->set_value(make_tuple(data,minC,maxC));

	}


	/**
	 * @brief Generating point or range queries.
	 * In case of range queries  two elements are selected from the column to be queried uniformly at random.
	 * In case of point queries only one element is selected at random. Function generatePointQueries() is better suited for this purpose.
	 * Queries are stored in the global variable QUERIES. 
	 * 
	 */
	void generateQueries(){

		std::uniform_int_distribution<int>  dist(0, NUM_DATAPOINTS);

		for (number i = 0; i < NUM_QUERIES; i++){
			int lv= dist(GENERATOR);
			int lindex=lv%NUM_DATAPOINTS;
			vector<db_t> lrecord=INPUT_DATA[lindex];
			db_t l=lrecord[QUERY_INDEX];
			db_t lWhere=lrecord[WHERE_INDEX];
	
			
			db_t r;
			db_t rWhere=lWhere;
			int count=0;
			if(!POINT_QUERIES){
				while(rWhere==lWhere or count>100){
					int rv= dist(GENERATOR);
					int rindex=rv%NUM_DATAPOINTS;
					vector<db_t> rrecord=INPUT_DATA[rindex];
					r=rrecord[QUERY_INDEX];
					rWhere=rrecord[WHERE_INDEX];
					count=count+1;
				}

				if(lWhere>rWhere){
					db_t tmp=lWhere;
					lWhere=rWhere;
					rWhere=tmp;
				}

				if(WHERE_INDEX==QUERY_INDEX){
					l=lWhere;
					r=rWhere;
				}
				
				if(l>r){
					db_t tmp=l;
					l=r;
					r=tmp;
				}

			}
			
			Query query= {
					attributeIndex: QUERY_INDEX,
					from: l,
					to: r,
					pointQuery: POINT_QUERIES,
					whereIndex: WHERE_INDEX,
					whereFrom: lWhere,
					whereTo: rWhere,					
					agg: QUERY_FUNCTION,
					epsilon: QUERY_RESPONSE_EPSILON / (double)NUM_QUERIES,
					extra:0
			};

			string qstr=queryToString(query);
			LOG(DEBUG, boost::wformat(toWString(qstr)));
			QUERIES.push_back(query);
		}
		
	}


	/**
	 * @brief Generating point queries by removing all duplicates from the column to be queried and then selecting uniformly at random. Queries are stored in the global variable QUERIES.
	 * 
	 */
	void generatePointQueries(){

		vector<db_t> values;
		for (size_t i = 0; i < NUM_DATAPOINTS; i++){
			values.push_back(INPUT_DATA[i][QUERY_INDEX]);
		}
		LOG(DEBUG, boost::wformat(L"sorting..."));
		sort(values.begin(),values.end());
		auto last= unique(values.begin(), values.end());
		values.erase(last,values.end());

		vector<db_t> lVec;

		LOG(DEBUG, boost::wformat(L"values.size() %d - num Queries %d ") % values.size() % NUM_QUERIES);

		if(NUM_QUERIES>=values.size()){
			NUM_QUERIES=values.size();
			LOG(WARNING, boost::wformat(L"Can only compute  %d distinct Point Queries due to domain size")  % NUM_QUERIES);
			lVec=values;
		}else{
			std::shuffle(values.begin(), values.end(), GENERATOR);
			lVec=std::vector<db_t>(&values[0],&values[NUM_QUERIES]);;
		}
		for(number i=0;i<NUM_QUERIES;i++){
			db_t l=lVec[i];

	
			Query query= {
					attributeIndex: QUERY_INDEX,
					from: l,
					to: l,
					pointQuery: true,
					whereIndex: QUERY_INDEX,
					whereFrom: l,
					whereTo: l,					
					agg: QUERY_FUNCTION,
					epsilon: QUERY_RESPONSE_EPSILON / (double)NUM_QUERIES,
					extra:0
			};

			string qstr=queryToString(query);
			LOG(DEBUG, boost::wformat(toWString(qstr)));
			QUERIES.push_back(query);
		}
		LOG(DEBUG, boost::wformat(L"finished generatingh  %d queries ") % NUM_QUERIES);

	}

	/**
	 * @brief This function creates queries with different sensitivities to better compare runtimes. 
	 * It might not create queries with  the exact sensitivity though, as the underlying data might not allow this.
	 * Only works for range queries. 
	 * Ignores WHERE_INDEX for simplicity. 
	 * 
	 */
	void generateQueriesWithSensitivity(){
		int nmax=ceil((double)NUM_DATAPOINTS*MAX_SENSITIVITY);
		double step=MAX_SENSITIVITY/SENSITIVITY_STEPS;
		int n_step=ceil((double)NUM_DATAPOINTS*step);
		if(nmax==0 or n_step==0){
			LOG(CRITICAL, boost::wformat(L"number of dataoints to selet should not be 0 (nmax %d, n_step %d). Please change the max selectivity (currently %f) or number of steps (currently %f). ") %nmax %n_step %MAX_SENSITIVITY %SENSITIVITY_STEPS);
		}
		LOG(DEBUG, boost::wformat(L"(nmax %d, n_step %d). max selectivity (currently %f), number of steps (currently %f). ") %nmax %n_step %MAX_SENSITIVITY %SENSITIVITY_STEPS);
			
				
		WHERE_INDEX=QUERY_INDEX;

		vector<db_t> values;
		for (size_t i = 0; i < NUM_DATAPOINTS; i++){
			values.push_back(INPUT_DATA[i][QUERY_INDEX]);
			cout<<(DBT::toString(INPUT_DATA[i][QUERY_INDEX]))<< " ";
		}
		cout<<endl;
		sort(values.begin(),values.end());


		std::uniform_int_distribution<int>  dist(0, values.size()-1);
	
		int ni=0; //number of values required for the sensitivity of this query
		int i=0;
		int repetition=0;

		while(QUERIES.size()<NUM_QUERIES){			
			i=i+1;
			ni=ni+n_step;

			if(ni>nmax){
				ni=n_step;
			}
			LOG(INFO, boost::wformat(L"Iteration %d")  % repetition) ;
			LOG(DEBUG, boost::wformat(L"Targeted Num Datapoints for Query selectivity  %d")  % ni) ;

			int rand=dist(GENERATOR)%NUM_DATAPOINTS;
			db_t start=values[rand];

			LOG(DEBUG, boost::wformat(L"index %d, totalNumDatpoints %d, values.size %d")  %rand %NUM_DATAPOINTS  %values.size()) ;
			LOG(DEBUG, boost::wformat(L"value %s")   %DBT::toWString(start)) ;

			size_t startPos;
			for(size_t pos=0;pos<NUM_DATAPOINTS;pos++){
				if(values[pos]==start){
					startPos=pos;
					break;
				}
			}
			size_t endPos=startPos;
			size_t current=startPos;
			int n_tmp=0;
			while(n_tmp<(ni*0.99)){
				size_t countCurrent=1;
				while(values[current]==values[endPos]){
					if(current==(NUM_DATAPOINTS-1)){
						LOG(DEBUG, boost::wformat(L"End of data reached."));
						break;
					}
					current++;
					countCurrent++;

				}
				n_tmp=n_tmp+countCurrent;
				endPos=current;

				if(current==(NUM_DATAPOINTS-1)){
					break;
				}
			}
			repetition++;
			LOG(DEBUG, boost::wformat(L"startpos %d, endpos %d")  %startPos %endPos) ;
			LOG(DEBUG, boost::wformat(L"startpos  val %s, endpos val %s")  %DBT::toWString(values[startPos]) %DBT::toWString(values[endPos])) ;

			LOG(DEBUG, boost::wformat(L"Actual number of datapoints for query %d. Targeted number: %d")  % n_tmp %ni) ;
			if(n_tmp<ni+ni*0.01){
				Query query= {
							attributeIndex: QUERY_INDEX,
							from: values[startPos],
							to: values[endPos],
							pointQuery: false,
							whereIndex: WHERE_INDEX,
							whereFrom: values[startPos],
							whereTo: values[endPos],					
							agg: QUERY_FUNCTION,
							epsilon: QUERY_RESPONSE_EPSILON / (double)NUM_QUERIES,
							extra:0
						};
				string qstr=queryToString(query);
				LOG(DEBUG, boost::wformat(toWString(qstr)));
				QUERIES.push_back(query);
				double sel=(double)n_tmp/(double) NUM_DATAPOINTS;
				LOG(INFO, boost::wformat(L"Adding Query with selectivity %2.4f (%d datapoints). Num Generated Queries %d/%d")  %sel %n_tmp % QUERIES.size() % NUM_QUERIES) ;
				repetition=0;
			}
			
		}	
	}

//..........................when GENERATE_NORMAL_DISTRIBUTED_DATA in false............................................................................................................


	/**
	 * @brief Generates a new data set and queries for a given maximal query selectivity.
	 * The column to be queried is uniformly distributed. All other columns will follow a normal distribution.
	 *  Data is stored in the global variable INPUT_DATA.
	 * 
	 * @param ptrue_means : means for all columns
	 * @param ptrue_stddev : standard deviation for all columns
	 */
	void createDatasetandQueriesWithSelectivity( vector<float> *ptrue_means, vector<float> *ptrue_stddev ){

		for(int i=0; i<(int)NUM_ATTRIBUTES;i++){
			AType type=COLUMN_FORMAT[i];
			double range= DBT::toDouble(MAX_VALUE[i] -MIN_VALUE[i]); 
			std::uniform_real_distribution<float>  dist_dev1(0, range*SPREAD);
			float stddev=dist_dev1(GENERATOR); 
			ptrue_stddev->push_back(stddev);

			std::uniform_real_distribution<float>  dist_dev2(DBT::toDouble(MIN_VALUE[i])+2*stddev, DBT::toDouble(MAX_VALUE[i])-2*stddev); //substracting, so no no true min or max right at the edge is possible

			if(type==AType::FLOAT or type==AType::INT){
				float random=dist_dev2(GENERATOR);
				ptrue_means->push_back(random);
				if((uint)i!=QUERY_INDEX)
					LOG(INFO, boost::wformat(L"Column %d - mean  %s, std %s ")  % i % random %stddev) ;
			}else{
				throw Exception("NOT IMPLEMENTED.");
			}
		}
		thread threads[NUM_ATTRIBUTES];	
		promise<genType2> promises[NUM_ATTRIBUTES];
		future<genType2> futures[NUM_ATTRIBUTES];
		
		vector<float> tM=*ptrue_means;
		vector<float> tS=*ptrue_stddev;
		for (int i = 0; i < (int)NUM_ATTRIBUTES; i++){
			if((uint)i==QUERY_INDEX){
				futures[i] = promises[i].get_future();
				threads[i] = thread(
				generateColumnWithSelectivity,
				i,
				tM,
				tS,
				&promises[i]);
			}else{
				futures[i] = promises[i].get_future();
				threads[i] = thread(
				generateColumnNormalDistribution,
				i,
				tM,
				tS,
				&promises[i]);
			}
	    }

		vector<vector<db_t>> columns;
		for (int i = 0; i <(int) NUM_ATTRIBUTES; i++){
			vector<db_t> column= futures[i].get();
			threads[i].join();
			columns.push_back(column);
		}
		for(number i=0;i<NUM_DATAPOINTS;i++){
			vector<db_t> record;
			for(uint j=0;j<NUM_ATTRIBUTES;j++){
				record.push_back(columns[j][i]);
			}	
			INPUT_DATA.push_back(record);
		}

		for(int i=0;i<(int)NUM_ATTRIBUTES;i++){
			wstring s_min=DBT::toWString(MIN_VALUE[i]); 
			wstring s_max= DBT::toWString(MAX_VALUE[i]);
			LOG(INFO, boost::wformat(L"Column %d - min %s, max %s")  % i % s_min %s_max) ;
		}
	}

	/**
	 * @brief Subfunction of createDatasetandQueriesWithSelectivity. Generates one column (the column to be queried) so queries will have fixed levels of selectivity.
	 * All queries will have the same selectivity which is given by the global variable MAX_SENSITIVITY. 
	 * Also generates queries (either point or range queries). These are stored in the global variable QUERIES.
	 * 
	 * 
	 * @param index : index of the column to generate
	 * @param ptrue_means : means for all columns
	 * @param ptrue_stddev : standard deviations for all columns
	 * @param promise : for parallelization, contains result after execution. In this case, contains data for the new column.
	 */
	void generateColumnWithSelectivity(int index, vector<float> ptrue_means,vector<float> ptrue_stddev, promise<genType2> * promise){

		db_t minC=MIN_VALUE[index];
		db_t maxC=MAX_VALUE[index];

		vector<db_t> data_column;
		db_t res=DATA_RESOLUTION[index];
		int allowedRange=(int)(DBT::toDouble(maxC-minC)*0.01);
		int n_sel=ceil((double)NUM_DATAPOINTS*MAX_SENSITIVITY);
		LOG_PARAMETER(n_sel);
		if(n_sel*NUM_QUERIES> NUM_DATAPOINTS){
			NUM_QUERIES=floor(NUM_DATAPOINTS/n_sel);
			LOG(WARNING,boost::wformat(L"Error in Query generation process (More datapoints supposed to be create" \
										"for queries than the maximal size of datapoints: %d>%d). Decreasing Number of Queries to %d. ")
										% (n_sel*NUM_QUERIES) %NUM_DATAPOINTS %NUM_QUERIES
										);
		}
		if(POINT_QUERIES){
			db_t point=minC;
			for(number i=0;i<NUM_QUERIES;i++){
				Query query= {
					attributeIndex: QUERY_INDEX,
					from: point,
					to: point,
					pointQuery: POINT_QUERIES,
					whereIndex: WHERE_INDEX,
					whereFrom: point,
					whereTo: point,					
					agg: QUERY_FUNCTION,
					epsilon: QUERY_RESPONSE_EPSILON / (double)NUM_QUERIES,
					extra:0
				};
				string qstr=queryToString(query);
				LOG(DEBUG, boost::wformat(toWString(qstr)));
				QUERIES.push_back(query);
				for(int j=0;j<n_sel;j++){
					data_column.push_back(point);
				}
				point=point+res;
				if(point>=maxC){
					LOG(CRITICAL,L"Error in Query generation process (Value of generated point query became larger than allowed). Either increase Domain Size or decrease POSSIBLE_TRUE_MAX. " );
				}
			}

			while(data_column.size()<NUM_DATAPOINTS){
				data_column.push_back(maxC);
			}
		}else {
			db_t value=minC;
			int rangesize=RANGEQUERY_RANGE;
			//int rangesize=1;TODO
			for(number i=0;i<NUM_QUERIES;i++){
				
				db_t lower=value;
				db_t upper=value+db_t(rangesize)*res;

				Query query= {
					attributeIndex: QUERY_INDEX,
					from: lower,
					to: upper,
					pointQuery: POINT_QUERIES,
					whereIndex: WHERE_INDEX,
					whereFrom: lower,
					whereTo: upper,					
					agg: QUERY_FUNCTION,
					epsilon: QUERY_RESPONSE_EPSILON / (double)NUM_QUERIES,
					extra:0
				};
				string qstr=queryToString(query);
				LOG(DEBUG, boost::wformat(toWString(qstr)));
				QUERIES.push_back(query);
				for(int ii=0;ii<n_sel;ii++){
					int j=ii % rangesize;
					data_column.push_back(value+db_t(j)*res);
				}
				value=value+(res)*db_t(rangesize+1);
				//rangesize=(rangesize% allowedRange) +1; TODO
				if(value>=maxC){
					LOG(CRITICAL,L"Error in Query generation process (Value of generated point query became larger than allowed). Either increase Domain Size or decrease POSSIBLE_TRUE_MAX. " );
				}
			}
			if(data_column.size()>NUM_DATAPOINTS){
				LOG(CRITICAL,L"Error in Query generation process (More datapoints were to be create for queries than the maximal size of datapoints). Pleas decrease the number of queries, or increase Domain Size (change either POSSIBLE_TRUE_MAX,POSSIBLE_TRUE_MIN or DATA_RESOLUTION) " );
			}

			while(data_column.size()<NUM_DATAPOINTS){
				data_column.push_back(maxC);
			}
		}
		promise->set_value((data_column));

	}

	/**
	 * @brief Generates a data column following a normal distribution.
	 * 
	 * @param index : index of the column to generate
	 * @param ptrue_means : means for all columns
	 * @param ptrue_stddev : standard deviations for all columns
	 * @param promise : for parallelization, contains result after execution. In this case, contains data for the new column.
	 */
	void generateColumnNormalDistribution(int index, vector<float> ptrue_means,vector<float> ptrue_stddev, promise<genType2> * promise){

		AType type=COLUMN_FORMAT[index];
		vector<db_t> data_column;
		float mean= ptrue_means[index];
		float absolute_stddev= ptrue_stddev[index] ;
		normal_distribution<> dist{ mean, absolute_stddev};

		for(number j=0;j<NUM_DATAPOINTS;j++){
			auto v = dist(GENERATOR);
			db_t value;
			if(type==AType::FLOAT){ 

				value=db_t((float) v);
				if(value<MIN_VALUE[index]){
					value=MIN_VALUE[index];
				}
				if(value>MAX_VALUE[index]){
					value=MAX_VALUE[index];
				}
				
			}else if (type==AType::INT){
				value= db_t((int) round(v));
				if(value<MIN_VALUE[index]){
					value= MIN_VALUE[index];
				}
				if(value>MAX_VALUE[index]){
					value= MAX_VALUE[index];
				}
			}else{
				throw Exception("NOT IMPLEMENTED.");
			}
		
			data_column.push_back(value);
		}
		db_t minC= (min_vector(data_column));
		db_t maxC=(max_vector(data_column));
		
		promise->set_value((data_column));

	}

}
