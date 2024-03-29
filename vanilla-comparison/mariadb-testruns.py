# Module Imports
import mariadb
import sys
import random 
import time
from math import *
import statistics
import scipy.stats
import numpy as np
random.seed(444)

numCols=5
numDatapoints=2**24
retrieve=[10,60,100,1000]
minVal=1000
maxVal=10000
repetitions=100
DB_NAME='PerformanceTests'
TABLE_NAME='tablePerformance'



def prepare_database():
  # Connect to MariaDB Platform
  try:
      conn = mariadb.connect(
          user="testleo",
          password="some_password",
          host="localhost"
              )
  except mariadb.Error as e:
      print(f"Error connecting to MariaDB Platform: {e}")
      sys.exit(1)

  # Get Cursor
  cur = conn.cursor()
  try:
    cur.execute("DROP DATABASE "+DB_NAME)  
  except:
    pass
  cur.execute("CREATE DATABASE "+DB_NAME)  

  cur.execute("SHOW DATABASES") 
  databaseList = cur.fetchall() 
    
  #for database in databaseList: 
  #  print(database) 


  #create Table
  colsStr=""
  for num,i in enumerate(range(numCols)):
    colsStr+="col_"+str(num)+" int"
    if num!=numCols-1:
      colsStr+=",\n"


  createTabStr="CREATE TABLE "+DB_NAME+"."+TABLE_NAME+" ("+colsStr+")"
  cur.execute(createTabStr)
  return cur,conn

def insertData(cur,conn):
  print("generate data")
  #create datapoints
  newVal=0
  datapoints=list()
  for num in retrieve:
    for i in range(0,10):
      repet=[newVal]*num
      datapoints.extend(repet)
      newVal+=1

  # fill remaining number of datapoins 
  remaining= numDatapoints-len(datapoints)
  for i in range(0,remaining):
    datapoints.append(random.randint(minVal,maxVal))

  print("insert data")
  #insert  datapoints
  colsStr=""
  for num,i in enumerate(range(numCols)):
    colsStr+="col_"+str(num)
    if num!=numCols-1:
      colsStr+=",\n"

  step_size=floor(numDatapoints*0.1)
  next_step=step_size
  stepnum=1

  batchSize=1000
  batchTotal=0

  dataStrBatch=''
  for i,dp in enumerate(datapoints):
    data=[str(dp)]*numCols
    dataStr=','.join(data)
    dataStrBatch+="("+dataStr+")"
    batchTotal+=1
    if batchTotal!=batchSize:
      dataStrBatch+=","
    
    if batchTotal==batchSize:
      strInsert= "INSERT INTO "+DB_NAME+"."+TABLE_NAME+" ("+colsStr+") VALUES("+dataStr+")"
      cur.execute(strInsert)
    
    if i==next_step:
        print(str(stepnum)+"0 %")
        next_step=next_step+step_size
        stepnum+=1
  
  conn.commit()

  #

def performanceQueries(cur):
  qVal=0 
  measurments=list()
  for num in retrieve:
    mSeries=list()
    for i in range(0,10):
      strQuery= "Select * From "+DB_NAME+"."+TABLE_NAME+" WHERE col_0="+str(qVal)
      start = time.thread_time_ns()
      cur.execute(strQuery)      
      end = time.thread_time_ns()
      mSeries.append((end-start)/1000000) #time in ms
      qVal+=1
    measurments.append(mSeries)
  return measurments

def mean_confidence_interval(data, confidence=0.95):
    a = 1.0 * np.array(data)
    n = len(a)
    m, se = np.mean(a), scipy.stats.sem(a)
    h = se * scipy.stats.t.ppf((1 + confidence) / 2., n-1)
    return h

def processData(measurments):
  print("num Datapoints:2^"+str(log(numDatapoints)/log(2)))
  for mSeries,num in zip(measurments,retrieve):
    mean_data=statistics.mean(mSeries)
    median_data=statistics.median(mSeries)
    std_data=statistics.stdev(mSeries)
    var_data=statistics.variance(mSeries)
    conf_data=mean_confidence_interval(measurments)
    print("   datapoints retrieved:"+ str(num))
    print("   mean "+str(mean_data)+" ms")
    print("   median "+str(median_data)+" ms")
    print("   std "+str(std_data)+" ms")
    print("   confidence interval "+str(conf_data)+" ms")

    #print("std "+std_data)

def cleanup(cur,conn):
  #remove database to reset original state
  cur.execute("DROP DATABASE "+DB_NAME)  

  cur.execute("SHOW DATABASES") 
  databaseList = cur.fetchall() 
    
  #for database in databaseList: 
  #  print(database) 
      
  conn.close() 


def main():
  cur,conn=prepare_database()
  insertData(cur,conn)
  print("running measurments")
  measurments= performanceQueries(cur)
  processData(measurments)
  cleanup(cur,conn)

main()