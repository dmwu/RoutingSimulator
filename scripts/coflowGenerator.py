def all2all_generator(torNum, filename):
    ff = '../trafficTraces/'+filename
    with open(ff,'w+') as f:
        f.write(str(torNum)+' 1\n')
        f.write('0 0 '+str(torNum)+' ')
        for tor in range(torNum):
            f.write(str(tor)+' ')
        f.write(str(torNum)+' ')
        for tor in range(torNum):
            f.write(str(tor)+':'+'64 ')
        f.write('\n')
    f.close()


if __name__ == "__main__":
    all2all_generator(128, "all2all_128")


