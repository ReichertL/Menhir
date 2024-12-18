# Menhir: An Oblivious Database with Protection against Access and Volume Pattern Leakage
This repository contains the code for the paper "Menhir: An Oblivious Database with Protection against Access and Volume Pattern Leakage"  by *Leonie Reichert, Gowri R Chandran, Phillipp Schoppmann, Thomas Schneider and Björn Scheuermann* which will be presented at the [ACM ASIA Conference on Computer and Communications Security](https://asiaccs2024.sutd.edu.sg/)(AsiaCCS '24). 
The paper is available [here](https://eprint.iacr.org/2024/556).


This repository contains an implementation of Menhir as well as resources for the evaluation. 
Menhir is an oblivious database (ODB) for Trusted Execution Environments (TEEs).
It is based on an oblivious AVL Tree construction implemented on top of PathORAM.

Please cite the paper when using our code in your research.

## 1. Installation 

### 1.1 Server Platform: How to Setup an AMD SEV-SNP
How to Setup an AMD SEV-SNP Server on AWS (Status: Spring 2024). 
Check here for information on AMD SEV-SNP on AWS: [AWS](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/sev-snp.html). 

To setup server: 
* Create AWS account
* Create an Instance as follows:
    * Select region: "US East (Ohio)" (us-east-2c)
    * Select instance type: r6a.4xlarge
    * Select additional storage (i.e. 100 GB)
    * Enable AMD SEV-SNP
    * Select OS/Platform: Amazon Linux 2023
* Once the new VM is created and ready, you can connect via ssh and start installing the Menhir Oblivious Database (ODB).

As reference, for our measurements we used 'Amazon Machine Image (AMI):  al2023-ami-2023.2.20230920.1-kernel-6.1-x86_64'.

### 1.2 Installation Instructions for the Menhir Oblivious Database (ODB)
The following section describes how to set up Menhir on a Ubuntu 22.04 machine.
For instructions on what changes when using Amazon Linux (which uses yum as package manager), please refer to the **Dockerfile**.
The Dockerfile can be executed with `sudo docker build -t "menhir:Dockerfile" .`.

#### Step 1: Compile Path-ORAM
* Install boost: `sudo apt-get install libboost-all-dev`

```
cd pathoram
make shared
cd ..
make copy-libs-dev
sudo make install-libs
```

#### Step 2: Compile ODB:

* First, make and install the `rpc` package. URL:  https://github.com/rpclib/rpclib/blob/master/doc/pages/compiling.md

```
    cd rpclib-2.3.0
    mkdir build
    cd build   
    cmake ..
    cmake --build .
    sudo make install
```
* Install openssl libraries: `sudo apt-get install libssl-dev`
* Install sqlite3 libraries: `sudo apt-get install sqlite3 libsqlite3-dev`


* Next, compile the executable.

```
cd menhir
make main
```
If errors occur during linking with libpathoram, rebuild path-oram and place `libpathoram.so` in `./lib`. Then install it again with :`sudo make install-libs`.

#### Step 3: Running Tests

* Install GMock and GTest
    -> https://stackoverflow.com/questions/36086037/cant-build-c-c-program-with-gmock-gmock-h-too-many-errors-are-generated
* Build Tests: `cd menhir && make run-tests`
* To run the program properly after building tests, please run: `make clean`

## 2. Execution / Command Line Arguments
Settings for the ODB can be either changed via command line arguments or in `include/globals.hpp`

Examples:
* Generate 2^16 data points and 20 range queries for a Database with two columns (both of type int). Inserts data points individually into the ODB and runs queries.
`./bin/main -d GENERATED --datapoints 65534 --seed 0 -c i,i --numQueries 20`

* To set the minimal and maximal value as well as data resolution for each column. This is relevant for the correct calculation of differentially private aggregates
`./bin/main -d GENERATED --datapoints 65534 --seed 0 -c i,i  --minVals '0,0' --maxVals '1000,1000' -r '1,10'  --numQueries 20`

* Running point queries
`./bin/main -d GENERATED --datapoints 65534 --seed 0 -c i,i --numQueries 20 --pointQueries 1`

* Adapt log level and write log to file 
`./bin/main -d GENERATED --datapoints 65534 --seed 0 -c i,i --numQueries 20 --verbosity DEBUG --fileLogging 1`

* To control the query sensitivity
`./bin/main -d GENERATED --datapoints 65534 --seed 0 -c i,i --numQueries 20 --controlSelectivity 1 --maxSensitivity 0.1`

* To change grade of parallelization (i.e. setting a different size for the ORAM of each ODB)
`./bin/main -d GENERATED --datapoints 65534 --seed 0 -c i,i --numQueries 20 --logcapacity 15`

* To run evaluation on for the naive approach which uses a linear scan 
`./bin/main -d GENERATED --datapoints 65534 --seed 0 -c i,i --numQueries 20 -useOram False`


* To run the ODB using an existing dataset
`./bin/main -d FROM_REAL  --dataset 'datasets/data-ipfs-2.csv' --datapoints 65534 --seed 0 -c i,i,i,i,i,i,i,i -r '1,1,1,1,1,1,1,1' --minVals 4,0,0,0,0,0,0,1695071408 --maxVals 6,906,65535,3,2,1139,4928637,1695067808  --agg MAX_COUNT --numQueries 20 --queryIndex 7 --whereIndex 7 --valueSize 0  --verbosity INFO --outdir ./results/test --fileLogging True`


## 3. Evaluation
To regenerate plots from the paper, check `evaluation/commands` for the corresponding python3 call. 
The calls to `plot.py` must be made from the `evaluation` sub-directory.
The following python3 packages must be installed: numpy, pandas, seaborn, statsmodels, matplotlib.

### 3.1 Datasets
The datasets used for evaluation can be found in the folder `datasets`. 
For information on the IPFS dataset can be found on this website: [link](https://monitoring.ipfs.trudi.group/).
Information such as IP address, peer address and content ID were replaced with pseudonyms.
When using this dataset, please also cite: 
```
@inproceedings{balduf2022monitoring,
  title={Monitoring data requests in decentralized data storage systems: A case study of IPFS},
  author={Balduf, Leonhard and Henningsen, Sebastian and Florian, Martin and Rust, Sebastian and Scheuermann, Bj{\"o}rn},
  booktitle={2022 IEEE 42nd International Conference on Distributed Computing Systems (ICDCS)},
  pages={658--668},
  year={2022},
  organization={IEEE}
}
```
The Covid dataset was taken from kaggle: [link](https://www.kaggle.com/datasets/meirnizri/covid19-dataset).
The original file was altered. Missing Values were originally written as 97,98,99 are now mapped as -1,-2,-2.
For 9999-99-99 in DATE_DIED, the value -3 was used. Dates in DATE_DIED are given as days since the 1.1.2020.



### 3.2 Profiling and Benchmarks

* For Benchmarking the ORAM:
```
make run-benchmarks
```

* For an analysis of the memory overhead 
```
valgrind --tool=massif \
./bin/main -c i -r '1' --datapoints 65534 --datasource GENERATED --filesDir /tmp/temp-storage/storage-files-thread7-seed:7 --minVals '10000' --maxVals '11000' --insertBulk 1 --agg SUM --numQueries 10 --queryIndex 0 --whereIndex 0 --pointQueries 0 --rangeQueryRange 10 --controlSelectivity 1 --maxSensitivity 0.09 --valueSize 0 --capacity 65534 --seed 7 --deletion False --verbosity INFO --outdir ./results/test --fileLogging True
```

* For an analysis of how different functions impact the runtime:
```
valgrind --tool=callgrind  --collect-atstart=no --instr-atstart=no \
./bin/main -c i -r '1' --datapoints 65534 --datasource GENERATED --filesDir /tmp/temp-storage/storage-files-thread7-seed:7 --minVals '10000' --maxVals '11000' --insertBulk 1 --agg SUM --numQueries 10 --queryIndex 0 --whereIndex 0 --pointQueries 0 --rangeQueryRange 10 --controlSelectivity 1 --maxSensitivity 0.09 --valueSize 0 --capacity 65534 --seed 7 --deletion False --verbosity INFO --outdir ./results/test --fileLogging True
```


### 3.3 Comparison to Standard SQL Database

* Setup an SQL database like MariaDB or MySQL 
* Setup so users can connect via localhost without sudo for database user 'testleo'
* Run test file in folder `vanilla-comparison`
    ```python3 mariadb-testruns.py```
    Parameters like database size and number of datapoints retrieved can be set in the python file.


## 4. Copyright Disclaimer

This code adapts individual functions from [Epsolute](https://github.com/epsolute/Epsolute) by  Dmytro Bogatov   under the [CC BY-NC 4.0 Deed](https://github.com/epsolute/epsolute/blob/master/LICENSE) license.
The corresponding functions are marked in the source code.

The Path Oram code by  Dmytro Bogatov is under the [CC BY-NC 4.0 Deed](https://github.com/epsolute/path-oram/blob/master/LICENSE) license. 

