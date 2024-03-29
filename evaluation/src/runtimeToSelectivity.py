
import matplotlib.pyplot as plt
import pandas as pd
from math import *
import numpy as np
import seaborn as sns
from src.util import *



def timePerQueryToSelectivity_GroupedBarPlot_withSTD(df,datasets, outdir,colors):
    qFunctions=df["queryFunction"].unique().tolist()
    dbFormats=df["columnFormat"].unique().tolist()
    n=65536
    qf=0
    
    plt.figure()
    mmax=0.03
    df_data=pd.DataFrame( columns =['selectivity', 'overhead in s',"numAttributes"])

    for thisFormat in dbFormats:         
        df_tmp=df[df['queryFunction']==qf]
        df_tmp=df_tmp[df_tmp['numDatapoints']==n]
        df_tmp=df_tmp[df_tmp['columnFormat']==thisFormat]
        df_tmp=df_tmp[df_tmp['useORAM']==True]

        selected_ds=df_tmp["RefIndex"].tolist()

        q_sel=list()
        q_noise=list()
        for ds_index in selected_ds:        
            for q in datasets[ds_index]["QUERIES"]:
                q_sel.append(float(q["real"])/n)  # selectivity
                value=float(q["overhead"])/1000000000
                q_noise.append(value) #overhead in s
        
        nA=len(thisFormat.split(","))
        df_tmp=pd.DataFrame(list(zip(q_sel, q_noise)),  columns =['selectivity', 'overhead in s'])
        df_tmp["numAttributes"]=str(nA)

        df_data=pd.concat([df_data,df_tmp])

    for thisFormat in dbFormats:         
        df_tmp=df[df['queryFunction']==qf]
        df_tmp=df_tmp[df_tmp['numDatapoints']==n]
        df_tmp=df_tmp[df_tmp['columnFormat']==thisFormat]
        df_tmp=df_tmp[df_tmp['useORAM']==False]

        selected_ds=df_tmp["RefIndex"].tolist()

        q_sel=list()
        q_noise=list()
        for ds_index in selected_ds:        
            for q in datasets[ds_index]["QUERIES"]:
                q_sel.append(float(q["real"])/n)  # selectivity
                value=float(q["overhead"])/1000000000
                q_noise.append(value) #overhead in s
        
        nA=len(thisFormat.split(","))
        df_tmp=pd.DataFrame(list(zip(q_sel, q_noise)),  columns =['selectivity', 'overhead in s'])
        df_tmp["numAttributes"]="LinearScan-"+str(nA)
        df_data=pd.concat([df_data,df_tmp])  

    steps=10
    groups=np.arange(0, mmax, mmax/steps)

    df_data=df_data[df_data['selectivity']>0]
    df_data["selectivity"]=pd.cut(df_data["selectivity"],groups)

    df_means=df_data.groupby(["selectivity","numAttributes"]).mean().reset_index()
    df_stds=df_data.groupby(["selectivity","numAttributes"]).std().reset_index()
    df_count=df_data.groupby(["selectivity","numAttributes"]).count().reset_index()

    df_index=df_count[df_count["overhead in s"]>=10].index
    df_means = df_means.filter(items = df_index, axis=0)
    df_stds = df_stds.filter(items = df_index, axis=0)
    to_tuples =df_means["selectivity"].unique()
    interval_tuples=[(i.left*100,i.right*100) for i in to_tuples]
    str_index=["("+"{:.1f}".format(i)+","+"{:.1f}".format(j)+"]" for (i,j) in interval_tuples]

    print(df_means)
    newData=[df_means["selectivity"].unique().tolist()]
    newColumns=["selectivity"]
    show=[]
    err=[]
    for i in df_data['numAttributes'].unique().tolist():
        temp_means=df_means[df_means["numAttributes"]==i]
        temp_stds=df_stds[df_stds["numAttributes"]==i]
        newData.append(temp_means['overhead in s'].tolist())
        newData.append(temp_stds['overhead in s'].tolist())
        newColumns.append(str(i)+" attribute(s)")
        show.append(str(i)+" attribute(s)")
        newColumns.append("std-"+str(i))
        err.append("std-"+str(i))
    newData=list(zip(*newData))    
    df_new=pd.DataFrame(newData, columns=newColumns)
    yerr_cols=[df_new[i]for i in err]

    from itertools import cycle, islice  
    my_colors = list(islice(cycle(colors), None, len(df_new)))
    df_new[show].plot.bar(yerr=yerr_cols,color=my_colors)

    plt.xticks(ticks=range(0,len(str_index)-1), labels=str_index[0:-1],rotation=45)
    plt.xlabel("Selectivity in %")
    plt.ylabel("Time per Query in s")
    plt.legend()
    plt.tight_layout()
    name="/Duration-perQuerySelectivity-n:"+str(n)+"-QueryFunction:"+str(qf)+"-mmax:"+str(mmax)+"-min10perGroup"
    plt.savefig(outdir+name+".png")
    plt.savefig(outdir+name+".svg")



