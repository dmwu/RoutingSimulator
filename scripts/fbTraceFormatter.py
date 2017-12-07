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
                    print(mapperNum, reducerNum)
                    if(mapperNum+reducerNum <= int(sizeFilter)):
                        coflowIndex += 1
                        if(startingTime < 0 or int(items[1]) < startingTime):
                            startingTime = int(items[1])
                        items[1] = str((int(items[1])-startingTime)/int(timeScaleRatio))
                        items[0] = str(coflowIndex)
                        wf.write(' '.join(items)+'\n')
                else:
                    wf.write(" ".join(items)+"\n")
        rf.close()
    wf.close()

def combineTwoNewTrace(file1, file2, newFile):
    with open(file1,'r') as f1:
        lines1 = f1.readlines()
        lines1 = [x.strip() for x in lines1 if len(x.strip().split(' '))>2]
    f1.close()
    with open(file2,'r') as f2:
        lines2 = f2.readlines()
        lines2 = [x.strip() for x in lines2 if len(x.strip().split(' '))>2]
    f2.close()
    lines3 = lines1+lines2
    lines3.sort(key = lambda x:int(x.split(' ')[1]))
    with open(newFile,'w+') as f3:
        for i in range(len(lines3)):
            items = lines3[i].split(' ')
            items[0] = str(int(i))
            f3.write(' '.join(items)+'\n')
    f3.close()

if __name__ == "__main__":
    formatter(sys.argv[1], sys.argv[2], sys.argv[3])
    #combineTwoNewTrace(sys.argv[1], sys.argv[2], sys.argv[3])
