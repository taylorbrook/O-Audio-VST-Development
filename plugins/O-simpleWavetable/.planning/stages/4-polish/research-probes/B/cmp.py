import struct, array, sys
def load(p):
    d={}; f=open(p,'rb')
    while True:
        h=f.read(4)
        if not h: break
        nl=struct.unpack('<i',h)[0]; name=f.read(nl).decode(); n=struct.unpack('<i',f.read(4))[0]
        a=array.array('f'); a.frombytes(f.read(4*n)); d[name]=a
    return d
A=load(sys.argv[1]); B=load(sys.argv[2])
for k in A:
    a,b=A[k],B[k]
    idx=[i for i in range(len(a)) if a[i]!=b[i]]
    mx=max((abs(a[i]-b[i]) for i in idx), default=0.0)
    span=f"samples {idx[0]}..{idx[-1]} ({(idx[-1]-idx[0]+1)} span, {len(idx)} differ)" if idx else "bit-identical"
    print(f"{k:32s} max|diff| {mx:.3e}  {span}")
