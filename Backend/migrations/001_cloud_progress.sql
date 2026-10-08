begin;
-- No service-role key belongs in the game. Every operation uses the player's Auth JWT.
create table public.player_progress (
 user_id uuid primary key references auth.users(id) on delete cascade,
 best_shards integer not null default 0 check (best_shards between 0 and 5000),
 best_relics integer not null default 0 check (best_relics between 0 and 8),
 completed boolean not null default false,
 updated_at timestamptz not null default now()
);
create table public.game_runs (
 user_id uuid not null references auth.users(id) on delete cascade,
 run_id uuid not null,
 seconds integer not null check (seconds between 30 and 86400),
 shards integer not null check (shards between 360 and 5000),
 relics integer not null check (relics between 0 and 8),
 assisted boolean not null default false,
 finished_at timestamptz not null default now(),
 primary key(user_id,run_id)
);
create index game_runs_ranking on public.game_runs(seconds) where not assisted;
alter table public.player_progress enable row level security;
alter table public.game_runs enable row level security;
revoke all on public.player_progress,public.game_runs from anon,authenticated;
grant select on public.player_progress,public.game_runs to authenticated;
create policy own_progress on public.player_progress for select to authenticated using ((select auth.uid())=user_id);
create policy own_runs on public.game_runs for select to authenticated using ((select auth.uid())=user_id);
-- Mutations are bounded and derive identity from the verified JWT, never a request's user_id.
create function public.sync_progress(p_shards integer,p_relics integer,p_completed boolean)
returns public.player_progress language plpgsql security definer set search_path='' as $$
declare result public.player_progress;
begin
 if auth.uid() is null then raise exception 'Sign in required' using errcode='42501'; end if;
 if p_shards is null or p_relics is null or p_completed is null or p_shards not between 0 and 5000 or p_relics not between 0 and 8 then
  raise exception 'Invalid progress' using errcode='22023';
 end if;
 insert into public.player_progress(user_id,best_shards,best_relics,completed)
 values(auth.uid(),p_shards,p_relics,p_completed)
 on conflict(user_id) do update set
 best_shards=greatest(player_progress.best_shards,excluded.best_shards),
 best_relics=greatest(player_progress.best_relics,excluded.best_relics),
 completed=player_progress.completed or excluded.completed,updated_at=now()
 returning * into result;
 return result;
end $$;
create function public.finish_run(p_run_id uuid,p_seconds integer,p_shards integer,p_relics integer,p_assisted boolean)
returns void language plpgsql security definer set search_path='' as $$
begin
 if auth.uid() is null then raise exception 'Sign in required' using errcode='42501'; end if;
 -- Serialize a player's submissions to enforce the cap under concurrent requests.
 perform pg_advisory_xact_lock(hashtextextended(auth.uid()::text,0));
 if exists(select 1 from public.game_runs where user_id=auth.uid() and run_id=p_run_id) then return; end if;
 if (select count(*) from public.game_runs where user_id=auth.uid() and finished_at>now()-interval '1 day')>=100 then
  raise exception 'Daily run limit reached' using errcode='54000';
 end if;
 insert into public.game_runs(user_id,run_id,seconds,shards,relics,assisted)
 values(auth.uid(),p_run_id,p_seconds,p_shards,p_relics,p_assisted);
 perform public.sync_progress(p_shards,p_relics,true);
 -- Keep the newest 100 runs per player; best totals remain in player_progress.
 delete from public.game_runs where user_id=auth.uid() and run_id in
  (select run_id from public.game_runs where user_id=auth.uid() order by finished_at desc,run_id offset 100);
end $$;
-- Only a generated nickname and score are published. UUIDs and private progress stay private.
-- These are client-reported, casual results, not server-authoritative competitive scores.
create function public.cake_leaderboard()
returns table(nickname text,seconds integer,shards integer,relics integer)
language sql stable security definer set search_path='' as $$
 select 'Ninja '||left(md5(best.user_id::text),6),best.seconds,best.shards,best.relics
 from (select distinct on(user_id) user_id,seconds,shards,relics from public.game_runs
       where not assisted order by user_id,seconds,finished_at) best
 order by best.seconds,best.shards desc limit 20
$$;
revoke all on function public.sync_progress(integer,integer,boolean),public.finish_run(uuid,integer,integer,integer,boolean),public.cake_leaderboard() from public,anon;
grant execute on function public.sync_progress(integer,integer,boolean),public.finish_run(uuid,integer,integer,integer,boolean),public.cake_leaderboard() to authenticated;
commit;
