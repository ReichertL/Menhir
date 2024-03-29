import pandas as pd 
from pandarallel import pandarallel


addressesList=[]
def prepareAddress(row):
    this_type=row['ip']
    index=addressesList.index(this_type)
    return index

def main():
    name='data-ipfs.csv'
    df=pd.read_csv(name)#, nrows=20)
    print('ok')
    global addressesList
    addressesList=df['ip'].unique().tolist()
    pandarallel.initialize(progress_bar=True)
    df['ip']=df.parallel_apply(prepareAddress,  axis=1)
    print('ok')
    df.to_csv('data-ipfs-2.csv', index=False)

main()

