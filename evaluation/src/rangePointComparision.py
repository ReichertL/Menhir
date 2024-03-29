
import matplotlib.pyplot as plt
import pandas as pd
from math import *
import numpy as np
import statsmodels.stats.api as sms
import seaborn as sns

from src.util import *


def overheadTotal_PointQueryVsRangeQuery_barplotWithCI(df,datasets, outdir,colors, markers, linestyles):
    pointQuery=df["pointQuery"].unique().tolist()
    n_values=df["numDatapoints"].unique().tolist()

    dbFormats=df["columnFormat"].unique().tolist()
    #n=2**16-2
    groupsize=10
    #qf=qFunctions[0]
    nA=5 #only allow 5 attributes

    mmax=0.1
    steps=10
    groups=np.arange(0, mmax, mmax/steps)
    print(groups)

    
    
    for n in n_values :
        fig=plt.figure(figsize=(8, 5), dpi=80)

        for pQ in pointQuery:
            df_data=pd.DataFrame( columns =['selectivity', 'overhead',"numAttributes"])
            for thisFormat in dbFormats:         
                df_tmp=df[df['pointQuery']==pQ]
                df_tmp=df_tmp[df_tmp['numDatapoints']==n]
                df_tmp=df_tmp[df_tmp['columnFormat']==thisFormat]

                selected_ds=df_tmp["RefIndex"].tolist()

                q_sel=list()
                q_noise=list()
                for ds_index in selected_ds:
                    n= int(datasets[ds_index]["NUM_DATAPOINTS"])
                    for q in datasets[ds_index]["QUERIES"]:

                        q_sel.append(float(q["real"])/n)  # selectivity
                        value= int(q["total"])-int(q['real'])  #Blocks that were either due to padding or due to noise
                        q_noise.append(value) # in blocks
            
                nA=len(thisFormat.split(","))
                df_tmp=pd.DataFrame(list(zip(q_sel, q_noise)),  columns =['selectivity', 'overhead'])
                df_tmp["numAttributes"]=nA
                df_data=pd.concat([df_data,df_tmp])
            
            print(df_data)
            df_data=df_data[df_data['selectivity']>0]
            print(df_data)
          
                
            
            line_data=line_data.groupby("selectivity",observed=True)
            tmp=line_data.apply(mean_confidence_interval)

            labelString=str(nA)+ " column(s), "+queryString
            plt.errorbar(x,y,yerr=err,capsize=3, label=labelString,color=colors[i%len(colors)], linestyle=linestyles[i%len(linestyles)], marker=markers[i%len(markers)],markersize=5) 

        plt.legend()
        plt.xticks(ticks=range(1,len(str_index)+1), labels=str_index,rotation=45)
        plt.xlabel("Selectivity in %")
        plt.ylabel("No. Additional Entries")
        plt.tight_layout()
        name="/comparisonRangeToPoint-overheadInBlocks-toQuerySelectivity-perNumAttributes-n:"+str(n)+"-mmax:"+str(mmax)+"-min"+str(groupsize)+"perGroup"
        plt.savefig(outdir+name+".png")
        plt.savefig(outdir+name+".svg")




