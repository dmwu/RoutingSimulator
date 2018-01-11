import sys,random,numpy as np

def all2all_generator(serverNum, K, Ratio, datasize):
    ff = '../trafficTraces/a2a_K'+str(K)+"Ratio"+str(Ratio)+"server"+str(serverNum)
    with open(ff,'w+') as f:
        f.write(str(serverNum)+' 1\n')
        f.write('0 0 '+str(serverNum)+' ')
        for t in range(serverNum):
            f.write(str(t)+' ')
        f.write(str(serverNum)+' ')
        for t in range(serverNum):
            f.write(str(t)+':'+str(datasize)+' ')
        f.write('\n')
    f.close()


def pod2pod_generator(K,podSize, srcPod, desPod, datasize):
    ff = '../trafficTraces/p2p_K'+str(K)+'Ratio'+str(Ratio)+"podSize"+str(podSize)
    with open(ff, 'w+') as f:
        f.write(str(podSize)+' 1\n')
        f.write('0 0 '+str(podSize)+' ')
        for server in range(srcPod*podSize, (srcPod+1)*podSize):
            f.write(str(server)+' ')
        f.write(str(podSize)+' ')
        for server in range(desPod*podSize, (desPod+1)*podSize):
            f.write(str(server)+':'+str(datasize)+' ')
        f.write('\n')
    f.close()


def rack2rack_generator(rackSize, srcStart, desStart, filename, datasize):
    ff = '../trafficTraces/'+filename
    with open(ff, 'w+') as f:
        f.write(str(rackSize)+' 1\n')
        f.write('0 0 '+str(rackSize)+' ')
        for server in range(srcStart, srcStart+rackSize):
            f.write(str(server)+' ')
        f.write(str(rackSize)+' ')
        for server in range(desStart, desStart+rackSize):
            f.write(str(server)+':'+str(datasize)+' ')
        f.write('\n')
    f.close()

def permutation(serverNum,rackNum, K, dataSize):
    interArrivalTimeMean = dataSize*8.0/3 # one half time of finish a flow
    ff = '../trafficTraces/perm_K'+str(K)+'Ratio'+str(Ratio)+'server'+str(serverNum)
    src = [x for x in range(serverNum)]
    dest = src[:]
    dest = np.roll(dest, 4096-256)
    arrivalTime = 0
    with open(ff, 'w+') as f:
        f.write(str(rackNum)+" "+str(serverNum)+'\n')
        for i in range(serverNum):
            arrivalTime = 0 if i==0 else arrivalTime+round(random.expovariate(1/interArrivalTimeMean),2)
            f.write(str(i)+' '+str(arrivalTime)+' 1 '+str(src[i])+' 1 '+str(dest[i])+':'+str(dataSize))
            f.write('\n')
    f.close()

def all2OneRack(serverNum,rackNum, Ratio, destRack, K, dataSize):
    interArrivalTimeMean = dataSize*8.0/2 # one half time of finishing a flow
    ff = '../trafficTraces/a2rack_K'+str(K)+'Ratio'+str(Ratio)+'RackSize'+str(K*Ratio/2)
    destLowIndex = destRack*K*Ratio/2
    destHighIndex = (destRack+1)*K*Ratio/2
    dest=[x for x in range(destLowIndex, destHighIndex)]
    src = [x for x in range(serverNum) if x not in dest]
    random.shuffle(src)
    arrivalTime = 0
    with open(ff, 'w+') as f:
        f.write(str(rackNum)+" "+str(len(src))+'\n')
        for i in range(len(src)):
            arrivalTime = 0 if i==0 else arrivalTime+round(random.expovariate(1/interArrivalTimeMean),2)
            f.write(str(i)+' '+str(arrivalTime)+' 1 '+str(src[i])+' 1 '+str(random.choice(dest))+':'+str(dataSize))
            f.write('\n')
    f.close()

if __name__ == "__main__":
    K = int(sys.argv[1])
    Ratio = int(sys.argv[2])
    dataSize_MB = int(sys.argv[3])
    numServers = K**3/4*Ratio
    serversPerRack = K/2*Ratio;
    serversPerPod = serversPerRack*K/2
    rackNum = K/2*K
    print 'numServers:%d, serversPerRack:%d dataSize_MB:%d' % (numServers, serversPerRack, dataSize_MB)
    #all2all_generator(numServers, K,Ratio, dataSize_MB)
    #pod2pod_generator(K,serversPerPod , 0, K-1, dataSize_MB)
    #rack2rack_generator(K/2, 0, 0+podSize, 'rack2rack_'+str(K/2), dataSize_MB)
    permutation(numServers, rackNum, K, dataSize_MB)
    #all2OneRack(numServers, rackNum, Ratio, 0, K, dataSize_MB)