def queryRuntimeToSelectivityByNumDatapoints_lineplotWithCI(df,datasets, outdir,colors, markers, linestyles):
    n_values=df["numDatapoints"].unique().tolist()
    print(n_values)
    dbFormats=df["columnFormat"].unique().tolist()
    #n=65536-2
    groupsize=10

    mmax=0.1
    steps=10
    groups=np.arange(0, mmax, mmax/steps)

    j=0
    fig=plt.figure(figsize=(8, 5), dpi=80)
    for n in n_values :
        print("selecting only files with numDatapoints="+str(n))
        df_data=pd.DataFrame( columns =['selectivity', 'overhead',"numAttributes"])

        for thisFormat in dbFormats:         
            df_tmp=df[df['numDatapoints']==n]
            df_tmp=df_tmp[df_tmp['columnFormat']==thisFormat]

            selected_ds=df_tmp["RefIndex"].tolist()

            q_sel=list()
            q_noise=list()
            for ds_index in selected_ds:
                #n= int(datasets[ds_index]["NUM_DATAPOINTS"])
                for q in datasets[ds_index]["QUERIES"]:
                    #print(q["real"]+" in query")
                    selectivity=float(q["real"])/n#float(q["total"])/n
                    #print(selectivity)
                    q_sel.append(selectivity) #this can be used for sanity checks to see if large error bars are due to noise or padding
                    #q_sel.append(float(q["real"])/n)  # selectivity
                    value= float(q["overhead"])/1000000000  #casting time from ns to seconds
                    q_noise.append(value) # in s
        
            nA=len(thisFormat.split(","))
            df_tmp=pd.DataFrame(list(zip(q_sel, q_noise)),  columns =['selectivity', 'overhead'])
            df_tmp["numAttributes"]=nA
            df_data=pd.concat([df_data,df_tmp])
        print(len(df_data))

        df_data=df_data[df_data['selectivity']>0]
        df_data["selectivity"]=pd.cut(df_data["selectivity"],groups)
        df_data=df_data.dropna(how='any')

        to_tuples =df_data["selectivity"].sort_values().unique().dropna()
        interval_tuples=[(i.left*100,i.right*100) for i in to_tuples]
        str_index=["["+"{:.1f}".format(i)+","+"{:.1f}".format(j)+")" for (i,j) in interval_tuples]

        s= df_data["selectivity"].sort_values().unique()
        selected=s[-1]

        attributes=df_data["numAttributes"].unique()
        for i,nA in enumerate(attributes):
                    
            line_data=df_data[df_data["numAttributes"]==nA]
            santiy=line_data[line_data["selectivity"].isin([selected])]
            #print(".............."+str(nA)+".............")
            #print(line_data)
            #print("sanity data for "+str(selected))
            #print(santiy)
            #print(santiy["overhead"].max())
            #print(santiy["overhead"].min())
            #print(santiy["overhead"].mean())
            #print(santiy["overhead"].median())
            #only consider selectivities where a useable number of datapoints exist
            check_count=line_data.groupby("selectivity").count()
            #print(check_count)
            selectivites=check_count[check_count["overhead"]<groupsize].index.to_list()
            #print(selectivites)
            line_data = line_data[~line_data["selectivity"].isin(selectivites)]
            
          
            line_data=line_data.groupby("selectivity",observed=True)
            tmp=line_data.apply(mean_confidence_interval)
            #print("tmp:")
            #print(tmp)
            x= tmp.index.tolist()
            tmp_data=tmp.tolist()
            y,err= zip(*tmp_data)
            y=list(y)
            err=list(err)
            x=range(1,len(y)+1)
            thislabel="2^"+str(ceil(log(n)/log(2)))
            plt.errorbar(x,y,yerr=err,capsize=3, label=thislabel,color=colors[j%len(colors)], linestyle=linestyles[j%len(linestyles)], marker=markers[j%len(markers)],markersize=5) 
            j=j+1

    plt.legend()
    plt.xticks(ticks=range(1,len(str_index)+1), labels=str_index,rotation=45)
    plt.xlabel("Selectivity in %")
    plt.ylabel("Query Duration in s")
    plt.tight_layout()
    name="/Duration-perQuerySelectivity-perNumDatapoints-mmax:"+str(mmax)+"-min"+str(groupsize)+"perGroup"
    plt.savefig(outdir+name+".png")
    plt.savefig(outdir+name+".svg")




