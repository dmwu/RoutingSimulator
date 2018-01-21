import numpy as np
import random

def main(coflowNum, k, rackNum, fanin):
    coflowSizeKB = [2, 6, 10, 14] #in KB
    load = 0.1
    interArrivalTimeMean = 1/load*fanin * np.mean(coflowSizeKB)*1000*8.0*3*1000/(1e9 * k**3/2) #in ms 20% utilization
    ff = '../trafficTraces/deadlineCoflows_fanin_'+str(fanin)+"_load_"+str(load)
    random.seed(0)
    arrivalTime = 0
    with open(ff, 'w+') as f:
        f.write(str(rackNum)+" "+str(coflowNum)+'\n')
        for des in range(coflowNum):
            cands = range(coflowNum)
            cands.remove(des)
            sources = np.random.choice(cands,fanin,replace=False)
            arrivalTime = 0 if des==0 else arrivalTime+round(random.expovariate(1/interArrivalTimeMean), 4)
            f.write(str(des)+' '+str(arrivalTime)+' '+str(fanin)+' ')
            for src in sources:
                f.write(str(src)+' ')
            f.write('1 '+str(des)+':'+str(np.random.choice(coflowSizeKB)*fanin))
            f.write('\n')
        f.close()

if __name__ == "__main__":
    main(1024, 16, 128, 40)