import pandas as pd 
pd.set_option('display.max_rows', None)
pd.set_option('display.max_columns', None)

#name='data-ipfs-2.csv'
#name='USCensus1990.csv'
name='covid-data/covid-data-adjusted.csv'
df=pd.read_csv(name)#, nrows=20)
agg=df.agg(['min', 'max','std'])
agg.transpose()
print(agg)
