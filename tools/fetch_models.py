#!/usr/bin/env python3
"""Explicit network provisioning only; the C++ worker never downloads anything."""
import hashlib,json,pathlib,subprocess,os
root=pathlib.Path(__file__).resolve().parents[1]
for pack in json.loads((root/'tools/models.lock.json').read_text())['packs']:
 d=root/'models'/pack['id'];d.mkdir(parents=True,exist_ok=True)
 for f in pack['files']:
  dst=d/f['name'];tmp=dst.with_suffix('.download')
  if dst.exists() and hashlib.sha256(dst.read_bytes()).hexdigest()==f['sha256']:continue
  subprocess.run(['curl','--fail','--location','--retry','2','--connect-timeout','20','--max-time','300',f['url'],'--output',str(tmp)],check=True)
  if hashlib.sha256(tmp.read_bytes()).hexdigest()!=f['sha256']:raise RuntimeError('hash mismatch: '+f['name'])
  os.replace(tmp,dst)
 (d/'manifest.json').write_text(json.dumps(pack,indent=2)+'\n')
 print('Verified',pack['id'],flush=True)