def queryRuntimeToSelectivity_lineplotWithCI(df,datasets, outdir,colors, markers, linestyles):
    n_values=df["numDatapoints"].unique().tolist()

    dbFormats=df["columnFormat"].unique().tolist()
    #n=65536-2
    groupsize=10

    mmax=0.1
    steps=10
    groups=np.arange(0, mmax, mmax/steps)
    
    for n in n_values :
        print("selecting only files with numDatapoints="+str(n))
        fig=plt.figure(figsize=(8, 5), dpi=80)
        df_data=pd.DataFrame( columns =['selectivity', 'overhead',"numAttributes"])

        for thisFormat in dbFormats:         
            df_tmp=df[df['numDatapoints']==n]
            df_tmp=df_tmp[df_tmp['columnFormat']==thisFormat]

            selected_ds=df_tmp["RefIndex"].tolist()

            q_sel=list()
            q_noise=list()
            for ds_index in selected_ds:
                n= int(datasets[ds_index]["NUM_DATAPOINTS"])
                for q in datasets[ds_index]["QUERIES"]:
                    #print(q["real"]+" in query")
                    selectivity=float(q["real"])/n#float(q["total"])/n
                    #this is only relevant for 1 meas
                    #if(selectivity<=0.01):
                    #    selectivity=0.011
                    #print(selectivity)
                    q_sel.append(selectivity) #this can be used for sanity checks to see if large error bars are due to noise or padding
                    #q_sel.append(float(q["real"])/n)  # selectivity
                    value= float(q["overhead"])/1000000000  #casting time from ns to seconds
                    q_noise.append(value) # in s
        
            nA=len(thisFormat.split(","))
            df_tmp=pd.DataFrame(list(zip(q_sel, q_noise)),  columns =['selectivity', 'overhead'])
            df_tmp["numAttributes"]=nA
            df_data=pd.concat([df_data,df_tmp])
        

        df_data=df_data[df_data['selectivity']>0]
        df_data["selectivity"]=pd.cut(df_data["selectivity"],groups)
        df_data=df_data.dropna(how='any')

        to_tuples =df_data["selectivity"].sort_values().unique().dropna()
        interval_tuples=[(i.left*100,i.right*100) for i in to_tuples]
        str_index=["["+"{:.1f}".format(i)+","+"{:.1f}".format(j)+")" for (i,j) in interval_tuples]

        s= df_data["selectivity"].sort_values().unique()
        selected=s[-2]

        attributes=df_data["numAttributes"].unique()
        tickslist=list()
        for i,nA in enumerate(sorted(attributes,reverse=True)):
                    
            line_data=df_data[df_data["numAttributes"]==nA]
            santiy=line_data[line_data["selectivity"].isin([selected])]
            print(".............."+str(nA)+".............")
            print(line_data)
            print("sanity data for "+str(selected))
            print(santiy)
            print(santiy["overhead"].max())
            print(santiy["overhead"].min())
            print(santiy["overhead"].mean())
            print(santiy["overhead"].median())
            #only consider selectivities where a useable number of datapoints exist
            check_count=line_data.groupby("selectivity").count()
            print(check_count)
            selectivites=check_count[check_count["overhead"]<groupsize].index.to_list()
            #print(selectivites)
            #line_data = line_data[~line_data["selectivity"].isin(selectivites)]
            
            

            line_data=line_data.groupby("selectivity",observed=True)
            tmp=line_data.apply(mean_confidence_interval)
            xtuple= tmp.index.tolist()
            tmp_data=tmp.tolist()
            y,err= zip(*tmp_data)
            y=list(y)
            err=list(err)
            x=[ i.left for i in xtuple]
            if(len(x)>len(tickslist)):
                tickslist=x
            thislabel=str(nA)+" "
            if(nA==1):
                thislabel+="column"
            else:
                thislabel+="columns"
            if(nA==21):
                thislabel="Covid Data"
            if(nA==8):
                thislabel="IPFS Data"
            plt.errorbar(x,y,yerr=err,capsize=3, label=thislabel,color=colors[i%len(colors)], linestyle=linestyles[i%len(linestyles)], marker=markers[i%len(markers)],markersize=5) 

        plt.legend()
        plt.xticks(ticks=tickslist, labels=str_index,rotation=45)
        plt.xlabel("Selectivity in %")
        plt.ylabel("Query Duration in s")
        plt.tight_layout()
        name="/Duration-perQuerySelectivity-perNumAttributes-n:"+str(n)+"-mmax:"+str(mmax)+"-min"+str(groupsize)+"perGroup"
        plt.savefig(outdir+name+".png")
        plt.savefig(outdir+name+".svg")


