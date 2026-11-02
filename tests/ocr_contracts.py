#!/usr/bin/env python3
"""Native worker CLI contracts; Python is a TEST driver, never the product OCR runtime."""
from pathlib import Path
import hashlib,json,subprocess,tempfile,shutil
root=Path(__file__).resolve().parents[1]; exe=root/'build/crossshot-ocr'
image=root/'artifacts/fixture.png'; results=[]
def run(name,args,expected):
 p=subprocess.run([str(exe),*args],capture_output=True,text=True,timeout=30)
 result=json.loads(p.stdout); assert p.returncode==expected,(name,p.returncode,p.stderr)
 assert result['status']==('completed' if expected==0 else 'failed'),(name,result)
 results.append({'name':name,'passed':True});return result
run('missing required arguments',[],1)
run('bad repeat',['--repeat','99'],1)
run('invalid image',['--image',str(root/'CMakeLists.txt'),'--models',str(root/'models/ppocr-v5-mobile')],1)
run('missing model manifest',['--image',str(image),'--models',str(root/'artifacts/nonexistent-model')],1)
with tempfile.TemporaryDirectory() as temp:
 d=Path(temp);(d/'det.onnx').write_bytes(b'corrupted')
 manifest={'id':'corrupted','files':[{'name':'det.onnx','sha256':'0'*64},{'name':'rec.onnx','sha256':'0'*64}]}
 (d/'manifest.json').write_text(json.dumps(manifest))
 j=run('reject tampered model',['--image',str(image),'--models',str(d)],1)
 assert 'hash mismatch' in j['error']
 manifest['files'][0]['name']='../escape.onnx';(d/'manifest.json').write_text(json.dumps(manifest))
 j=run('reject manifest traversal',['--image',str(image),'--models',str(d)],1)
 assert 'unexpected model file' in j['error']
for pack in ['ppocr-v4-mobile','ppocr-v5-mobile']:
 j=run(pack+' synthetic recognition',['--image',str(image),'--models',str(root/'models'/pack)],0)
 assert 'Invoice 12345.67 ABC xyz' in j['text'] and 'CrossShot C++20 Qt 6' in j['text'],j
 assert len(j['lines'])==3 and j['modelPack']==pack
report={'status':'passed','checks':results,'testInput':'synthetic fixture only','liveNetworkRequired':False}
(root/'artifacts/ocr-contracts.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
