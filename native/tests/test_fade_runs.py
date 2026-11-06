"""The run detector must catch old ink of any index, and reject real edges."""
from pathlib import Path
import importlib.util

spec=importlib.util.spec_from_file_location('compare',Path(__file__).with_name('compare_native.py'))
compare=importlib.util.module_from_spec(spec);spec.loader.exec_module(compare)
w=h=12
classic=bytes([9])*(w*h)
for colour in (0,4):
    for axis in ('horizontal','vertical'):
        native=bytearray(classic)
        for n in range(2,10):
            x,y=(n,4) if axis=='horizontal' else (4,n)
            native[y*w+x]=colour
        found=compare.thin_runs(native,classic,w,h,(0,0,w,h))
        assert found[axis]['max']==8,found
        assert found[axis]['longest']==([2,4,8] if axis=='horizontal' else [4,2,8]),found
        same=compare.thin_runs(native,native,w,h,(0,0,w,h))
        assert not same['horizontal']['max'] and not same['vertical']['max'],same
edge=bytes([9])*(w*h//2)+bytes([4])*(w*h//2)
found=compare.thin_runs(edge,classic,w,h,(0,0,w,h))
assert not found['horizontal']['max'] and not found['vertical']['max'],found
print('Detected horizontal/vertical zero and old-colour slits; rejected shared runs and ordinary edges')
