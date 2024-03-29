import matplotlib.pyplot as plt
import pandas as pd
from math import *
import numpy as np
import statsmodels.stats.api as sms
from src.util import *

def numDatapointsToDeletionTime(datasets,outdir,colors, markers,linestyles):
    plt.figure()
    #insertion time
    dbFormats=list()


    i=0
    all_data=list()
    for d in datasets: 
        if bool(d["DELETION"])==True:
            numAttributes=int(d["NUM_ATTRIBUTES"])
            n=int(d["NUM_DATAPOINTS"])               
            deletetionList=d["DeletionTimes"].split(",")
            for index,val in enumerate(deletetionList):
                if index==0:
                    #drop first measurments as it sometimes takes longer than all others due to caching effects
                    continue
                dt=int(val)/1000#1000000 data is in microseconds and we convert to microseconds
                n=int(d["NUM_DATAPOINTS"])               
                all_data.append([n, numAttributes, dt])
    df=pd.DataFrame(all_data, columns=["n",'nA','deletionTime'])
    df=df[df['n']>1000]

    nAList=df['nA'].unique()

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
        
        plt.errorbar(x,y,yerr=yerr, linestyle=linestyles[i%len(linestyles)], marker=markers[i%len(markers)],markersize=5, label=thislabel,color=colors[i%len(colors)])
        i=i+1
        
    tickslist=[2**i for i in range(16,21)]
    tickslables=[r'$2^{'+str(i)+r'}$' for i in [16]]
    tickslables+=[r'  $2^{'+str(i)+r'}$' for i in [17]]
    tickslables+=[r'$2^{'+str(i)+r'}$' for i in range(18,21)]
    plt.xticks(ticks=tickslist, labels=tickslables)
    plt.xlabel("i-th Data Point")
    plt.ylabel("Deletion Duration in ms")
    #plt.legend(loc="upper right", bbox_to_anchor=(1, 0.9))
    plt.tight_layout()
    plt.xscale("log",base=2)       

    plt.savefig(outdir+"/DeletionTimes.svg")
    plt.savefig(outdir+"/DeletionTimes.png")
    
    nMax=df['n'].max()
    df_comp=df[df['n']==nMax]
    df_comp=df_comp[['nA','deletionTime']]
    df_mcomp= df_comp.groupby('nA').median()
    factor=df_mcomp.iloc[0][0]
    print(df_mcomp['deletionTime']/factor)
    