def selectivityToOverhead_PointQueryVsRangeQuery_GroupedBarPlot_withBoxPlot(df,datasets, outdir,colors):
    qFunctions=df["queryFunction"].unique().tolist()
    PQs=df["pointQuery"].unique().tolist()
    mmax=0.1

    for qf in qFunctions :
        fig=plt.figure(figsize=(8, 5), dpi=80)
        df_data=pd.DataFrame( columns =['selectivity', 'overhead','PQ'])

        for thisPQ in PQs:         
            df_tmp=df[df['queryFunction']==qf]
            df_tmp=df_tmp[df_tmp['pointQuery']==thisPQ]
            df_tmp=df_tmp[df_tmp['numAttributes']==1]

            selected_ds=df_tmp["RefIndex"].tolist()

            q_sel=list()
            q_noise=list()
        
            for ds_index in selected_ds:
                n= int(datasets[ds_index]["NUM_DATAPOINTS"])
                for q in datasets[ds_index]["QUERIES"]:
                    if int(q["real"])>0:
                        q_sel.append(round(float(q["real"])/n,2))  # selectivity
                        value=(int(q["total"])-int(q["real"]))/int(q["real"])  #int(q["noise"])  #(int(q["total"])-int(q["real"])) #/int(q["real"]) #float(q["padding"])+float(q["noise"]))/float(q["total"]) # this is a relativ percentage (of the number of retrieved datapoints) not a runtime
                        q_noise.append(value) #overhead in s
            
            df_tmp=pd.DataFrame(list(zip(q_sel, q_noise)),  columns =['selectivity', 'overhead'])
            df_tmp["PQ"]= True if thisPQ=="true" else False
            df_data=pd.concat([df_data,df_tmp])

        steps=10
        groups=np.arange(0, mmax, mmax/steps)
        df_data=df_data[df_data['selectivity']>0]
        df_data=df_data[df_data['overhead']>0]

        df_data["selectivity"]=pd.cut(df_data["selectivity"],groups)

        to_tuples =df_data["selectivity"].unique().dropna()
        interval_tuples=[(i.left*100,i.right*100) for i in to_tuples]
        str_index=["["+"{:.1f}".format(i)+","+"{:.1f}".format(j)+")" for (i,j) in interval_tuples]
        to_tuples=df_data["selectivity"].unique()
        customPalette = sns.set_palette(sns.color_palette(colors[2:]))
        df_data=df_data.sort_values(by=['PQ'])
        print(df_data)
        df_data["PQ_label"]=df_data.apply (lambda row: "point query" if row['PQ'] else "range query", axis=1)
        print(df_data)
        sns.boxplot(x = df_data['selectivity'],
                    y = df_data['overhead'],
                    hue = df_data['PQ_label'],
                    palette = customPalette)

        ax = plt.gca()
        handles, labels = ax.get_legend_handles_labels()
        ax.legend(handles=handles[0:], labels=labels[0:])
        adjust_box_widths(fig, 0.75)
        sns.despine(offset=11)

        plt.xticks(ticks=range(0,len(str_index)), labels=str_index,rotation=45)
        plt.xlabel("Selectivity in %")
        plt.ylabel("Noise + Padding in %")
        plt.tight_layout()
        name="/Overhead-perQuerySelectivity"
        plt.savefig(outdir+name+".png")
        plt.savefig(outdir+name+".svg")


