import subprocess
from math import *
import sys
import os
from multiprocessing.pool import ThreadPool
import numpy as np
import os.path
import sqlite3
import psutil
import time

use_oram=True
useGamma=True
power=[16]
numDatapoints=[2**p-2**(p-16) for p in power] # each osm can store 2**16-1 datapoints by default

numQueries=100 
aggFunctions= ["SUM" ]
runPointQuery=0 
logLevel="INFO" 
logToFile=True
columnOptions=[21]
dataSourceString= "FROM_REAL"#"GENERATED"#"FROM_FILE" # Alternatives FROM_REAL or GENERATED
dataSetString="datasets/covid-data/covid-data-adjusted.csv" #only used if FROM_REAL is set 
insertBulkFlag=1 # insertion is done in a non-oblivious way to facilitate measurements
controlSelectivityFlag=1
valueSizes=[0]#[16*25-60,16*50-60,16*100-60, 16*300-60] #total block size needs to be devisable by 16, 60 is the default size with one column and no additional data stored
seeds=range(3,10)
minVals='1,1,1,1,-3,-2,-2,-2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,1,-2'
maxVals='2,13,2,2,487,2,2,121,2,2,2,2,2,2,2,2,2,2,2,7,2'
queryIndex=4
whereIndex=4

from itertools import chain
#domainSizes=[1000]#[1000,2000,4000,8000,16000,32000,64000,128000,256000] # domainsizes of less than 1000 so range(100,1000,100) 
selectivities=[0.1]#[0.04,0.05,0.06,0.07,0.08,0.09] #0.5
rangequery_ranges=[10]#[4,8,16,32,64,128]
#retrieveExactly=[1]#[10,20,30,40,50,60]
outdir="./results/covidData-size:2^20-query:4-where:4"

maxNumThreads=1
waitDuration=5  

def run_with_seed(s):   
    #print("Running one thread with seed "+str(s))
    fdir="~/tmp/storage-files-thread"+str(s)
    seed="--seed "+str(s)+" "
    numOptions=len(numDatapoints)*len(aggFunctions)*len(columnOptions)+len(selectivities)+len(valueSizes)
    # only integer columns
    i=0
    for nC in sorted(columnOptions,reverse=True):

        cols="i,"*nC
        cols=cols[0:-1]
        cols="-c "+cols+" "
        res="1,"*nC
        res=res[0:-1]
        res="-r '"+res+"' "

        for nD in sorted(numDatapoints,reverse=True):

            #logCapacity=ceil(log(nD)/log(2))
            datapoints="--datapoints "+str(nD)+" "

            for a in aggFunctions:

                agg="--agg "+a+" "                   
    
                for maxSelectivityValue in selectivities:
                    maxSelectivityString="--maxSensitivity "+str(maxSelectivityValue)+" "

                    for vs in valueSizes:
                                
                        for rangequeryrange in rangequery_ranges:
                            rangequeryrangeString=""
                            if not runPointQuery:
                                rangequeryrangeString="--rangeQueryRange "+str(rangequeryrange)+" "
            
                            i=i+1
                            queries="--numQueries "+str(numQueries)+" "
                            valueSize="--valueSize "+str(vs)+" "
                            pointQueries="--pointQueries "+str(runPointQuery)+" "

                            minValString="--minVals "+str(minVals)+" "
                            maxValString="--maxVals "+str(maxVals)+" "

                            queryIndexString="--queryIndex "+str(queryIndex)+" "
                            whereIndexString="--whereIndex "+str(whereIndex)+" "


                            verbosityStr="--verbosity "+str(logLevel)+" "
                            outdirStr="--outdir "+str(outdir)+" "
                            fileLoggingStr="--fileLogging "+str(logToFile)+" "
                                   
                            insertBulk="--insertBulk "+str(insertBulkFlag)+" "
                            controlSelectivity="--controlSelectivity "+str(controlSelectivityFlag)+" "
                            useGammaString="--useGamma "+str(useGamma)+" "
                            use_oramFlag="--useOram "+str(use_oram)+" "


                            thisfDir=str(fdir)+"-seed:"+str(s)
                            filesExists= os.path.isdir(thisfDir)
                            useExisting=(filesExists and (not createNewData))

                            if  dataSourceString=="FROM_FILE" and useExisting:
                                dataSource="--datasource "+str(dataSourceString)+" "    
                                dataSource+="--filesDir "+thisfDir+" "
                            elif dataSourceString=="FROM_REAL":
                                dataSource="--datasource "+str(dataSourceString)+" "
                                dataSource+="--dataset "+dataSetString+" "
                            else:
                                createNewData=False
                                dataSource="--datasource GENERATED "
                                dataSource+="--filesDir "+thisfDir+" "
                                        
                                        
                                    #command="cd " +path+";"
                            command="./bin/main "\
                                                +cols+res+datapoints\
                                                +dataSource\
                                                +use_oramFlag\
                                                +minValString+maxValString\
                                                +insertBulk\
                                                +agg+queries\
                                                +queryIndexString+whereIndexString\
                                                +pointQueries+rangequeryrangeString\
                                                +controlSelectivity+maxSelectivityString\
                                                +valueSize\
                                                +useGammaString\
                                                +seed\
                                                +verbosityStr+outdirStr+fileLoggingStr


                            print(command)
                            print("Thread "+str(s)+": Execution "+str(i)+"/"+str(numOptions)+ " ("+command+")", flush=True)
       
                            subprocess.run(command, shell=True,    
                                            stdout=subprocess.DEVNULL,
                                            #stdout=subprocess.STDOUT,
                                            stderr=sys.stdout)



def main():
    dbName="ram-lock.db"
    availableRAM=int((psutil.virtual_memory().total/ ( 1024 * 1024 *  1024))-3) # in GB
    if (not os.path.isfile(dbName)):
        cur = sqlite3.connect(dbName)
        cur.execute("CREATE TABLE LOCKED (ID INT PRIMARY KEY NOT NULL, USED INT NOT NULL);")
        cur.execute("INSERT INTO LOCKED (ID, USED) VALUES (0, "+str(availableRAM)+");")
        cur.commit()
        cur.close()

    global maxNumThreads
    if maxNumThreads==0:
        perThread=0 
        #esitmating number of threads
        c=max(columnOptions)
        i=round(log(max(numDatapoints))/log(2))-16
        if(c==1):
            perThread=col1[i]
        if(c==5):
            perThread=col5[i]
        else:
            factor=col5[i]/col1[i]
            perThread=factor*c
        
        peak=perThread*2.5
        maxNumThreads=floor((availableRAM-peak)/perThread)
        print("Calculated max Number of Thread is "+str(maxNumThreads))
        exit(0)

        


    num = min(len(seeds), maxNumThreads) # sqet to the number of workers you want (it defaults to the cpu count of your machine)
    print(str(num)+" Threads selected")
    tp = ThreadPool(num)
    results=list()
    for s in seeds:
        #if(s is not min(seeds)):
        #    print("Thread "+str(s)+" sleeps for "+str(waitDuration)+" minutes unitl started.")
        #    time.sleep(waitDuration*60) #time in seconds
        x= tp.apply_async(run_with_seed, (s,))
        results.append(x)
    for x in results:
        x.get()
    tp.close()
    tp.join()

main()