def queryRuntimeToSelectivityPerValueSize_lineplotWithCI(df,datasets, outdir,colors, markers, linestyles):
    qFunctions=df["queryFunction"].unique().tolist()
    n_values=df["numDatapoints"].unique().tolist()
    print(n_values)
    groupsize=10

    overhead=60 #bytes

    mmax=0.1
    steps=10
    groups=np.arange(0, mmax, mmax/steps)
    print(groups)
    
    for n in n_values :
        fig=plt.figure(figsize=(8, 5), dpi=80)
        df_tmp=df[df['numDatapoints']==n]
        df_tmp=df_tmp[df_tmp['numAttributes']==1]

        selected_ds=df_tmp["RefIndex"].tolist()

        q_sel=list()
        q_noise=list()
        q_vs=list()
        for ds_index in selected_ds:
            n= int(datasets[ds_index]["NUM_DATAPOINTS"])
            vs= int(datasets[ds_index]["VALUE_SIZE"])
            for q in datasets[ds_index]["QUERIES"]:
                q_vs.append(vs)
                q_sel.append((float(q["real"])/n))  # selectivity
                #print(q["real"]+ " sel: "+str(float(q["real"])/n))
                value= float(q["overhead"])/1000000000  #casting time from ns to seconds
                q_noise.append(value) # in s
        
        df_data=pd.DataFrame(list(zip(q_sel, q_noise,q_vs)),  columns =['selectivity', 'overhead', 'valueSize'])

        df_data=df_data[df_data['selectivity']>0]
        df_data=df_data[df_data['selectivity']<=mmax]

        print(len(df_data[df_data['selectivity']>=0.08]))

        df_data["selectivity"]=pd.cut(df_data["selectivity"],groups)
        print(df_data['selectivity'].unique().tolist())

        to_tuples =df_data["selectivity"].sort_values().unique().dropna()
        #sorted(to_tuples)
        interval_tuples=[(i.left*100,i.right*100) for i in to_tuples]
        str_index=["["+"{:.1f}".format(i)+","+"{:.1f}".format(j)+")" for (i,j) in interval_tuples]


        s= df_data["selectivity"].sort_values().unique()
        print(s)
        selected=s[-2]

        valueSizes=df_data["valueSize"].unique()
        for i,vs in enumerate(sorted(valueSizes,reverse=True)):
                    
            line_data=df_data[df_data["valueSize"]==vs]
            santiy=line_data[line_data["selectivity"].isin([selected])]
            print(santiy)
            #print(santiy["overhead"].max())
            #print(santiy["overhead"].min())
            #print(santiy["overhead"].mean())
            #print(santiy["overhead"].median())
            #only consider selectivities where a useable number of datapoints exist
            check_count=line_data.groupby("selectivity").count()
            #print(check_count)
            #selectivites=check_count[check_count["overhead"]<groupsize].index.to_list()
            #print(selectivites)
            #line_data = line_data[~line_data["selectivity"].isin(selectivites)]
            
          
            line_data=line_data.groupby("selectivity",observed=True)
            tmp=line_data.apply(mean_confidence_interval)
            x= tmp.index.tolist()
            tmp_data=tmp.tolist()
            print(tmp_data)
            y,err= zip(*tmp_data)
            y=list(y)
            err=list(err)
            x=range(1,len(y)+1)
            label=str(vs)+" bytes"
            if vs==0:
                label="baseline"
            plt.errorbar(x,y,yerr=err,capsize=3, label=label,color=colors[i%len(colors)], linestyle=linestyles[i%len(linestyles)], marker=markers[i%len(markers)],markersize=5) 

        plt.legend()
        plt.xticks(ticks=range(1,len(str_index)+1), labels=str_index,rotation=45)
        plt.xlabel("Selectivity in %")
        plt.ylabel("Query Duration in s")
        plt.tight_layout()
        name="/Duration-perQuerySelectivity-perValueSize-n:"+str(n)+"-mmax:"+str(mmax)+"-min"+str(groupsize)+"perGroup"
        plt.savefig(outdir+name+".png")
        plt.savefig(outdir+name+".svg")

