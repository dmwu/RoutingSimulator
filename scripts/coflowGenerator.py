import sys
def all2all_generator(serverNum, filename, datasize):
    ff = '../trafficTraces/'+filename
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


def pod2pod_generator(podSize, srcStart, desStart, filename, datasize):
    ff = '../trafficTraces/'+filename
    with open(ff, 'w+') as f:
        f.write(str(podSize)+' 1\n')
        f.write('0 0 '+str(podSize)+' ')
        for server in range(srcStart, srcStart+podSize):
            f.write(str(server)+' ')
        f.write(str(podSize)+' ')
        for server in range(desStart, desStart+podSize):
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

if __name__ == "__main__":
    K = int(sys.argv[1])
    numServers = K**3/4
    podSize = K**2/4
    dataSize_MB = int(sys.argv[2])
    print 'numServers:%d, podSize:%d dataSize_MB:%d' % (numServers, podSize, dataSize_MB)
    #all2all_generator(numServers, "all2all_"+str(numServers), dataSize_MB)
    pod2pod_generator(podSize, 0, 0+podSize, 'pod2pod_'+str(podSize), dataSize_MB)
    #rack2rack_generator(K/2, 0, 0+podSize, 'rack2rack_'+str(K/2), dataSize_MB)

