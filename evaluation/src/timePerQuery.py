

import matplotlib.pyplot as plt
import pandas as pd
from math import *
import numpy as np
import statsmodels.stats.api as sms
from src.util import *




def numDatapointToTimePerQuery_nFixed(df,outdir,colors):
    qFunctions=df["queryFunction"].unique().tolist()
    pointQuery=df["pointQuery"].unique().tolist()

    n=65536
    for pQ in pointQuery:
        for qf in qFunctions :
            df_tmp=df[df["pointQuery"]==pQ]
            df_tmp=df_tmp[df_tmp["numDatapoints"]==n]
            df_tmp=df_tmp[df_tmp["numQueries"]==100]

            y=df_tmp["numAttributes"].unique().tolist()

            min_rep=1000000000
            max_rep=0
            box_data={}
            for yi in y:
                df_tmp2=df_tmp[df_tmp['numAttributes']==yi]
                df_tmp2['totalPerQuery']=df_tmp2['totalPerQuery']/1000000
                df_mean=df_tmp2.mean(numeric_only=True)
                df_std=df_tmp2.std(numeric_only=True)
                df_count=df_tmp2.count()
                count=df_count["columnFormat"].min()

                box_data[yi]=df_tmp2['totalPerQuery'].tolist()
                min_rep=min(min_rep,len( box_data[yi]))
                max_rep=max(max_rep,len( box_data[yi]))

       
            fig, ax = plt.subplots()   
            ax.boxplot(box_data.values())
            ax.set_xticklabels(box_data.keys())

            plt.xlabel("Number of Attributes")
            plt.ylabel("Time per Query in ms")
            plt.legend()
            name="/TimePerQuery-perNumAttributes"\
                    +"-n:"+str(n)\
                     +"-PointQuery:"+str(pQ)\
                    +"-QueryFunction:"+str(qf)\
                    +"-minrep:"+str(min_rep)\
                    +"-maxrep:"+str(max_rep)
            plt.tight_layout()
            plt.savefig(outdir+name+".png")
            plt.savefig(outdir+name+".svg")



