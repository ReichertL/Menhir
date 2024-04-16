

import matplotlib.pyplot as plt
import pandas as pd
from math import *
import numpy as np
import statsmodels.stats.api as sms
from src.util import *




def retrievedNumDatapointsToTimePerQuery(onlySelected,datasets,outdir,colors, markers,linestyles):


    #pQ=True
    #pQ=False
    qF=0#->"SUM"
    #nQ=100

    dbFormats=list()

    for d in datasets:
        thisFormat=d["COLUMN_FORMAT"]
        if thisFormat not in dbFormats:
            dbFormats.append(thisFormat)

    df=pd.DataFrame()
    for thisFormat in dbFormats:
        numAttributes=0
        data=list()
        for d in datasets: 
            if d["COLUMN_FORMAT"]==thisFormat :
                numAttributes=int(d["NUM_ATTRIBUTES"])
                queries=list()
                for t in d["QUERIES"]:
                    tnew=dict()
                    tnew["log_capacity"]=ceil(log(int((d["NUM_DATAPOINTS"])))/log(2))
                    tnew["numAttributes"]=int(d["NUM_ATTRIBUTES"])
                    tnew["total"]=int(t["total"])
                    try:
                        tnew["numOsms"]=int(d["NUM_OSMs"])
                    except:
                        tnew["numOsms"]=1
                    tnew["retrieveExactly"]=int(t["total"])
                    tnew["overhead"]=int(t["overhead"])/1000000 #to get s
                    if(tnew["retrieveExactly"]>1):
                        queries.append(tnew)
                data=data+queries
        df2=pd.DataFrame(data)
        df = pd.concat([df, df2], ignore_index=True, sort=False)      

    numAttributes=df['numAttributes'].unique().tolist()
    log_capacitys=df['log_capacity'].unique().tolist()
    re=df['retrieveExactly'].sort_values().unique().tolist()
    total=df['total'].sort_values().unique().tolist()
    numOsms=df['numOsms'].sort_values().unique().tolist()

    test_totalSize_log=list()
    test_runtime_2_16=list()

    logCaps=list()
    if onlySelected:
        logCaps=[24,16]
    else: 
        logCaps= log_capacitys

    selectedAttributes=[5,1] #sorted(numAttributes)
    i=len(selectedAttributes)*len(logCaps)-1
    for logCap in sorted(logCaps,reverse=True):
        for numA in selectedAttributes:
            df_this=df[df['numAttributes']==numA]
            df_this=df_this[df_this['log_capacity']==logCap]
            df_mean=df_this.groupby(["retrieveExactly"]).mean().reset_index()
            df_median=df_this.groupby(["retrieveExactly"]).median().reset_index()
            df_count=df_this.groupby(["retrieveExactly"]).count().reset_index()
            df_min=df_this.groupby(["retrieveExactly"]).min().reset_index()

            def fci(x):
                a,b=sms.DescrStatsW(x).tconfint_mean()
                m=float(sms.DescrStatsW(x).mean)
                a_rel=abs(float(a)-m)
                b_rel=abs(float(b)-m)
                return a_rel, b_rel
            
            df_ci=pd.DataFrame(df_this.groupby(["retrieveExactly"])["overhead"].apply(fci).tolist(), columns=["upper","lower"])

            ci_tuples=[df_ci["lower"].tolist(), df_ci["upper"].tolist()]

            thislabel="n="+r'$2^{'+str(logCap)+r'}$'+", "
            if(numA==1):
                thislabel+=str(numA)+" column"
            else:
                thislabel+=str(numA)+" columns"

            plt.plot(df_min["retrieveExactly"],df_min["overhead"], linestyle=linestyles[i%len(linestyles)], marker=markers[i%len(markers)],color='#BBBBBB')#colors[i%len(colors)])

            plt.errorbar(df_mean["retrieveExactly"],df_mean["overhead"],yerr=ci_tuples, capsize=5, linestyle=linestyles[i%len(linestyles)], marker=markers[i%len(markers)], label=thislabel,color=colors[i%len(colors)])
                              
            print(thislabel)
            print(df_mean[["retrieveExactly","overhead"]])


            
            try: 
                print(thislabel)
                res=np.polyfit(df_mean["retrieveExactly"],df_mean["overhead"], 1)
                print("y="+str(res[0])+"*x+"+str(res[1]))
                y=10000 #10s
                print("for y="+str(y)+": "+str(y/res[0]+res[1]/res[0])+"")
                x=pow(2,16)
                y=res[0]*x+res[1]
                print("for x="+str(x)+": "+str(y)+"\n ")
                if(numA==1):
                    test_runtime_2_16.append(y/1000) # in s
                    test_totalSize_log.append(2**logCap)
            except:
                pass

            i=i-1
            print("-----------")


    #    
    #tickslist=[2**i for i in range(16,20)]
    #tickslables=[r'$2^{'+str(i)+r'}$' for i in range(16,20)]
    #plt.xticks(ticks=tickslist, labels=tickslables)
    plt.xlabel("Number of Retrieved Data Points")
    plt.ylabel("Time per Query in ms")
    plt.legend()
    name="/TimePerQuery-perRetrievedNumDatapoints-perNumAttributes"
    plt.tight_layout()         
    plt.savefig(outdir+name+".png")
    plt.savefig(outdir+name+".svg")


    try: 
        plt.figure()
        res=np.polyfit(test_totalSize_log,test_runtime_2_16, 1)
        xlist=[2**i for i in range(16,25)]
        ylist=[res[0]*x+res[1] for x in xlist]
        plt.plot(xlist,ylist)
        plt.xscale("log",base=2)   
        name="/Interpolation-runtimeForRetrieving2^16-differentNumDatapoints"
        plt.xlabel("Total Number of Datapoints")
        plt.ylabel("Runtime for 2^16 in s")
        plt.tight_layout()         
        plt.savefig(outdir+name+".png")
    except:
        print("Error during interpolation")
        pass


