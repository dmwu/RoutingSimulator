import sys
def all2all_generator(torNum, filename, datasize):
    ff = '../trafficTraces/'+filename
    with open(ff,'w+') as f:
        f.write(str(torNum)+' 1\n')
        f.write('0 0 '+str(torNum)+' ')
        for tor in range(torNum):
            f.write(str(tor)+' ')
        f.write(str(torNum)+' ')
        for tor in range(torNum):
            f.write(str(tor)+':'+str(datasize)+' ')
        f.write('\n')
    f.close()


if __name__ == "__main__":
    numServers = int(sys.argv[1])**3/4
    dataSize_MB = int(sys.argv[2])
    print 'numServers:%d, dataSize_MB:%d' % (numServers,dataSize_MB)
    all2all_generator(numServers, "all2all_"+str(numServers), dataSize_MB)