def overheadPerQueryToSelectivity_GroupedBarPlot_withSTD(df,datasets, outdir,colors):
    qFunctions=df["queryFunction"].unique().tolist()
    dbFormats=df["columnFormat"].unique().tolist()
    n=65536

    for qf in qFunctions :
        plt.figure()
        mmax=0.03
        df_data=pd.DataFrame( columns =['selectivity', 'overhead in s',"numAttributes"])

        for thisFormat in dbFormats:         
            df_tmp=df[df['queryFunction']==qf]
            df_tmp=df_tmp[df_tmp['numDatapoints']==n]
            df_tmp=df_tmp[df_tmp['columnFormat']==thisFormat]

            selected_ds=df_tmp["RefIndex"].tolist()

            q_sel=list()
            q_noise=list()
            for ds_index in selected_ds:

                for q in datasets[ds_index]["QUERIES"]:
                    t=float(q["overhead"])/float(q["total"])/1000000000
                    q_sel.append(float(q["real"])/n)  # selectivity
                    value=(float(q["padding"])+float(q["noise"]))*t
                    q_noise.append(value) #overhead in s
        
            nA=len(thisFormat.split(","))
            df_tmp=pd.DataFrame(list(zip(q_sel, q_noise)),  columns =['selectivity', 'overhead in s'])
            df_tmp["numAttributes"]=nA
            df_data=pd.concat([df_data,df_tmp])
        
        steps=10
        groups=np.arange(0, mmax, mmax/steps)

        df_data=df_data[df_data['selectivity']>0]

        df_data["selectivity"]=pd.cut(df_data["selectivity"],groups)

        df_means=df_data.groupby(["selectivity","numAttributes"]).mean().reset_index()
        df_stds=df_data.groupby(["selectivity","numAttributes"]).std().reset_index()
        df_count=df_data.groupby(["selectivity","numAttributes"]).count().reset_index()

        df_index=df_count[df_count["overhead in s"]>=10].index
        df_means = df_means.filter(items = df_index, axis=0)
        df_stds = df_stds.filter(items = df_index, axis=0)
        to_tuples =df_means["selectivity"].unique()
        interval_tuples=[(i.left*100,i.right*100) for i in to_tuples]
        str_index=["["+"{:.1f}".format(i)+","+"{:.1f}".format(j)+")" for (i,j) in interval_tuples]

        newData=[df_means["selectivity"].unique().tolist()]
        newColumns=["selectivity",]
        show=[]
        err=[]
        for i in df_data['numAttributes'].unique().tolist():
            temp_means=df_means[df_means["numAttributes"]==i]
            temp_stds=df_stds[df_stds["numAttributes"]==i]
            newData.append(temp_means['overhead in s'].tolist())
            newData.append(temp_stds['overhead in s'].tolist())
            newColumns.append(str(i)+" attribute(s)")
            show.append(str(i)+" attribute(s)")
            newColumns.append("std-"+str(i))
            err.append("std-"+str(i))
        newData=list(zip(*newData))    
        df_new=pd.DataFrame(newData, columns=newColumns)
        yerr_cols=[df_new[i]for i in err]

        from itertools import cycle, islice  
        my_colors = list(islice(cycle(colors), None, len(df_new)))
        df_new[show].plot.bar(yerr=yerr_cols,color=my_colors)



        plt.xticks(ticks=range(0,len(str_index)), labels=str_index,rotation=45)
        plt.xlabel("Selectivity in %")
        plt.ylabel("Overhead per Query in s")
        plt.legend()
        plt.tight_layout()
        name="/Overhead-perQuerySelectivity-n:"+str(n)+"-QueryFunction:"+str(qf)+"-mmax:"+str(mmax)+"-min10perGroup"
        plt.savefig(outdir+name+".png")
        plt.savefig(outdir+name+".svg")