def retrievedNumDatapointsToTimePerQueryToValueSize(datasets,outdir,colors, markers,linestyles):


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
    df=pd.DataFrame()
    for thisFormat in dbFormats:
        numAttributes=0;
        data=list()
        for d in datasets: 
            if d["COLUMN_FORMAT"]==thisFormat :
                numAttributes=int(d["NUM_ATTRIBUTES"])
                queries=list()
                for index,t in enumerate(d["QUERIES"]):
                    if index==0:
                        #drop first measurments as it sometimes takes longer than all others due to caching effects
                        continue
                    tnew=dict()
                    tnew["valueSize"]=int(d["VALUE_SIZE"])
                    tnew["numAttributes"]=int(d["NUM_ATTRIBUTES"])
                    tnew["total"]=int(t["total"])
                    tnew["numOsms"]=int(d["NUM_OSMs"])
                    tnew["retrieveExactly"]=int(t["total"])
                    tnew["overhead"]=int(t["overhead"])/1000000 #to get ms
                    if(tnew["retrieveExactly"]>1):
                        queries.append(tnew)
                data=data+queries
        df2=pd.DataFrame(data)
        df = pd.concat([df, df2], ignore_index=True, sort=False)      

    numAttributes=df['numAttributes'].unique().tolist()
    valueSizes=df['valueSize'].unique().tolist()
    re=df['retrieveExactly'].sort_values().unique().tolist()
    total=df['total'].sort_values().unique().tolist()
    numOsms=df['numOsms'].sort_values().unique().tolist()

    test_totalSize_log=list()
    test_runtime_2_16=list()

    for valsize in sorted(valueSizes,reverse=True):#[16,24]:
        for numA in [1]:#sorted(numAttributes,reverse=False):
            df_this=df[df['numAttributes']==numA]
            df_this=df_this[df_this['valueSize']==valsize]
            df_mean=df_this.groupby(["retrieveExactly"]).mean().reset_index()
            df_median=df_this.groupby(["retrieveExactly"]).median().reset_index()
            df_count=df_this.groupby(["retrieveExactly"]).count().reset_index()

            def fci(x):
                a,b=sms.DescrStatsW(x).tconfint_mean()
                m=float(sms.DescrStatsW(x).mean)
                a_rel=abs(float(a)-m)
                b_rel=abs(float(b)-m)
                return a_rel, b_rel
            
            df_ci=pd.DataFrame(df_this.groupby(["retrieveExactly"])["overhead"].apply(fci).tolist(), columns=["upper","lower"])

            ci_tuples=[df_ci["lower"].tolist(), df_ci["upper"].tolist()]

            thislabel=str(valsize)+" bytes"

            plt.errorbar(df_mean["retrieveExactly"],df_mean["overhead"],yerr=ci_tuples, capsize=5, linestyle=linestyles[i%len(linestyles)], marker=markers[i%len(markers)], label=thislabel,color=colors[i%len(colors)])
            
            try: 
                print(thislabel)
                res=np.polyfit(df_mean["retrieveExactly"],df_mean["overhead"], 1)
                print("y="+str(res[0])+"*x+"+str(res[1]))
                y=10000 #10s
                print("for y="+str(y)+": "+str(y/res[0]+res[1]/res[0])+"")
                x=pow(2,16)
                y=res[0]*x+res[1]
                print("for x="+str(x)+": "+str(y)+"\n ")
                if(numA==1):
                    test_runtime_2_16.append(y/1000) # in s
                    test_totalSize_log.append(2**logCap)
            except:
                pass

            i=i+1


    #    
    #tickslist=[2**i for i in range(16,20)]
    #tickslables=[r'$2^{'+str(i)+r'}$' for i in range(16,20)]
    #plt.xticks(ticks=tickslist, labels=tickslables)
    plt.xlabel("Number of Retrieved Data Points")
    plt.ylabel("Time per Query in ms")
    plt.legend()
    name="/ValueSize-TimePerQuery-perRetrievedNumDatapoints"
    plt.tight_layout()         
    plt.savefig(outdir+name+".png")
    plt.savefig(outdir+name+".svg")





