import sys
sys.argv=['x','orig.bin','fix.bin']
exec(open('cmp.py').read().split('A=load')[0])
A=load('orig.bin'); B=load('fix.bin')
def mstep(a,i0,i1): return max(abs(a[i]-a[i-1]) for i in range(i0,i1))
for k,t in [('mono_legato_60_96',12000),('mono_legato_crossing_65_67',24000),('poly_wheel_sweep_crossing',12000)]:
    for nm,D in (('orig',A),('fix',B)):
        a=D[k]; st_new=mstep(a,t+2400,t+7200); st_old=mstep(a,t-4800,t-240)
        fade=mstep(a,t-32,t+2400)
        print(f"{k:28s} {nm}: max|dy| fade {fade:.4f}  steady old {st_old:.4f} new {st_new:.4f}  ratio vs max(steady) {fade/max(st_new,st_old):.3f}")
