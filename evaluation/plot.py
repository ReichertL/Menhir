import os
import argparse
import json
import matplotlib.pyplot as plt
import pandas as pd
from math import *
import numpy as np
import seaborn as sns

from src.getData import *
from src.insertionTimes import numDatapointsToInsertionTime
from src.deletionTimes import *
from src.timePerQuery import *
from src.runtimeToSelectivity import *
from src.util import *
from src.oramOverhead import *
from src.padding import *
from src.rangePointComparision import *
from src.retrieveExactly import *
from src.parallelisationOverhead import *


def main():
    parser = argparse.ArgumentParser(
                        prog = 'Plotting graphs for Menhir',
                        description = 'Helps to plot results.')
    parser.add_argument('-f', dest='files', nargs='+', default=[], help='Pass path to csv files. Searches recusivly for files.')
    parser.add_argument('-p', dest='dirpath',nargs='+',  default=[], help='Pass path to folder with json files. All json files in Folder will be read.')
    parser.add_argument('-naive', dest='naive_files', nargs='+', default=[], help='Pass path to csv files whith data from Naive measurements. Searches recusivly for files.')

    parser.add_argument('-outdir',  nargs='?', default='../plots')           # positional argument
    parser.add_argument('-i', action="store_true",   default=False)           # positional argument
    parser.add_argument('-timePerQuery', action="store_true",  default=False)           # positional argument
    parser.add_argument('-overhead', action="store_true",  default=False)           # positional argument
    parser.add_argument('-rangeToPoint', action="store_true",  default=False)           # positional argument
    parser.add_argument('-selectivity', action="store_true",  default=False)           # positional argument
    parser.add_argument('-sel2', dest='selectivityToSetsize', action="store_true",  default=False)           # positional argument
    parser.add_argument('-valueSize', action="store_true",  default=False)           # positional argument
    parser.add_argument('-deletion', action="store_true",  default=False)           # positional argument
    parser.add_argument('-padding', action="store_true",  default=False)           # positional argument
    parser.add_argument('-noiseRQ', action="store_true",  default=False)           # positional argument
    parser.add_argument('-domainSize', action="store_true",  default=False)           # positional argument
    parser.add_argument('-retrieved', action="store_true",  default=False)           # positional argument
    parser.add_argument('-retrievedSelected', action="store_true",  default=False)           # positional argument
    parser.add_argument('-parallel', action="store_true",  default=False)           # positional argument

    #parser.add_argument('-opS',  action="store_true",  default=False)           # positional argument

    args = parser.parse_args()



    

    global outdir
    outdir=args.outdir
    #print(args.files)
    #print(args.dirpath)
    datasets,df=getData(args.files, args.dirpath)
    
    naive_datasets,naive_df=getData(args.naive_files, '')


    if(len(datasets)==0):
        exit(1)

    #numDatapointsToNoiseFraction(df)
    #insertionRequiredPadding()
    #numDatapointToTimeperUsefulBlocks(df)
    #numDatapointToNumRetrievedBlocks(df)

    #-----using these plotting functions
    if args.i:
        numDatapointsToInsertionTime(datasets, outdir, colors,markers,linestyle)

    if args.padding:
        selectivityToPadding_GroupedBoxPlot(df,datasets, outdir,colors)
        selectivityToPadding_lineplotWithCI(df,datasets, outdir,colors, markers, linestyle) 

    if args.noiseRQ:
        rangeQueryRangeToNoise(df,datasets,outdir,colors, markers,linestyle) 
    
    if args.domainSize:
        domainSizeToNoise_RangeQuery_GroupedBarPlot_withBoxPlot(df,datasets, outdir,colors)

    if args.rangeToPoint:
        selectivityToOverhead_PointQueryVsRangeQuery_GroupedBarPlot_withBoxPlot(df,datasets, outdir,colors)
    
    if args.timePerQuery:
        numDatapointToTimePerQuery(datasets,outdir,colors, markers,linestyle)
        if(args.naive_files !=[]):
            numDatapointToTimePerQueryComparision(datasets, naive_datasets,outdir,colors, markers,linestyle)

    if args.retrievedSelected:
        retrievedNumDatapointsToTimePerQuery(True,datasets,outdir,colors, markers,linestyle)

    if args.retrieved:
        retrievedNumDatapointsToTimePerQuery(False,datasets,outdir,colors, markers,linestyle)

    if args.parallel:
        numOSMsToORAMRuntime(datasets,outdir,colors, markers,linestyle)
        numDatapointsToORAMRuntimeCompareORAMLogCapacities(datasets,outdir,colors, markers,linestyle)


    if args.selectivity:   
        queryRuntimeToSelectivity_lineplotWithCI(df,datasets, outdir,colors, markers, linestyle)
    
    if args.selectivityToSetsize:
        queryRuntimeToSelectivityByNumDatapoints_lineplotWithCI(df,datasets, outdir,colors, markers, linestyle)
    
    
    if args.valueSize:
        #queryRuntimeToSelectivityPerValueSize_lineplotWithCI(df,datasets, outdir,colors, markers, linestyle)
        retrievedNumDatapointsToTimePerQueryToValueSize(datasets, outdir,colors, markers, linestyle)


    if args.overhead:
        overheadPercentagePerQueryToSelectivity_GroupedBarPlot_withBoxPlot(df,datasets,outdir,colors)
    
    if args.deletion:
        numDatapointsToDeletionTime(datasets, outdir, colors,markers,linestyle)



main()