def retrievedNumDatapointsToTimePerQueryComparision(datasets,naive,outdir,colors, markers,linestyles):

    qF=0#->"SUM"
    dbFormats=list()

    for d in datasets:
        thisFormat=d["COLUMN_FORMAT"]
        if thisFormat not in dbFormats:
            dbFormats.append(thisFormat)

    i=0
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
                for t in d["QUERIES"]:
                    tnew=dict()
                    tnew["numAttributes"]=int(d["NUM_ATTRIBUTES"])
                    tnew["retrieveExactly"]=int(d["RETRIEVE_EXACTLY"])
                    #tnew["selectivity"]=int(t["real"])/tnew["numDatapoints"]*100 # to get %
                    tnew["overhead"]=int(t["overhead"])/1000000 #to get ms
                    queries.append(tnew)
                data=data+queries
        df=pd.DataFrame(data)
        #df=df[df["selectivity"]>0]
        #df=df[df["selectivity"]<=5]

        df_mean=df.groupby(["retrieveExactly"]).mean().reset_index()
        df_median=df.groupby(["retrieveExactly"]).median().reset_index()
        df_count=df.groupby(["retrieveExactly"]).count().reset_index()

        #print(df_median)
        #print(df_count)

        def fci(x):
            a,b=sms.DescrStatsW(x).tconfint_mean()
            m=float(sms.DescrStatsW(x).mean)
            a_rel=abs(float(a)-m)
            b_rel=abs(float(b)-m)
            return a_rel, b_rel
        
        df_ci=pd.DataFrame(df.groupby(["retrieveExactly"])["overhead"].apply(fci).tolist(), columns=["upper","lower"])

        ci_tuples=[df_ci["lower"].tolist(), df_ci["upper"].tolist()]

        thislabel=str(numAttributes)+" columns(s)"

        plt.errorbar(df_mean["retrieveExactly"],df_mean["overhead"],yerr=ci_tuples, capsize=5, linestyle=linestyles[i%len(linestyles)], marker=markers[i%len(markers)], label=thislabel,color=colors[i%len(colors)])
        i=i+1

    #
    print("naive")

    for thisFormat in dbFormats:
        numAttributes=0;
        data=list()
        #isPQ=True
        #if d["POINT_QUERIES"] == "false":
        #    isPQ=False
        for d in naive: 
            if d["COLUMN_FORMAT"]==thisFormat and int(d["QUERY_FUNCTION"])==qF:
                numAttributes=int(d["NUM_ATTRIBUTES"])
                queries=list()
                for t in d["QUERIES"]:
                    tnew=dict()
                    tnew["numAttributes"]=int(d["NUM_ATTRIBUTES"])
                    tnew["retrieveExactly"]=int(d["RETRIEVE_EXACTLY"])
                    #tnew["selectivity"]=int(t["real"])/tnew["numDatapoints"]*100 # to get %
                    tnew["overhead"]=int(t["overhead"])/1000000 #to get ms
                    queries.append(tnew)
                data=data+queries
        df=pd.DataFrame(data)
        #df=df[df["selectivity"]>0]
        #df=df[df["selectivity"]<=5]

        df_mean=df.groupby(["retrieveExactly"]).mean().reset_index()
        df_median=df.groupby(["retrieveExactly"]).median().reset_index()
        df_count=df.groupby(["retrieveExactly"]).count().reset_index()

        print(numAttributes)

        def fci(x):
            a,b=sms.DescrStatsW(x).tconfint_mean()
            m=float(sms.DescrStatsW(x).mean)
            a_rel=abs(float(a)-m)
            b_rel=abs(float(b)-m)
            return a_rel, b_rel
        
        df_ci=pd.DataFrame(df.groupby(["retrieveExactly"])["overhead"].apply(fci).tolist(), columns=["upper","lower"])

        ci_tuples=[df_ci["lower"].tolist(), df_ci["upper"].tolist()]

        thislabel="Naive:"+str(numAttributes)+" columns(s)"

        print("x vals")
        print(df_mean["retrieveExactly"])
        print("y vals")
        print(df_mean["overhead"])
        print("error:")
        print(ci_tuples)
        plt.errorbar(df_mean["retrieveExactly"],df_mean["overhead"],yerr=ci_tuples, capsize=5, linestyle=linestyles[i%len(linestyles)], marker=markers[i%len(markers)], label=thislabel,color=colors[i%len(colors)])
        i=i+1


    
    #plt.xscale("log",base=2)       
    #tickslist=[2**i for i in range(16,20)]
    #tickslables=[r'$2^{'+str(i)+r'}$' for i in range(16,20)]
    #plt.xticks(ticks=tickslist, labels=tickslables)
    plt.xlabel("Number of Retrieved Data Points")
    plt.ylabel("Time per Query in ms")
    plt.legend()
    name="/Comparision-TimePerQuery-perRetrievedNumDatapoints-perNumAttributes"#-maxSelectivity:"+str(max_selectivity)+"%"
    plt.tight_layout()         
    plt.savefig(outdir+name+".png")
    plt.savefig(outdir+name+".svg")

