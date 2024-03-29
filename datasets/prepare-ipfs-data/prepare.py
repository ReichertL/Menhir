import pandas as pd 
import swifter 
from pandarallel import pandarallel

uniquePeers=[]
uniqueEntryTypes=[]
uniqueContentIds=[]

def prepareAddress(row):
    addressString=row['address']
    tokens=addressString.split("/")
    #print(tokens)

    ipv=0
    if (tokens[1]=='ip4'):
        ipv=4
    elif (tokens[1]=='ip6'):
        ipv=6
    else:
        print(tokens)

    ip=int(hash(tokens[2]))
    if (ip==0):
        print(tokens)

    proto=0
    if( tokens[3]=='tcp'):
        proto=1
    elif(tokens[3]=='udp'):
        proto=2
        if(len(tokens)==6):
            if(tokens[5]=='quic'):
                proto=3
            else:
                print(tokens)
    else:
        print(tokens)

    port=int(tokens[4])

    this_type=row['entry_type']
    entry_type=uniqueEntryTypes.index(this_type)

    this_type=row['peer']
    peer=uniquePeers.index(this_type)

    this_type=row['cid']
    cid=uniqueContentIds.index(this_type)

    this_tp=row['timestamp']
    ts=int(pd.Timestamp(this_tp).timestamp())

    return [ipv, ip, port, proto,entry_type, peer, cid, ts]



def main (): 

    pandarallel.initialize(progress_bar=True)

    name='data-leonie_2023-09-18T23_10_08+02_00.csv'
    df=pd.read_csv(name)#, nrows=20)
    print('step1')
    global uniqueEntryTypes
    uniqueEntryTypes=df['entry_type'].unique().tolist()
    print('step2')
    global uniquePeers
    uniquePeers=df['peer'].unique().tolist()
    print('step3')
    global uniqueContentIds
    uniqueContentIds=df['cid'].unique().tolist()
    print('step4')
    df2=pd.DataFrame()

    df2[['ipv','ip','port','proto','entry_type','peer','cid','ts']]=df.parallel_apply(prepareAddress,  axis=1,result_type='expand' )
    print('step5')
    df2.to_csv('data-ipfs.csv', index=False)

main()
