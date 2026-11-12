import re,sys
src=open(sys.argv[1],encoding='utf-8').read()
body=src[src.index('return {'):]
# split at preset starts
starts=[m.start() for m in re.finditer(r'\{ "(?:Drones|Expressive|Keys|Live|Motion|Pads|Rhythmic|Textures) \\xc2\\xb7 ',body)]
starts.append(len(body))
G=['feedback','fb_low_cut','fb_high_cut','fb_saturation','fb_diffusion','reverb_mix','reverb_size','reverb_damp']
out=[]
for a,b in zip(starts,starts[1:]):
    blk=body[a:b]
    name=re.match(r'\{ "([^"]+)"',blk).group(1).replace('\\xc2\\xb7','·')
    vals=dict(re.findall(r'\{ "([a-z0-9_]+)", ([^}]+?) \}',blk))
    dests=re.findall(r'\{ "(mod\d+_dest)", modDest \("([a-z0-9_]+)"\) \}',blk)
    srcs={k:v for k,v in vals.items() if re.match(r'mod\d+_source',k)}
    en=[1]+[n for n in (2,3,4) if vals.get('s%d_enable'%n,'0').startswith('1')]
    if vals.get('s1_enable','1').startswith('0'): en.remove(1)
    live='true' in blk.split('\n')[-3:] .__str__() 
    out.append((name,vals,dests,en,blk))
print(len(out))
nf=sum('feedback' in v for _,v,_,_,_ in out); nr=sum('reverb_mix' in v for _,v,_,_,_ in out)
print('feedback set:',nf,'reverb_mix set:',nr)
for g in G: print(g, sum(g in v for _,v,_,_,_ in out))
for name,v,d,en,blk in out:
    g=' '.join('%s=%s'%(k.replace('fb_','fb.').replace('reverb_','rv.'),v[k].rstrip('f')) for k in G if k in v)
    gd=[x for _,x in d if x in G or x.startswith('lfo')]
    pr=[k for k in v if k.endswith('pitch_rand')]
    pl=[x for _,x in d if x.endswith('_pitch')]
    print('%-26s S%s | %s | gdest:%s | dests:%s%s%s'%(name,''.join(map(str,en)),g,','.join(gd) or '-',','.join(x for _,x in d) or '-', ' PR' if pr else '', ' PITCHROUTE' if pl else ''))
