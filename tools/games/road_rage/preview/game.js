const canvas=document.querySelector('canvas'),ctx=canvas.getContext('2d',{alpha:false});
const loading=document.querySelector('#loading'),buttons=[...document.querySelectorAll('[data-key]')];
let api,held=null,last=0,paintAt=0,audio=null,sound=false,priorHealth=100,priorAttack=0,priorPhase=0,priorBoost=0;
const img=ctx.createImageData(320,240);
function cue(effect){
  if(!sound||!audio||audio.state!=='running')return;
  const buffer=audio.createBuffer(1,3200,16000),data=buffer.getChannelData(0);
  for(let i=0;i<3200;i++)data[i]=api.game_sound_sample(effect,i)/32768;
  const source=audio.createBufferSource();source.buffer=buffer;source.connect(audio.destination);source.start();
}
async function unlockAudio(){if(sound){audio??=new AudioContext();await audio.resume();}}
document.querySelector('#sound').addEventListener('click',async event=>{
  sound=!sound;event.currentTarget.textContent=sound?'音效开':'音效关';event.currentTarget.setAttribute('aria-pressed',String(sound));
  event.currentTarget.setAttribute('aria-label',sound?'关闭音效':'开启音效');await unlockAudio();
});
function press(key,source){
  if(!api||held)return;
  unlockAudio();
  held={key,source,long:false,timer:0};buttons.find(b=>Number(b.dataset.key)===key).classList.add('held');
  if(key<2)api.game_input(key); // PRESS for A/C lane changes; B CLICK avoids attacks on long pause.
  held.timer=setTimeout(()=>{
    if(!held||held.source!==source)return;
    held.long=true;
    if(key===2)api.game_input(3);
    else if(key===0&&api.game_phase()!==1)api.game_input(4);
  },500);
}
function release(source,cancel=false){
  if(!held||held.source!==source)return;
  const key=held.key;clearTimeout(held.timer);buttons.find(b=>Number(b.dataset.key)===key).classList.remove('held');
  if(!cancel&&!held.long&&key===2)api.game_input(2);
  held=null;
}
function interrupt(){
  if(held)release(held.source,true);
  if(api?.game_phase()===1)api.game_input(3);
  last=0;paintAt=0;
}
for(const button of buttons){
  const key=Number(button.dataset.key);
  button.addEventListener('click',event=>{if(event.detail===0){press(key,'accessible');release('accessible');}});
  button.addEventListener('pointerdown',event=>{event.preventDefault();button.setPointerCapture(event.pointerId);press(key,`p${event.pointerId}`);});
  button.addEventListener('pointerup',event=>release(`p${event.pointerId}`));
  button.addEventListener('pointercancel',event=>release(`p${event.pointerId}`,true));
  button.addEventListener('lostpointercapture',event=>release(`p${event.pointerId}`,true));
}
const keys={ArrowLeft:0,KeyA:0,ArrowRight:1,KeyC:1,KeyD:1,KeyB:2,Space:2,Enter:2};
window.addEventListener('keydown',event=>{
  if(!(event.code in keys)||event.target.closest('#sound')||event.target.closest('[data-key]')&&['Enter','Space'].includes(event.code))return;
  event.preventDefault();if(!event.repeat)press(keys[event.code],`k${event.code}`);
});
window.addEventListener('keyup',event=>{if(event.code in keys){event.preventDefault();release(`k${event.code}`);}});
window.addEventListener('blur',interrupt);document.addEventListener('visibilitychange',()=>{if(document.hidden)interrupt();});
function paint(){
  const ptr=api.game_frame(),pixels=new Uint16Array(api.memory.buffer,ptr,320*240);
  for(let i=0;i<pixels.length;i++){
    const v=pixels[i],j=i*4;
    img.data[j]=((v>>11)&31)*255/31;img.data[j+1]=((v>>5)&63)*255/63;img.data[j+2]=(v&31)*255/31;img.data[j+3]=255;
  }
  ctx.putImageData(img,0,0);
}
function loop(time){
  if(!document.hidden&&time-paintAt>=1000/30){
    const delta=last?Math.min(250,Math.round(time-last)):0;last=time;paintAt=time;api.game_tick(delta);
    const hp=api.game_health(),attack=api.game_attack(),phase=api.game_phase(),boost=api.game_boost();
    if(hp<priorHealth)cue(2);else if(boost>priorBoost)cue(7);else if(attack>priorAttack)cue(1);else if(phase===1&&priorPhase===0||phase===3&&priorPhase===1)cue(3);
    priorHealth=hp;priorAttack=attack;priorPhase=phase;priorBoost=boost;paint();
  }
  streamMusic();requestAnimationFrame(loop);
}
try{
  const response=await fetch('game.wasm');if(!response.ok)throw Error(`HTTP ${response.status}`);
  const {instance}=await WebAssembly.instantiate(await response.arrayBuffer(),{});api=instance.exports;
  api.game_init(0xD057);loading.remove();paint();requestAnimationFrame(loop);
}catch(error){loading.textContent=`加载失败：${error.message}。请通过本地 HTTP 服务打开。`;console.error(error);}

let musicAt=0,musicCursor=0,musicNodes=[];
function streamMusic(){
  const theme=api?.game_music_theme();
  if(!sound||audio?.state!=='running'||!theme||document.hidden){
    for(const node of musicNodes){try{node.stop();}catch{}}musicNodes=[];musicAt=0;return;
  }
  musicNodes=musicNodes.filter(n=>n.endAt>audio.currentTime);
  if(musicAt<audio.currentTime)musicAt=audio.currentTime;
  for(let count=0;count<4&&musicAt<audio.currentTime+0.12;count++){
    const buffer=audio.createBuffer(1,1280,16000),data=buffer.getChannelData(0);
    for(let i=0;i<1280;i++)data[i]=api.game_music(theme,musicCursor++>>>0)/32768;
    const node=audio.createBufferSource();node.buffer=buffer;node.connect(audio.destination);
    node.start(musicAt);musicAt+=0.08;node.endAt=musicAt;musicNodes.push(node);
  }
}