def numDatapointToTimePerQuery(datasets,outdir,colors, markers,linestyles):


    #pQ=True
    #pQ=False
    qF=0#->"SUM"
    #nQ=100

    dbFormats=list()
    #allowedCols=[1,5]
    retirevedExactly=1
    for d in datasets:
        thisFormat=d["COLUMN_FORMAT"]
        if thisFormat not in dbFormats:
           #if(len(thisFormat.split(",")) in allowedCols):
            dbFormats.append(thisFormat)


    i=0
    df=pd.DataFrame()
    #max_selectivity=5 #%
    for thisFormat in dbFormats:
        numAttributes=0;
        data=list()
        #isPQ=True
        #if d["POINT_QUERIES"] == "false":
        #    isPQ=False
        for d in datasets: 
            if d["COLUMN_FORMAT"]==thisFormat and int(d["QUERY_FUNCTION"])==qF:
                numAttributes=int(d["NUM_ATTRIBUTES"])
                queries=list()
                for index,t in enumerate(d["QUERIES"]):
                    if index==0:
                        #to mitigate measurments bias due to caching effects
                        continue
                    tnew=dict()
                    tnew["numAttributes"]=int(d["NUM_ATTRIBUTES"])
                    tnew["numDatapoints"]=int(d["NUM_DATAPOINTS"])
                    tnew["selectivity"]=int(t["real"])/tnew["numDatapoints"]*100 # to get %
                    #old measurments did not collect oram time seperatly
                    tnew["overhead"]=int(t["overhead"])/1000000 #overhead is in nanoseconds
                   #tnew["overhead"]=int(t["orams"])/1000000 #to get ms
                    if int(t["total"])==retirevedExactly:
                        queries.append(tnew)
                data=data+queries
        df2=pd.DataFrame(data)
        df = pd.concat([df, df2], ignore_index=True, sort=False)      


    #df_tmp=df[df['numAttributes']==5]
    #df_tmp=df_tmp[df_tmp['numDatapoints']==2097120]
    #print(df_tmp)

    numAttributes=df['numAttributes'].unique().tolist()
    
    i=len(numAttributes)-1
    for numA in sorted(numAttributes,reverse=True):

        df_this=df[df['numAttributes']==numA]

        df_mean=df_this.groupby(["numDatapoints"]).mean().reset_index()
        df_median=df_this.groupby(["numDatapoints"]).median().reset_index()
        df_count=df_this.groupby(["numDatapoints"]).count().reset_index()



        def fci(x):
            a,b=sms.DescrStatsW(x).tconfint_mean()
            m=float(sms.DescrStatsW(x).mean)
            a_rel=abs(float(a)-m)
            b_rel=abs(float(b)-m)
            return a_rel, b_rel
        
        df_ci=pd.DataFrame(df_this.groupby(["numDatapoints"])["overhead"].apply(fci).tolist(), columns=["upper","lower"])

        ci_tuples=[df_ci["lower"].tolist(), df_ci["upper"].tolist()]

        thislabel=str(numA)+" columns"
        if numA==1:
            thislabel=str(numA)+" column"

        
        print(ci_tuples)
        print(df_mean["numDatapoints"])
        print(df_mean["overhead"])

        plt.errorbar(df_mean["numDatapoints"],df_mean["overhead"],yerr=ci_tuples, capsize=5, linestyle=linestyles[i%len(linestyles)], marker=markers[i%len(markers)], label=thislabel,color=colors[i%len(colors)])
        
        xfit=np.power(np.log(df_mean["numDatapoints"])/log(2),2)
        #xfit=np.sqrt(df_mean["numDatapoints"])#np.power(np.log(df_mean["numDatapoints"]),2)
        yfit=df_mean["overhead"]
        res=np.polyfit(xfit,yfit , 1)
        print(str(res[0])+"*log^2(x)+"+str(res[1]))
        datapoints=[2**i for i in range(24,28)]
        y=[res[0]*pow(log(x)/log(2),2)+res[1] for x in datapoints]
        #y=[res[0]*sqrt(x)+res[1] for x in datapoints]
        #plt.plot(datapoints,y, linestyle=linestyles[i%len(linestyles)], marker=markers[i%len(markers)],  color="#808080")

        print(res)
        i=i-1


    #datapoints=df['numDatapoints'].unique().tolist()
    #y=[(0.05035356)*log(x)*log(x)/log(2)-14.758617141534057 for x in datapoints]
    #plot log^2(n) and n line
    #plt.plot(datapoints,y, marker=".",label="log^2(n)/log(2)",  color="#808080")

    #y2=[(1/1000000)*x for x in datapoints]
    #plot log^2(n) and n line
    #plt.plot(datapoints,y2, marker=".",label="log^2(n)/log(2)",  color="#808080")
    #print(datapoints)
    #print(y)

    plt.xscale("log",base=2)       
    #tickslist=[2**i for i in range(21,28)]
    #tickslables=[i for i in range(21,28)]
    #tickslables=[r'$2^{'+str(i)+r'}$' for i in range(21,28)]
    #plt.xticks(ticks=tickslist, labels=tickslables,rotation=45)
    plt.xlabel("Number of Data Points")
    plt.ylabel("Time per Query in ms")
    plt.legend()
    name="/TimePerQuery-perNumDatapoints-perNumAttributes"#-maxSelectivity:"+str(max_selectivity)+"%"
    plt.tight_layout()         
    plt.savefig(outdir+name+".png")
    plt.savefig(outdir+name+".svg")





