import {readFileSync} from 'node:fs';
import {execFileSync} from 'node:child_process';
import assert from 'node:assert/strict';
const root=new URL('../../../',import.meta.url).pathname;
const expected=execFileSync(`${root}build/games/alley_ninja/test_game_plain`,['replay'],{encoding:'utf8'}).trim().split('\n');
const {instance}=await WebAssembly.instantiate(readFileSync(`${root}build/games/alley_ninja/preview/game.wasm`),{});
const api=instance.exports,actual=[];api.game_init(0xA11E);
for(let i=0;i<2000;i++){
  const now=i*20;
  if(i%90===1)api.game_edge(1,1,now);
  if(i%90===40)api.game_edge(1,0,now);
  if(i===500)api.game_edge(2,1,now);
  if(i===545)api.game_edge(2,0,now);
  if(i===1000)api.game_cancel(now);
  api.game_tick(now);
  if(i%100===0||i===1999){
    const ptr=api.game_frame(),pixels=new Uint16Array(api.memory.buffer,ptr,320*240);let h=2166136261;
    for(const p of pixels){h=Math.imul(h^(p&255),16777619)>>>0;h=Math.imul(h^(p>>8),16777619)>>>0;}
    actual.push(`${i} ${api.game_hash()>>>0} ${h}`);
  }
}
assert.deepEqual(actual,expected);console.log(`Alley Ninja native/Wasm: PASS (${actual.length} state + RGB565 frame checkpoints)`);
