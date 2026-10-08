#!/usr/bin/env python3
"""Exercise real cloud API isolation with two disposable profiles. Never logs credentials."""
import configparser, json, os, time, uuid
from pathlib import Path
from urllib.request import Request, urlopen
from urllib.error import HTTPError
ROOT=Path(__file__).resolve().parents[2]
config=configparser.ConfigParser();config.read(ROOT/'Config/Cloud.ini')
url=config.get('PieceOfCake.Cloud','URL').strip('"');key=config.get('PieceOfCake.Cloud','PublishableKey')
if not url.startswith('https://') or not url.endswith('.supabase.co'):raise SystemExit('Expected the configured Supabase HTTPS project')
def call(path,body=None,token=None,method=None):
 headers={'apikey':key,'Content-Type':'application/json'}
 if token:headers['Authorization']='Bearer '+token
 request=Request(url+path,data=None if body is None else json.dumps(body).encode(),headers=headers,method=method or ('GET' if body is None else 'POST'))
 try:
  with urlopen(request,timeout=12) as response:raw=response.read();return response.status,json.loads(raw) if raw else None
 except HTTPError as error:
  return error.code,json.loads(error.read())
def one(value):return value[0] if isinstance(value,list) else value
profiles=[];checks=[]
for _ in range(2):
 status,data=call('/auth/v1/signup',{})
 if status not in (200,201) or not data.get('access_token'):raise SystemExit(f'Anonymous authentication unavailable (HTTP {status}); no deployment validation claimed.')
 profiles.append(data)
runtime=ROOT/'Backend/runtime';runtime.mkdir(exist_ok=True);os.chmod(runtime,0o700)
private=runtime/f'live-test-profiles-{int(time.time())}.json'
with os.fdopen(os.open(private,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600),'w') as f:json.dump(profiles,f)
a,b=profiles;ta,tb=a['access_token'],b['access_token']
def check(name,ok):
 if not ok:raise AssertionError(name)
 checks.append(name)
check('unauthenticated read denied',call('/rest/v1/player_progress?select=*')[0] in (401,403))
status,data=call('/rest/v1/rpc/sync_progress',{'p_shards':420,'p_relics':2,'p_completed':True},ta)
check('cloud progress write',status==200 and one(data)['best_shards']==420)
status,data=call('/rest/v1/player_progress?select=*',token=tb)
check('second profile cannot read first profile',status==200 and data==[])
status,_=call('/rest/v1/player_progress',{'user_id':a['user']['id'],'best_shards':999},tb)
check('forged ownership write denied',status in (401,403))
status,_=call('/rest/v1/rpc/sync_progress',{'p_shards':-1,'p_relics':0,'p_completed':False},ta)
check('invalid progress rejected',400<=status<500)
status,data=call('/rest/v1/rpc/sync_progress',{'p_shards':1,'p_relics':0,'p_completed':False},ta)
check('stale update preserves cloud best',status==200 and one(data)['best_shards']==420 and one(data)['completed'])
run={'p_run_id':str(uuid.uuid4()),'p_seconds':381,'p_shards':420,'p_relics':2,'p_assisted':True}
for _ in range(2):check('assisted run submission accepted',call('/rest/v1/rpc/finish_run',run,ta)[0] in (200,204))
status,data=call('/rest/v1/game_runs?select=*',token=ta)
check('retry stores one run',status==200 and len(data)==1)
status,data=call('/rest/v1/game_runs?select=*',token=tb)
check('run history is private',status==200 and data==[])
status,data=call('/rest/v1/rpc/cake_leaderboard',{},ta)
check('public score projection contains no identifiers',status==200 and all(set(x)=={'nickname','seconds','shards','relics'} for x in data))
status,fresh=call('/auth/v1/token?grant_type=refresh_token',{'refresh_token':a['refresh_token']})
check('session refresh works',status==200 and bool(fresh.get('access_token')))
status,data=call('/rest/v1/player_progress?select=*',token=fresh['access_token'])
check('progress persists across renewed session',status==200 and one(data)['best_shards']==420)
profiles[0]=fresh
with private.open('w') as f:json.dump(profiles,f)
report={'passed':True,'checks':checks,'provider':'Supabase Free','scope':'real HTTPS Auth and Postgres API; disposable assisted test profiles','timestamp':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())}
(ROOT/'Artifacts/cloud-api-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
