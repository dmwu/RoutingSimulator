import sys

#intensify coflow + filtering gigantic coflow
def formatter(filename, timeScaleRatio, sizeFilter):
    with open('new'+filename,'w+') as wf:
        with open(sys.argv[1],'r') as rf:
            startingTime = -1
            coflowIndex = -1
            for line in rf:
                items = line.strip().split(' ')
                if(len(items)>2):
                    mapperNum = int(items[2])
                    reducerNum = int(items[2+mapperNum+1])
                    #print(mapperNum, reducerNum)
                    if(mapperNum+reducerNum <= int(sizeFilter)):
                        coflowIndex += 1
                        if(startingTime < 0 or int(items[1]) < startingTime):
                            startingTime = int(items[1])
                        items[1] = str((int(items[1])-startingTime)/int(timeScaleRatio))
                        items[0] = str(coflowIndex)
                        wf.write(' '.join(items)+'\n')
        rf.close()
    wf.close()

if __name__ == "__main__":
    formatter(sys.argv[1], sys.argv[2], sys.argv[3])
