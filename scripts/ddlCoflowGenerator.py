import numpy as np
import random

def main(coflowNum, k, rackNum, serverNum, fanin):
    coflowSizeKB = [2, 6, 10, 14] #in KB
    interArrivalTimeMean = 20* np.mean(coflowSizeKB)*1000*8.0*3*1000/(1e9 * k**3/2) #in ms 20% utilization
    ff = '../trafficTraces/deadlineCoflows_fanin_'+str(fanin)

    arrivalTime = 0
    with open(ff, 'w+') as f:
        f.write(str(rackNum)+" "+str(coflowNum)+'\n')
        for i in range(coflowNum):
            sources = np.random.choice(range(serverNum), fanin+1, replace=False)
            arrivalTime = 0 if i==0 else arrivalTime+round(random.expovariate(1/interArrivalTimeMean), 4)
            f.write(str(i)+' '+str(arrivalTime)+' '+str(fanin)+' ')
            for x in range(fanin):
                f.write(str(sources[x])+' ')
            f.write('1 '+str(sources[fanin])+':'+str(np.random.choice(coflowSizeKB)*fanin))
            f.write('\n')
        f.close()

if __name__ == "__main__":
    main(1000, 16, 128, 1024, 20)