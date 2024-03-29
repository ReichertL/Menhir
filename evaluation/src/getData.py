import os
import json
import pandas as pd


def getData(files, dirpath):
    if dirpath!= []:
        for path in dirpath:
            for fileF in os.listdir(path):
                if fileF.endswith(".json"):
                    files.append(os.path.join(path, fileF))      

    datasets=list()
    for filepath in files:
        #print(filepath)
        f=open(filepath)
        d=json.load(f)
        datasets.append(d)

    colnames= [     "COLUMN_FORMAT","NUM_ATTRIBUTES",
                    "NUM_DATAPOINTS","POINT_QUERIES",
                    "NUM_QUERIES","BLOCK_SIZE",
                    "QUERY_FUNCTION", 
                    "USE_ORAM" ]

    colsaggregates=[  "timePerQuery","paddingPerQuery",
                    "noisePerQuery","totalPerQuery",
                    "timeTotal"]

    colnames_new= [     "columnFormat","numAttributes",
                    "numDatapoints","pointQuery",
                    "numQueries","blockSize",
                    "queryFunction", 
                    "useORAM", 

                    "timePerQuery","paddingPerQuery",
                    "noisePerQuery","totalPerQuery",
                    "timeTotal"]

    types=[str,int,int,str,int,int,
            #int,
            str,bool]
    types2=[float,int,int,int,float]

    rows=list()
    i=0
    for d in datasets: 
        #print(d["LOG_FILENAME"])
        row=list()
        for name,t in zip(colnames,types):
            row.append(t(d[name]))
        for name,t in zip(colsaggregates, types2):
            row.append(t(d["aggregates"][name]))
        row.append(i)
        rows.append(row)
        i=i+1
    colnames_new=colnames_new+["RefIndex"]
    df=pd.DataFrame(rows,columns=colnames_new)
    return [datasets,df]



