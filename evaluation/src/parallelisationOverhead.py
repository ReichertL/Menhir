

import matplotlib.pyplot as plt
import pandas as pd
from math import *
import numpy as np
import statsmodels.stats.api as sms
from src.util import *




def numDatapointsToORAMRuntimeCompareORAMLogCapacities(datasets,outdir,colors, markers,linestyles):
    plt.figure()
    dbFormats=list()

    for d in datasets:
        thisFormat=d["COLUMN_FORMAT"]
        if thisFormat not in dbFormats:
            dbFormats.append(thisFormat)
    
    i=0
    df=pd.DataFrame()
    for thisFormat in dbFormats:
        numAttributes=0;
        data=list()
        for d in datasets: 
            if d["COLUMN_FORMAT"]==thisFormat :
                numAttributes=int(d["NUM_ATTRIBUTES"])
                queries=list()
                for t in d["QUERIES"]:
                    tnew=dict()
                    tnew["log_datapoints"]=ceil(log(int((d["NUM_DATAPOINTS"])))/log(2))
                    tnew["numAttributes"]=int(d["NUM_ATTRIBUTES"])
                    tnew["total"]=int(t["total"])
                    tnew["oramlogcap"]=int(d["ORAM_LOG_CAPACITY"])
                    tnew["retrieveExactly"]=int(t["total"])
                    tnew["overhead"]=int(t["orams"])/1000000 #to get s
                    if(tnew["retrieveExactly"]>1):
                        queries.append(tnew)
                data=data+queries
        df2=pd.DataFrame(data)
        df = pd.concat([df, df2], ignore_index=True, sort=False)      

    numAttributes=df['numAttributes'].unique().tolist()
    oramlogcaps=df['oramlogcap'].sort_values().unique().tolist()
    print(oramlogcaps)
    retrieveExactly=60
    df=df[df['retrieveExactly']==retrieveExactly]

    for oramlogcap in sorted(oramlogcaps,reverse=False):
        for numA in sorted(numAttributes,reverse=True):
            df_this=df[df['numAttributes']==numA]
            df_this=df_this[df_this['oramlogcap']==oramlogcap]

            df_mean=df_this.groupby(["log_datapoints"]).mean().reset_index()
            df_median=df_this.groupby(["log_datapoints"]).median().reset_index()
            df_count=df_this.groupby(["log_datapoints"]).count().reset_index()

            def fci(x):
                a,b=sms.DescrStatsW(x).tconfint_mean()
                m=float(sms.DescrStatsW(x).mean)
                a_rel=abs(float(a)-m)
                b_rel=abs(float(b)-m)
                return a_rel, b_rel
                
            df_ci=pd.DataFrame(df_this.groupby(["log_datapoints"])["overhead"].apply(fci).tolist(), columns=["upper","lower"])
            ci_tuples=[df_ci["lower"].tolist(), df_ci["upper"].tolist()]

            thislabel="ORAM Size:"+r'$2^{'+str(oramlogcap)+r'}$'+", "
            if(numA==1):
                thislabel+=str(numA)+" column"
            else:
                thislabel+=str(numA)+" columns"

            plt.errorbar(df_mean["log_datapoints"],df_mean["overhead"],yerr=ci_tuples, capsize=5, linestyle=linestyles[i%len(linestyles)], marker=markers[i%len(markers)], label=thislabel,color=colors[i%len(colors)])
            print(thislabel)
            print(df_mean["log_datapoints"])
            print(df_mean["overhead"])
            print(ci_tuples)
            i=i+1
            
        #plt.xscale("log",base=2)   
        log_datapoints=df['log_datapoints'].sort_values().unique().tolist()
        tickslables=[r'$2^{'+str(i)+r'}$' for i in log_datapoints]
        plt.xticks(ticks=log_datapoints, labels=tickslables)

        plt.xlabel("Number of Data Points")
        plt.ylabel("Time for Data Retrieval in ms")
        plt.legend()
        name="/TimeInOram-perNumDatapoints-perORAMlogcap-perNumAttributes"#-maxSelectivity:"+str(max_selectivity)+"%"
        plt.tight_layout()         
        plt.savefig(outdir+name+".png")
        plt.savefig(outdir+name+".svg")



def numOSMsToORAMRuntime(datasets,outdir,colors, markers,linestyles):
    plt.figure()
    dbFormats=list()

    for d in datasets:
        thisFormat=d["COLUMN_FORMAT"]
        if thisFormat not in dbFormats:
            dbFormats.append(thisFormat)

    i=0
    logCapacity=16
    df=pd.DataFrame()
    for thisFormat in dbFormats:
        numAttributes=0;
        data=list()
        for d in datasets: 
            if d["COLUMN_FORMAT"]==thisFormat and  int(d["ORAM_LOG_CAPACITY"])==logCapacity:
                numAttributes=int(d["NUM_ATTRIBUTES"])
                queries=list()
                for t in d["QUERIES"]:
                    tnew=dict()
                    tnew["log_datapoints"]=ceil(log(int((d["NUM_DATAPOINTS"])))/log(2))
                    tnew["numAttributes"]=int(d["NUM_ATTRIBUTES"])
                    tnew["total"]=int(t["total"])
                    tnew["numOsms"]=int(d["NUM_OSMs"])
                    tnew["retrieveExactly"]=int(t["total"])
                    tnew["overhead"]=int(t["orams"])/1000000 #to get s
                    if(tnew["retrieveExactly"]>1):
                        queries.append(tnew)
                data=data+queries
        df2=pd.DataFrame(data)
        df = pd.concat([df, df2], ignore_index=True, sort=False)      

    numAttributes=df['numAttributes'].unique().tolist()
    numOsms=df['numOsms'].sort_values().unique().tolist()
    retrieveExactly=60
    df=df[df['retrieveExactly']==retrieveExactly]

    for numA in sorted(numAttributes,reverse=False):
        if True: #placeholder
            df_this=df[df['numAttributes']==numA]
            df_mean=df_this.groupby(["numOsms"]).mean().reset_index()
            df_median=df_this.groupby(["numOsms"]).median().reset_index()
            df_count=df_this.groupby(["numOsms"]).count().reset_index()

            def fci(x):
                a,b=sms.DescrStatsW(x).tconfint_mean()
                m=float(sms.DescrStatsW(x).mean)
                a_rel=abs(float(a)-m)
                b_rel=abs(float(b)-m)
                return a_rel, b_rel
                
            df_ci=pd.DataFrame(df_this.groupby(["numOsms"])["overhead"].apply(fci).tolist(), columns=["upper","lower"])
            ci_tuples=[df_ci["lower"].tolist(), df_ci["upper"].tolist()]


            thislabel=str(numA)+" column"
            if(numA>1):
                thislabel=str(numA)+" columns"

            plt.errorbar(df_mean["numOsms"],df_mean["overhead"],yerr=ci_tuples, capsize=5, linestyle=linestyles[i%len(linestyles)], marker=markers[i%len(markers)], label=thislabel,color=colors[i%len(colors)])
            i=i+1

        plt.xscale("log",base=2)   
        plt.xticks(numOsms)
        plt.xlabel("Number OSMs")
        plt.ylabel("Time for Data Retrieval in ms")
        plt.legend()
        name="/TimeInOram-perNumOSMs-perNumAttributes"#-maxSelectivity:"+str(max_selectivity)+"%"
        plt.tight_layout()         
        plt.savefig(outdir+name+".png")
        plt.savefig(outdir+name+".svg")