def numDatapointToTimePerQueryComparision(datasets,naive_datasets,outdir,colors, markers,linestyles):

    plt.figure()
    #calculates the 90% percent quantil for each column
    def fci(x):
        a,b=sms.DescrStatsW(x).tconfint_mean()
        m=float(sms.DescrStatsW(x).mean)
        a_rel=abs(float(a)-m)
        b_rel=abs(float(b)-m)
        return a_rel, b_rel

    #pQ=True
    #pQ=False
    qF=0#->"SUM"
    #nQ=100

    dbFormats=list()

    for d in datasets:
        thisFormat=d["COLUMN_FORMAT"]
        if thisFormat not in dbFormats:
            dbFormats.append(thisFormat)

    i=0
    #max_selectivity=5 #%
    for thisFormat in dbFormats:
        numAttributes=0
        data=list()

        for d in datasets: 
            if d["COLUMN_FORMAT"]==thisFormat and int(d["QUERY_FUNCTION"])==qF:
                numAttributes=int(d["NUM_ATTRIBUTES"])
                queries=list()
                for t in d["QUERIES"]:
                    tnew=dict()
                    tnew["numAttributes"]=int(d["NUM_ATTRIBUTES"])
                    tnew["numDatapoints"]=int(d["NUM_DATAPOINTS"])
                    tnew["selectivity"]=int(t["real"])/tnew["numDatapoints"]*100 # to get %
                    tnew["overhead"]=int(t["overhead"])/1000000 #to get s
                    queries.append(tnew)
                data=data+queries
        df=pd.DataFrame(data)
        df_mean=df.groupby(["numDatapoints"]).mean().reset_index()
        df_median=df.groupby(["numDatapoints"]).median().reset_index()
        df_count=df.groupby(["numDatapoints"]).count().reset_index()

        print(df_median)
        print(df_count)

        df_ci=pd.DataFrame(df.groupby(["numDatapoints"])["overhead"].apply(fci).tolist(), columns=["upper","lower"])
        ci_tuples=[df_ci["lower"].tolist(), df_ci["upper"].tolist()]
        thislabel=str(numAttributes)+" columns(s)"
        plt.errorbar(df_mean["numDatapoints"],df_mean["overhead"],yerr=ci_tuples, capsize=5, linestyle=linestyles[i%len(linestyles)], marker=markers[i%len(markers)], label=thislabel,color=colors[i%len(colors)])
        i=i+1


    for thisFormat in dbFormats:
        numAttributes=0
        data=list()

        for d in naive_datasets: 
            if d["COLUMN_FORMAT"]==thisFormat and int(d["QUERY_FUNCTION"])==qF:
                numAttributes=int(d["NUM_ATTRIBUTES"])
                queries=list()
                for t in d["QUERIES"]:
                    tnew=dict()
                    tnew["numAttributes"]=int(d["NUM_ATTRIBUTES"])
                    tnew["numDatapoints"]=int(d["NUM_DATAPOINTS"])
                    tnew["selectivity"]=int(t["real"])/tnew["numDatapoints"]*100 # to get %
                    tnew["overhead"]=int(t["overhead"])/1000000 #to get s
                    tnew['oram']=False
                    queries.append(tnew)
                data=data+queries
        df=pd.DataFrame(data)
        df_mean=df.groupby(["numDatapoints"]).mean().reset_index()
        df_median=df.groupby(["numDatapoints"]).median().reset_index()
        df_count=df.groupby(["numDatapoints"]).count().reset_index()

        df_ci=pd.DataFrame(df.groupby(["numDatapoints"])["overhead"].apply(fci).tolist(), columns=["upper","lower"])
        ci_tuples=[df_ci["lower"].tolist(), df_ci["upper"].tolist()]
        thislabel="Naive, "+str(numAttributes)+" columns(s)"
        plt.errorbar(df_mean["numDatapoints"],df_mean["overhead"],yerr=ci_tuples, capsize=5, linestyle=linestyles[i%len(linestyles)], marker=markers[i%len(markers)], label=thislabel,color=colors[i%len(colors)])
        i=i+1   

    plt.xscale("log",base=2)       
    #tickslist=[2**i for i in range(16,20)]
    #tickslables=[r'$2^{'+str(i)+r'}$' for i in range(16,20)]
    #plt.xticks(ticks=tickslist, labels=tickslables)
    plt.xlabel("Number of Data Points")
    plt.ylabel("Time per Query in ms")
    plt.legend()
    name="/TimePerQuery-withNaive-perNumDatapoints-perNumAttributes"#-maxSelectivity:"+str(max_selectivity)+"%"
    plt.tight_layout()         
    plt.savefig(outdir+name+".png")
    plt.savefig(outdir+name+".svg")