# python3
import argparse
import sys
import re

DEBUGME = False

if __name__ == '__main__':

    infile = ''
    outfile = ''
    
    argParser = argparse.ArgumentParser(description='Transform standard ZView data file to csv.',exit_on_error=True,add_help=True)
    argParser.add_argument('filename',help="file name, with or without the '.z' suffix. The output file will have the same name, but with the '.csv' suffix (will overwrite without warning!).")
    args = argParser.parse_args()
    
    fname = args.filename
    if fname.endswith('.z') or fname.endswith('.Z'):
        infile = fname
    else:
        infile = fname + ".z"
    outfile = infile[:-2] + ".csv"
    print(f"Converting '{infile}' to '{outfile}'.")
    
    in_data = False
    idx_freq = 0
    idx_real = 4
    idx_img = 5
    dataset = {}
    indatacounter = 0
    outdatacounter = 0
    with open(infile,'r') as reader:
        for line in reader:
            l = line.strip().lower()
            if len(l) > 0:
                if not in_data:
                    if l == 'end comments':
                        in_data = True
                        continue
                else:
                    data = re.split("[ \t]+",l)
                    indatacounter += 1
                    
                    if DEBUGME:
                        #debug
                        outstr = f"{float(data[idx_freq])},{float(data[idx_real])},{float(data[idx_img])}"
                        print(f"in: {outstr}")
                    
                    freq = data[idx_freq] # keep it a string. Sort will come later
                    realval = float(data[idx_real])
                    imgval = float(data[idx_img])
                                                            
                    if (freq not in dataset):
                        dataset[freq] = [[realval],[imgval]]
                    else:
                        if DEBUGME:
                            print(f"old: {dataset[freq]}")
                        realvals = dataset[freq][0]
                        realvals.append(realval)
                        imgvals = dataset[freq][1]
                        imgvals.append(imgval)
                        dataset[freq] = [realvals,imgvals]
                        if DEBUGME:
                            print(f"new: {dataset[freq]}")
             
    # make the keys float
    dataset = {float(k) : v for k, v in dataset.items()}
    # sort on the keys
    dataset = dict(sorted(dataset.items()))
    with open(outfile,'w') as writer:
        if DEBUGME:
            print("********************************")
        for k,v in dataset.items():
            r = sum(v[0]) / len(v[0])
            i = sum(v[1]) / len(v[1])
            writer.write(f"{k},{r},{i}\n")
            outdatacounter += 1
print(f"Done. Data is written to '{outfile}'")
if indatacounter != outdatacounter:
    print(f"The {indatacounter} lines in the input file were averaged into {outdatacounter} lines.")
            
        