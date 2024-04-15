
import matplotlib.pyplot as plt
import pandas as pd
from math import *
import numpy as np
import statsmodels.stats.api as sms
from src.util import *

def numDatapointsToInsertionTime(datasets,outdir,colors, markers,linestyles):
    fig=plt.figure()
    #insertion time
    legend=True

    all_data=list()
    for d in datasets: 
        numAttributes=int(d["NUM_ATTRIBUTES"])
        insertionList=d["InsetionTimes"].split(",")
        for i, insertVal in enumerate(insertionList):
            #if i==0:
                #drop first measurments as it sometimes takes longer than all others due to caching effects
                #continue
            insertT=int(insertVal)/1000 #insertion measurments are collected in microseconds
            n=int(d["NUM_DATAPOINTS"])               
            all_data.append([n, numAttributes, insertT])       
    df=pd.DataFrame(all_data, columns=["n",'nA','insertionTime'])
    df=df[df['n']>1000]

    nAList=df['nA'].unique()
    print(nAList)
    i=0
    for numAttributes in sorted(nAList,reverse=True):
        df_sel=df[df['nA']==numAttributes]

        def fci(x):
            a,b=sms.DescrStatsW(x).tconfint_mean()
            mean=float(sms.DescrStatsW(x).mean)
            a_rel=abs(float(a)-mean)
            b_rel=abs(float(b)-mean)
            return mean,a_rel,b_rel
        
        df_ci=df_sel.groupby(['n','nA']).apply(fci)
        df_ci=df_ci.reset_index(name="ci")
        df_ci[['mean','lower','upper']] = pd.DataFrame(df_ci['ci'].tolist(), index=df_ci.index)

        yerr=[df_ci['lower'].tolist(),df_ci['upper'].tolist()]
        x=df_ci['n'].tolist()
        y=df_ci['mean'].tolist()

        thislabel=str(numAttributes)+" "
        if(numAttributes==1):
            thislabel+="column"
        else:
            thislabel+="columns"

        if legend:
            plt.errorbar(x,y,yerr=yerr, linestyle=linestyles[i%len(linestyles)],capsize=5, marker=markers[i%len(markers)],markersize=5, label=thislabel,color=colors[i%len(colors)])
        else:
            plt.errorbar(x,y,yerr=yerr, linestyle=linestyles[i%len(linestyles)], capsize=5,marker=markers[i%len(markers)],markersize=5,color=colors[i%len(colors)])
        i=i+1
        
    tickslist=[2**i for i in range(16,21)]

    tickslables=[r'$2^{'+str(i)+r'}$' for i in [16]]
    tickslables+=[r'  $2^{'+str(i)+r'}$' for i in [17]]
    tickslables+=[r'$2^{'+str(i)+r'}$' for i in range(18,21)]

    plt.xticks(ticks=tickslist, labels=tickslables)
    plt.xscale("log",base=2)       
    plt.xlabel("i-th Data Point")
    plt.ylabel("Insert Duration in ms")
    if legend:
        plt.legend(loc="upper right", bbox_to_anchor=(1, 0.9))
    plt.tight_layout()

    plt.savefig(outdir+"/InsertionTimes.svg")
    plt.savefig(outdir+"/InsertionTimes.png")
    
    nMax=df['n'].max()
    df_comp=df[df['n']==nMax]
    df_comp=df_comp[['nA','insertionTime']]
    df_mcomp= df_comp.groupby('nA').median()
    factor=df_mcomp.iloc[0][0]
    print("Factor compared to 1 column ")
    print(df_mcomp['insertionTime']/factor)
    
    if not legend:
        produceLegend(df,outdir,colors, markers,linestyles)



