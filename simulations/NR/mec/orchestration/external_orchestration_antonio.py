#!/usr/bin/env python
# coding: utf-8 

import sys
from numpy import double
import pandas as pd  
import math

# k: current number of tasks
# m: number of servers
# n: server capacity

def read_file(file):  
   # df = pd.read_table(file, delim_whitespace=True, header=None, names=['timestamps', 'tasks'])
    # timestamp = np.array(df.iloc[:,:-1].values.flatten().tolist())
    # tasks = df.iloc[:,-1].to_numpy()
    # print(df)
    return df  

def orchestration(k, m, n, timestamp, method): 
    if method == 'by_threshold':
        #print('by_threshold') 
        thr = 0                      # to be defined manually  
        if k >= m*n - thr:
            return 1                 # activate new server
        elif k < (m-1)*n - 1: 
            return -1                # deactivate server  
        else: 
            return 0                 # no action required 
    elif method == 'reactive': 
        #print('reactive')                 
        if k >= m*n:                 # activation when reaching maximum capacity 
            return 1                  
        elif k < (m-1)*n - 1: 
            return -1                  
        else: 
            return 0    
    elif method == 'conservative':   # activation when reaching percentage
        #print('conservative')
        percentage_activation = 0.5
        percentage_deactivation = 0.3
        if k >= percentage_activation*m*n:  
            return 1 
        elif k <= percentage_deactivation*m*n: 
            return -1
        else: 
            return 0 
    elif method == 'oracle':         # activation based on the current number of tasks
        #print('oracle')
        # return tasks value based on timestamp value 
        task = df.loc[df['timestamps'] == timestamp, 'tasks'].iloc[0]    
        if task > m*n: 
            return 1 
        elif task < m*n: 
            return -1 
        else: 
            return 0  
        
        # return tasks value based on next timestamp value 
        # ind = df.index[df['timestamps'] == timestamp].tolist() 
        # ind = ind[0]
        # task = df['tasks'].iloc[ind+1]    
        # if task > m*n: 
        #     return 1 
        # elif task < m*n: 
        #     return -1 
        # else: 
        #     return 0  
               
if __name__ == "__main__": 
    # print(f"Arguments count: {len(sys.argv)}")
    # for i, arg in enumerate(sys.argv):
    #   print(f"Argument {i:>6}: {arg}")
    
    file = 'traceFile_timestamp_custom.txt'
    #df = read_file(file)
        
    tasks = int(sys.argv[1])        
    m = int(sys.argv[2]) 
    n = int(sys.argv[3]) 
    time = double(sys.argv[4])
    
    method = sys.argv[5]
    #method = 'by_threshold'

    # action = orchestration(tasks, m, n, method)
    action = orchestration(tasks, m, n, time, method)
    print(action) 
    