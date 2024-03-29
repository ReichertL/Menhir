# Menhir
This repository contains the code for the paper "Menhir: An Oblivious Database with Protection against Access  and Volume Pattern Leakage" which has been accepted to the AsiaCSS 2024.

This is an implementation of Menhir, an oblivious database (ODB) for Trusted Execution Environments (TEEs).
The database is based on an oblivious AVL Tree construction implemented on top of PathORAM.
The AVL Tree relies on an ORAM which itself is not oblivious. 


## Server Platform: How to Setup an AMD SEV-SNP
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

## Installation Instructions for the Menhir Oblivious Database (ODB)
The following section describes how to set up Menhir on a Ubuntu 22.04 machine.
For instructions on what changes when using Amazon Linux (which uses yum as package manager), please refer to the **Dockerfile**.
The Dockerfile can be executed with `sudo docker build -t "menhir:Dockerfile" .`.

### Compile Path-ORAM
* Install boost: `sudo apt-get install libboost-all-dev`
```
cd pathoram
make shared
cd ..
make copy-libs-dev
sudo make install-libs
```

### Compile ODB:

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

### Running Tests

* Install GMock and GTest
    -> https://stackoverflow.com/questions/36086037/cant-build-c-c-program-with-gmock-gmock-h-too-many-errors-are-generated
* Build Tests: `cd menhir && make run-tests`
* To run the program properly after building tests, please run: `make clean`

### Change Settings
Settings for the ODB can be either changed via command line arguments or in `include/globals.hpp`

### To execute the ODB:
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


## To generate the figures from the paper:  
Check `evaluation/commands` for the python3 call to regenerate the plots of interest. 
The calls to `plot.py` must be made from the `evaluation` sub-directory.
The following python3 packages must be installed: numpy, pandas, seaborn, statsmodels, matplotlib.

## For profiling

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


## Comparison to Standard SQL Database

* Setup an SQL database like MariaDB or MySQL 
* Setup so users can connect via localhost without sudo for database user 'testleo'
* Run test file in folder `vanilla-comparison`
    ```python3 mariadb-testruns.py```
    Parameters like database size and number of datapoints retrieved can be set in the python file.
