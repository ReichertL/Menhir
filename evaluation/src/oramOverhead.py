import os
import matplotlib.pyplot as plt
import pandas as pd
from math import *
import numpy as np
import seaborn as sns
from src.insertionTimes import numDatapointsToInsertionTime
from src.timePerQuery import *
from src.runtimeToSelectivity import *
from src.util import *


def numAttributesToBlockSize(df):
    plt.figure()
    y=df.groupby("numAttributes").first()
    plt.plot((y.index+1),y["blockSize"],linestyle='--', marker='o')
        
    plt.xlabel("Number of Attributes")
    plt.ylabel("Size of a single ORAM Block")
    plt.title("Num Attributes vs Size of ORAM Block")
    plt.savefig(outdir+"/NumAttributs-to-ORAMBlockSize.png")

def insertionRequiredPadding():
    plt.figure()
    for nCols in [2,4,8,16,32]:
        x=range(1,1000,10)
        #func ceil(1.4*log(n))+ceil(1+1.4*log(n))*3   --- find leaf+ rebalance each node
        y=[(ceil(1.4*log(n)+1)*nCols+ceil(1.4*log(n)+1)*3)*nCols for n in x]
        plt.plot(x,y, label="NumAttributes:"+str(nCols))
        
    plt.xlabel("Num Elements in ORAM")
    plt.ylabel("Number of ORAM Operations for Insertion")
    plt.legend()
    plt.savefig(outdir+"/numDatapoints-To-OramOperations.png")