def produceLegend(df,outdir,colors, markers,linestyles):
    fig=plt.figure()

    i=4
    nAList=df['nA'].unique()

    for numAttributes in sorted(nAList,reverse=False):
        df_sel=df[df['nA']==numAttributes]

        def fci(x):
            a,b=sms.DescrStatsW(x).tconfint_mean()
            mean=float(sms.DescrStatsW(x).mean)
            a_rel=abs(float(a)-mean)
            b_rel=abs(float(b)-mean)
            return mean,a_rel,b_rel
        
        df_ci=df_sel.groupby(['n','nA']).apply(fci)
        df_ci=df_ci.reset_index(name="ci")
        df_ci[['mean','lower','upper']] = pd.DataFrame(df_ci['ci'].tolist(), index=df_ci.index)

        yerr=[df_ci['lower'].tolist(),df_ci['upper'].tolist()]
        x=df_ci['n'].tolist()
        y=df_ci['mean'].tolist()

        thislabel=str(numAttributes)+" "
        if(numAttributes==1):
            thislabel+="column"
        else:
            thislabel+="columns"

        plt.errorbar(x,y,yerr=yerr, linestyle=linestyles[i%len(linestyles)], marker=markers[i%len(markers)],markersize=5, label=thislabel,color=colors[i%len(colors)])
        i=i-1
        
    plt.tight_layout()
    export_legend(fig.axes[0])

    

def numDatapointsToInsertionTime_old(datasets,outdir,colors, markers,linestyles):
    plt.figure()
    #insertion time
    dbFormats=list()

    for d in datasets:
        thisFormat=d["COLUMN_FORMAT"]
        if thisFormat not in dbFormats:
            dbFormats.append(thisFormat)
    log_max=16
    offset=2
    n=2**log_max-offset


    selection=np.logspace(2, log_max,num=100,endpoint=True, base=2, dtype=int)
    selection=selection-offset
    selection=selection.tolist()
    selection=list(set(selection))
    selection=sorted(selection)
    #print(selection)
    comparison=list()
    #data_for_later=list()

    i=0
    legendLabels=list()
    for thisFormat in dbFormats:
        numAttributes=0;
        data=list()
        all_data=list()
        for d in datasets: 
            if d["COLUMN_FORMAT"]==thisFormat and int(d["NUM_DATAPOINTS"])==n:
                numAttributes=int(d["NUM_ATTRIBUTES"])
                insertT=d["InsetionTimes"]
                split=insertT.split(",")
                l=[int(x) for x in split]
                comparison.append([numAttributes,l[-1]])
                l_small=[l[i-1] for i in selection]
                data.append(l_small)
                all_data.append(l)
        df=pd.DataFrame(data)/1000000
        df_mean=df.mean(axis=0)
        #print(df)
        #print(df_mean)

        def fci(x):
            a,b=sms.DescrStatsW(x).tconfint_mean()
            a_rel=abs(float(a)-float(sms.DescrStatsW(x).mean))
            b_rel=abs(float(b)-float(sms.DescrStatsW(x).mean))
            return (a_rel,b_rel)
        
        df_ci=df.apply(fci, axis=0)
        #print(df_ci)
        ci_tuples=list(df_ci.itertuples(index=False, name=None))
        x=selection
        thislabel=str(numAttributes)+" columns(s)"
        legendLabels.append(thislabel)
        #print(str(i)+" "+thislabel)
        
        plt.errorbar(x,df_mean,yerr=ci_tuples, linestyle=linestyles[i%len(linestyles)], marker=markers[i%len(markers)],markersize=5, color=colors[i%len(colors)])
        i=i+1
        
    tickslist=[2**i for i in range(13,17)]
    tickslables=[r'$2^{'+str(i)+r'}$' for i in range(13,17)]
    plt.xticks(ticks=tickslist, labels=tickslables)
    plt.xlabel("i-th Datapoint")
    plt.ylabel("Insert Duration in ms")
    legend= plt.legend(loc="upper right", bbox_to_anchor=(1, 0.9))
    plt.tight_layout()

    #plt.gcf().subplots_adjust(bottom=0.15)
    plt.savefig(outdir+"/InsertionTimes-old.svg")
    plt.savefig(outdir+"/InsertionTimes-old.png")
        
    df_comp=pd.DataFrame.from_records(comparison, columns=['numAttributes','insertionLast'])
    df_mcomp= df_comp.groupby('numAttributes').median()
    factor=df_mcomp.iloc[0][0]
    #print(df_mcomp['insertionLast']/factor)
    
