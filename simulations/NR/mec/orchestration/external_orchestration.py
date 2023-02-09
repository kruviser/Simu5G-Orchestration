#!/usr/bin/env python
# coding: utf-8 
import pandas as pd 
import numpy as np 
import sys 
# k: current number of tasks
# m: number of servers
# n: server capacity

def read_file(file):  
    df = pd.read_table(file, sep='\s+', header=None, names=['timestamps', 'tasks'])
    # timestamp = np.array(df.iloc[:,:-1].values.flatten().tolist())
    # tasks = df.iloc[:,-1].to_numpy()
    # print(df)
    return df  

def orchestration(k, m, n, timestamp, method): 
    if method == 'by_threshold':     # activation when reaching threshold
        thr = 2                       
        if k >= m*n - thr:
            return 1                 # activate new server      
        elif k < (m-1)*n - 1: 
            return -1                # deactivate server          
        else:              
            return 0                 # no action required                  
    elif method == 'reactive':                  
        if k >= m*n:                 # activation when reaching maximum capacity 
            return 1                  
        elif k < (m-1)*n - 1: 
            return -1                  
        else: 
            return 0    
    elif method == 'conservative':   # activation when reaching percentage
        percentage_activation = 0.5
        percentage_deactivation = 0.2 
        if k >= (percentage_activation*m*n):  
            return 1 
        elif k <= (percentage_deactivation*m*n): 
            return -1
        else: 
            return 0 
    elif method == 'oracle':         
        activation_time = 1 
        next_timestamp = timestamp + activation_time
        snapshot = df['timestamps'].tolist() 
        if next_timestamp in snapshot: 
            task = df.loc[df['timestamps'] == next_timestamp, 'tasks'].iloc[0]    
            #print('value is on list!')
            #print('task: ', task)
            if task >= m*n:                 
                return 1                   
        elif k < (m-1)*n - 0.5: 
            return -1                  
        else: 
            return 0     
      
if __name__ == "__main__":        
    #file = 'traceFile_timestamp_custom2.txt'
    file = sys.argv[6]
    df = read_file(file)
        
    tasks = int(sys.argv[1])        
    m = int(sys.argv[2]) 
    n = int(sys.argv[3]) 
    time = float(sys.argv[4])
    method = sys.argv[5]
    
    action = orchestration(tasks, m, n, time, method)
    print(action)
    