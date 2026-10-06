import pedalboard
p = pedalboard.load_plugin("/Users/taylorbrook/Library/Audio/Plug-Ins/VST3/O-simpleWavetable-dev.vst3")
for k,v in p.parameters.items():
    print(k, "|", v.raw_value, "|", getattr(v,'string_value',None), "|", v.num_steps if hasattr(v,'num_steps') else '')
print(p.is_instrument)
