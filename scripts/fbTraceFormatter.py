import sys
if __name__ == "__main__":
    with open('new'+sys.argv[1],'w+') as wf:
        with open(sys.argv[1],'r') as rf:
            startingTime = -1
            startingFlowId = -1
            for line in rf:
                items = line.strip().split(' ')
                if(len(items)>2):
                    if(startingTime < 0 or int(items[1]) < startingTime):
                        startingTime = int(items[1])
                    if(startingFlowId < 0 or int(items[0] < startingFlowId)):
                        startingFlowId = int(items[0])
                    items[1] = str(int(items[1])-startingTime)
                    items[0] = str(int(items[0])-startingFlowId)
                wf.write(' '.join(items)+'\n')
        rf.close()
    wf.close()
