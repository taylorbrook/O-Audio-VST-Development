import re,sys
files=dict(a.split('=') for a in sys.argv[1:])
data={};names=[]
for k,f in files.items():
    name=None
    for line in open(f,encoding='utf-8',errors='replace'):
        if line.startswith('   slot'):
            m=re.search(r'slot (\d).*lane dB:((?:\s+-?[\d.]+)+)\s+\| d60 ([-+\d.]+).*g mean ([-+\d.]+) dB max ([-+\d.]+) cap (\d+)% pkl (\d+)%(?: st (\d+)%)? \| rClamps (\d+)',line)
            if m:
                key=name+' /s'+m.group(1)
                if key not in names: names.append(key)
                lv=[float(x) for x in m.group(2).split()]
                data[(k,key)]=dict(d60=float(m.group(3)),lv=lv,cl=int(m.group(9)),g=float(m.group(4)),cap=int(m.group(6)),pk=int(m.group(7)),st=int(m.group(8) or 0))
            else:
                m=re.search(r'out dB \[5,10\] ([-\d.]+) \[25,30\] ([-\d.]+) \[\d+,\d+\] ([-\d.]+) \| out rmsHz \[5,10\] (\d+) \[\d+,\d+\] (\d+)',line)
                if m: data[(k,name,'out')]=(float(m.group(1)),float(m.group(2)),float(m.group(3)),int(m.group(4)),int(m.group(5)))
        elif not line.startswith(('MK','[','presets','st ')):
            name=line.strip().split('  [')[0][:34].replace('Â','')
print('%-36s'%'min/max vs t0, d60 (clampk)'+''.join('%24s'%k for k in files))
for n in names:
    row='%-36s'%n
    for k in files:
        d=data.get((k,n))
        row+='%24s'%('%+.1f/%+.1f %+.1f (%dk)'%(min(d['lv'])-d['lv'][0],max(d['lv'])-d['lv'][0],d['d60'],d['cl']//1000) if d else '-')
    print(row)
k0=list(files)[0]
print('\n'+k0+': g mean, cap%, pkl%, st%  | out dB 5-10/25-30/55-60, rmsHz 5-10 -> 55-60')
for n in names:
    d=data[(k0,n)]; o=data.get((k0,n.split(' /s')[0],'out'))
    print('%-36s g %+5.1f cap %3d pk %3d st %3d | %6.1f %6.1f %6.1f  %5d->%5d'%(n,d['g'],d['cap'],d['pk'],d['st'],*o))
