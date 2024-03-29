import matplotlib.pyplot as plt
import pandas as pd
from math import *
import numpy as np
import statsmodels.stats.api as sms
import seaborn as sns

from src.util import *

#ONLY RELEVANT FOR RANGE QUERIES and data tha follows NORMAL DISTRIBUTION

def selectivityToPadding_lineplotWithCI(df,datasets, outdir,colors, markers, linestyles):

    groupsize=10
    n=2**16-2
    mmax=0.1
    steps=10
    groups=np.arange(0, mmax, mmax/steps)
    print(groups)
    
    fig=plt.figure(figsize=(8, 5), dpi=80)

    df_data=pd.DataFrame( columns =['selectivity', 'overhead'])
    df_tmp=df[df['pointQuery']=='false']
    df_tmp=df_tmp[df_tmp['numDatapoints']==n]
    df_tmp=df_tmp[df_tmp['numAttributes']==1]

    selected_ds=df_tmp["RefIndex"].tolist()

    q_sel=list()
    q_noise=list()
    for ds_index in selected_ds:
        n= int(datasets[ds_index]["NUM_DATAPOINTS"])
        for q in datasets[ds_index]["QUERIES"]:

            if int(q["real"])==0:
                #catches a bug that exists in some old measurments
                pass 
            else:
                q_sel.append(float(q["real"])/n)  # selectivity
                value= int(q["padding"])
                q_noise.append(value) # in blocks
            
        df_tmp=pd.DataFrame(list(zip(q_sel, q_noise)),  columns =['selectivity', 'overhead'])
        df_data=pd.concat([df_data,df_tmp])
            
    df_data=df_data[df_data['selectivity']>0]
    df_data["selectivity"]=pd.cut(df_data["selectivity"],groups)

    to_tuples =df_data["selectivity"].sort_values().unique().dropna()
    sorted(to_tuples)
    interval_tuples=[(i.left*100,i.right*100) for i in to_tuples]
    str_index=["["+"{:.1f}".format(i)+","+"{:.1f}".format(j)+")" for (i,j) in interval_tuples]
                       
    #only consider selectivities where a useable number of datapoints exist
    check_count=df_data.groupby("selectivity").count()
    selectivites=check_count[check_count["overhead"]<groupsize].index.to_list()
    df_data = df_data[~df_data["selectivity"].isin(selectivites)]
                
    df_data=df_data.groupby("selectivity",observed=True)
    tmp=df_data.apply(mean_confidence_interval)
    print(tmp)
    x= tmp.index.tolist()
    tmp_data=tmp.tolist()
    y,err= zip(*tmp_data)
    y=list(y)
    err=list(err)
    x=range(1,len(y)+1)
    plt.errorbar(x,y,yerr=err,capsize=3,color=colors[0], linestyle=linestyles[0], marker=markers[0],markersize=5) 

    plt.xticks(ticks=range(1,len(str_index)+1), labels=str_index,rotation=45)
    plt.xlabel("Selectivity in %")
    plt.ylabel("Padding per Query")
    plt.tight_layout()
    name="/SelectivitytoPadding-lineplot-mmax:"+str(mmax)+"-min"+str(groupsize)+"perGroup"
    plt.savefig(outdir+name+".png")
    plt.savefig(outdir+name+".svg")


def selectivityToPadding_GroupedBoxPlot(df,datasets, outdir,colors):
    qFunctions=df["queryFunction"].unique().tolist()
    PQs=df["pointQuery"].unique().tolist()


    fig=plt.figure(figsize=(8, 5), dpi=80)
    df_data=pd.DataFrame( columns =['selectivity', 'overhead'])

    df_tmp=df[df['pointQuery']=='false']
    df_tmp=df_tmp[df_tmp['numAttributes']==1]
    selected_ds=df_tmp["RefIndex"].tolist()

    q_sel=list()
    q_noise=list()
        
    for ds_index in selected_ds:
        n= int(datasets[ds_index]["NUM_DATAPOINTS"])
        for q in datasets[ds_index]["QUERIES"]:
            q_sel.append(round(float(q["real"])/n,2))  # selectivity
            q_noise.append(int(q["padding"]) )
            
        df_tmp=pd.DataFrame(list(zip(q_sel, q_noise)),  columns =['selectivity', 'overhead'])
        df_data=pd.concat([df_data,df_tmp])
    
    mmax=0.1
    steps=10
    groups=np.arange(0, mmax, mmax/steps)
    df_data=df_data[df_data['selectivity']>0]
    df_data=df_data[df_data['overhead']>0]

    df_data["selectivity"]=pd.cut(df_data["selectivity"],groups)

    to_tuples =df_data["selectivity"].unique().dropna()
    interval_tuples=[(i.left*100,i.right*100) for i in to_tuples]
    str_index=["["+"{:.1f}".format(i)+","+"{:.1f}".format(j)+")" for (i,j) in interval_tuples]
    to_tuples=df_data["selectivity"].unique()
    from itertools import cycle, islice  
    my_colors = list(islice(cycle(colors), None, len(to_tuples)))
    colors=[ '#CC6677', '#332288', '#DDCC77', '#117733', '#88CCEE', '#882255', '#44AA99', '#999933', '#AA4499'] 
    customPalette = sns.set_palette(sns.color_palette(colors))

    sns.boxplot(x = df_data['selectivity'],
                    y = df_data['overhead'],
                    palette = customPalette)

    ax = plt.gca()
    #handles, labels = ax.get_legend_handles_labels()
    #ax.legend(handles=handles[0:], labels=labels[0:])
    #adjust_box_widths(fig, 0.75)
    #sns.despine(offset=11)

    plt.xticks(ticks=range(0,len(str_index)), labels=str_index,rotation=45)
    plt.xlabel("Selectivity in %")
    plt.ylabel("Padding per Query")
    plt.tight_layout()
    name="/SelectivitytoPadding-groupedBoxplot"
    plt.savefig(outdir+name+".png")
    plt.savefig(outdir+name+".svg")