def domainSizeToNoise_RangeQuery_GroupedBarPlot_withBoxPlot(df,datasets, outdir,colors):
    qFunctions=df["queryFunction"].unique().tolist()
    PQs=df["pointQuery"].unique().tolist()
    sel=0.01

    


    for qf in qFunctions :
        fig=plt.figure(figsize=(8, 5), dpi=80)
        df_data=pd.DataFrame( columns =['selectivity', 'overhead', 'domainSize', 'PQ'])

        for thisPQ in PQs:         
            df_tmp=df[df['queryFunction']==qf]
            df_tmp=df_tmp[df_tmp['pointQuery']==thisPQ]
            df_tmp=df_tmp[df_tmp['numAttributes']==1]    
            selected_ds=df_tmp["RefIndex"].tolist()

            q_sel=list()
            q_noise=list()
            q_ds=list()
        
            for ds_index in selected_ds:
                n= int(datasets[ds_index]["NUM_DATAPOINTS"])
                minV= datasets[ds_index]["MIN_VALUES"].split(',')
                maxV= datasets[ds_index]["MAX_VALUE"].split(',')
                ds=[log(int(up)-int(lo))/log(2) for lo,up in zip(minV,maxV)]
                #ds= datasets[ds_index]["DOMAIN_SIZE"].split(',')
                where_index=int(datasets[ds_index]["WHERE_INDEX"])
                this_ds=int(ds[where_index])
                #if this_ds not in selectedDomainSizes:
                #    continue

                for q in datasets[ds_index]["QUERIES"]:
                    q_ds.append(this_ds)  # domain size for queried column
                    this_sel=round(float(q["real"])/n,2)
                    q_sel.append(this_sel)  # selectivity
                    q_noise.append(int(q["noise"])) #overhead in s
            
            df_tmp=pd.DataFrame(list(zip(q_sel, q_noise,q_ds)),  columns =['selectivity', 'overhead','domainSize'])
            df_tmp["PQ"]= True if thisPQ=="true" else False
            df_data=pd.concat([df_data,df_tmp])

        #df_data=df_data[df_data["domainSize"]>=2000]
        df_data=df_data[df_data["selectivity"]==sel]
        customPalette = sns.set_palette(sns.color_palette(colors[4:]))
        df_data=df_data.sort_values(by=['PQ'])
        print(df_data)
        df_data["PQ_label"]=df_data.apply (lambda row: "point query" if row['PQ'] else "range query", axis=1)
        print(df_data)
        sns.boxplot(x = df_data['domainSize'],
                    y = df_data['overhead'],
                    hue = df_data['PQ_label'],
                    palette = customPalette)

        ax = plt.gca()
        handles, labels = ax.get_legend_handles_labels()
        ax.legend(handles=handles[0:], labels=labels[0:])
        #adjust_box_widths(fig, 0.75)
        #sns.despine(offset=11)
        
        
        tickslist= df_data['domainSize'].sort_values().unique().tolist()
        tickslables=[r'$2^{'+str(i)+r'}$' for i in tickslist]
        ticks=range(0,len(tickslist)+1)
        ax.set_xticks(range(len(df_data['domainSize'].unique())), labels=tickslables)
        plt.xlabel("Domain Size")
        plt.ylabel("Noise per Query")
        plt.tight_layout()
        name="/DomainsizeToNoise-pointQueriesvsRangeQueries-Selectivity:"+str(sel)
        plt.savefig(outdir+name+".png")
        plt.savefig(outdir+name+".svg")



def rangeQueryRangeToNoise(df,datasets,outdir,colors, markers,linestyles):
    PQs=df["pointQuery"].unique().tolist()

    fixed_sel=0.01
    fig=plt.figure(figsize=(8, 5), dpi=80)
    df_data=pd.DataFrame( columns =['range', 'overhead','selectivity'])
    
    #things that needt to be fixed: NUM DATAPOINTS, Selectivity,only Range Queries

    df_tmp=df[df['pointQuery']=="false"]
    df_tmp=df_tmp[df_tmp['pointQuery']=="false"]
    df_tmp=df_tmp[df_tmp['numAttributes']==1]
    selected_ds=df_tmp["RefIndex"].tolist()

    q_range=list()
    q_noise=list()
    q_sel=list()
        
    for ds_index in selected_ds:
        n= int(datasets[ds_index]["NUM_DATAPOINTS"])
        r= int(datasets[ds_index]["RANGEQUERY_RANGE"])
        sel= round(float(datasets[ds_index]["MAX_SENSITIVITY"]),2)

        for q in datasets[ds_index]["QUERIES"]:

            q_range.append(r)  
            q_noise.append(int(q["noise"])) 
            
        df_tmp=pd.DataFrame(list(zip(q_range, q_noise)),  columns =['range', 'overhead'])
        df_tmp["selectivity"]=sel
        df_data=pd.concat([df_data,df_tmp])
    df_data=df_data[df_data['selectivity']==fixed_sel]


    #customPalette = sns.set_palette(sns.color_palette(colors[2:]))

    sns.boxplot(x = df_data['range'],
                    y = df_data['overhead'],
                    color=colors[4])
    ax = plt.gca()
    tickslist= df_data['range'].sort_values().unique().tolist()
    tickslables=[r'$2^{'+str(int(log(i)/log(2)))+r'}$' for i in tickslist]
    ticks=range(0,len(tickslist)+1)
    ax.set_xticks(range(len(df_data['range'].unique())), labels=tickslables)
    plt.xlabel("Range of Range Queries")
    plt.ylabel("Noise per Query")
    plt.tight_layout()
    name="/Noise-perRangeQueryRange-selectivity:"+str(fixed_sel)
    plt.savefig(outdir+name+".png")
    plt.savefig(outdir+name+".svg")
