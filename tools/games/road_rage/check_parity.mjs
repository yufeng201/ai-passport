import {readFileSync} from 'node:fs';
import {execFileSync} from 'node:child_process';
import assert from 'node:assert/strict';
const root=new URL('../../../',import.meta.url).pathname;
const native=process.argv[2]||`${root}build/games/road_rage/test_game_plain`;
const expected=execFileSync(native,['replay'],{encoding:'utf8'}).trim().split('\n');
const {instance}=await WebAssembly.instantiate(readFileSync(`${root}build/games/road_rage/preview/game.wasm`),{});
const api=instance.exports;api.game_init(0xD057);const actual=[];
for(let i=0;i<1400;i++){
  if(i===1||i%61===0)api.game_input(2);
  if(i%37===0)api.game_input(0);
  if(i%53===0)api.game_input(1);
  if(i===120||i===131)api.game_input(3);
  if(i===900)api.game_input(4);
  api.game_tick(16+i%41);
  if(i%70===0||i===1399){
    const pointer=api.game_frame(),pixels=new Uint16Array(api.memory.buffer,pointer,320*240);
    let hash=2166136261;
    for(const p of pixels){hash=Math.imul(hash^(p&255),16777619)>>>0;hash=Math.imul(hash^(p>>8),16777619)>>>0;}
    actual.push(`${i} ${api.game_hash()>>>0} ${hash}`);
  }
}
assert.deepEqual(actual,expected);
console.log(`Native C / Wasm parity: PASS (${actual.length} state + RGB565 frame checkpoints; 1400 steps)`);
