import {test, before, after} from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {PGlite} from '@electric-sql/pglite';
const db=new PGlite();
const alice='00000000-0000-4000-8000-000000000001', bob='00000000-0000-4000-8000-000000000002';
async function actor(id,role='authenticated') {
 await db.exec('reset role');
 await db.query("select set_config('request.jwt.claim.sub',$1,false)",[id||'']);
 await db.exec(`set role ${role}`);
}
before(async()=>{
 await db.exec(`create role anon;create role authenticated;create schema auth;
 create table auth.users(id uuid primary key);
 create function auth.uid() returns uuid language sql stable as $$ select nullif(current_setting('request.jwt.claim.sub',true),'')::uuid $$;
 grant usage on schema auth,public to anon,authenticated;grant execute on function auth.uid() to anon,authenticated;`);
 await db.query('insert into auth.users values($1),($2)',[alice,bob]);
 await db.exec(await readFile(new URL('../migrations/001_cloud_progress.sql',import.meta.url),'utf8'));
});
after(()=>db.close());
test('cloud data access and write rules',async t=>{
 await t.test('unauthenticated callers cannot read or mutate progress',async()=>{
  await actor(null,'anon');
  await assert.rejects(db.query('select * from public.player_progress'));
  await assert.rejects(db.query('select public.sync_progress(10,0,false)'));
 });
 await t.test('authenticated players read only their own row',async()=>{
  await actor(alice);await db.query('select public.sync_progress(40,1,false)');
  assert.equal((await db.query('select * from public.player_progress')).rows.length,1);
  await actor(bob);assert.equal((await db.query('select * from public.player_progress')).rows.length,0);
  await db.query('select public.sync_progress(20,0,false)');
  const rows=(await db.query('select * from public.player_progress')).rows;
  assert.equal(rows.length,1);assert.equal(rows[0].user_id,bob);
 });
 await t.test('clients cannot forge ownership or directly update tables',async()=>{
  await assert.rejects(db.query('update public.player_progress set best_shards=500 where user_id=$1',[alice]));
  await assert.rejects(db.query('insert into public.player_progress(user_id) values($1)',[alice]));
 });
 await t.test('negative, null and oversized progression is rejected',async()=>{
  for(const [shards,relics,done] of [[-1,0,false],[5001,0,false],[0,9,false],[null,0,false],[0,0,null]])
   await assert.rejects(db.query('select public.sync_progress($1,$2,$3)',[shards,relics,done]));
 });
 await t.test('stale updates cannot erase better cloud progress',async()=>{
  await actor(alice);await db.query('select public.sync_progress(500,8,true)');
  await db.query('select public.sync_progress(5,0,false)');
  const r=(await db.query('select * from public.player_progress')).rows[0];
  assert.equal(r.best_shards,500);assert.equal(r.best_relics,8);assert.equal(r.completed,true);
 });
 await t.test('finished runs are idempotent and private',async()=>{
  const id='00000000-0000-4000-8000-000000000010';
  for(let i=0;i<2;i++) await db.query('select public.finish_run($1,380,420,2,false)',[id]);
  assert.equal((await db.query('select * from public.game_runs')).rows.length,1);
  await actor(bob);assert.equal((await db.query('select * from public.game_runs')).rows.length,0);
 });
 await t.test('leaderboard excludes assisted runs and hides identifiers',async()=>{
  await db.query("select public.finish_run('00000000-0000-4000-8000-000000000011',200,420,2,true)");
  const rows=(await db.query('select * from public.cake_leaderboard()')).rows;
  assert.equal(rows.length,1);assert.equal(rows[0].seconds,380);
  assert.deepEqual(Object.keys(rows[0]).sort(),['nickname','relics','seconds','shards']);
 });
 await t.test('invalid runs fail atomically',async()=>{
  await assert.rejects(db.query("select public.finish_run('00000000-0000-4000-8000-000000000012',2,420,2,false)"));
  assert.equal((await db.query('select * from public.game_runs')).rows.length,1);
 });
});
