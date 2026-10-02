const canvas=document.querySelector('canvas'),ctx=canvas.getContext('2d',{alpha:false});
const buttons=[...document.querySelectorAll('[data-key]')],loading=document.querySelector('#loading');
const img=ctx.createImageData(320,240);let api,held=null,epoch=performance.now(),paintAt=0,audio,sound=false;
const now=()=>Math.round(performance.now()-epoch)>>>0;
async function unlockAudio(){if(sound){audio??=new AudioContext();await audio.resume();}}
function cue(effect){
  if(!sound||!effect||audio?.state!=='running')return;
  const buffer=audio.createBuffer(1,3200,16000),data=buffer.getChannelData(0);
  for(let i=0;i<3200;i++)data[i]=api.game_sound_sample(effect,i)/32768;
  const source=audio.createBufferSource();source.buffer=buffer;source.connect(audio.destination);source.start();
}
function press(key,source){
  if(!api||held)return;unlockAudio();held={key,source};
  buttons[key].classList.add('held');api.game_edge(key,1,now());
}
function release(source,cancel=false){
  if(!held||held.source!==source)return;
  const key=held.key;if(cancel)api.game_cancel(now());api.game_edge(key,0,now());
  buttons[key].classList.remove('held');held=null;
}
function interrupt(){if(held)release(held.source,true);if(api)api.game_cancel(now());}
for(const button of buttons){
  const key=Number(button.dataset.key);
  button.addEventListener('pointerdown',e=>{e.preventDefault();button.setPointerCapture(e.pointerId);press(key,`p${e.pointerId}`);});
  button.addEventListener('pointerup',e=>release(`p${e.pointerId}`));
  button.addEventListener('pointercancel',e=>release(`p${e.pointerId}`,true));
  button.addEventListener('lostpointercapture',e=>release(`p${e.pointerId}`,true));
  button.addEventListener('click',e=>{if(e.detail===0){press(key,'accessible');release('accessible');}});
}
const keys={KeyA:0,ArrowLeft:0,KeyB:1,Space:1,Enter:1,KeyC:2,ArrowRight:2};
window.addEventListener('keydown',e=>{
  if(!(e.code in keys)||e.target.closest('button')&&['Enter','Space'].includes(e.code))return;
  e.preventDefault();if(!e.repeat)press(keys[e.code],`k${e.code}`);
});
window.addEventListener('keyup',e=>{if(e.code in keys){e.preventDefault();release(`k${e.code}`);}});
window.addEventListener('blur',interrupt);document.addEventListener('visibilitychange',()=>{if(document.hidden)interrupt();});
document.querySelector('#sound').addEventListener('click',async e=>{
  sound=!sound;e.currentTarget.textContent=sound?'浏览器音效开':'浏览器音效关';e.currentTarget.setAttribute('aria-pressed',String(sound));await unlockAudio();
});
function paint(){
  const ptr=api.game_frame(),pixels=new Uint16Array(api.memory.buffer,ptr,320*240);
  for(let i=0;i<pixels.length;i++){const v=pixels[i],j=i*4;img.data[j]=((v>>11)&31)*255/31;img.data[j+1]=((v>>5)&63)*255/63;img.data[j+2]=(v&31)*255/31;img.data[j+3]=255;}
  ctx.putImageData(img,0,0);
}
function loop(time){
  if(!document.hidden&&time-paintAt>=1000/30){paintAt=time;api.game_tick(now());cue(api.game_effect());paint();}
  streamMusic();requestAnimationFrame(loop);
}
try{
  const response=await fetch('game.wasm');if(!response.ok)throw Error(`HTTP ${response.status}`);
  const {instance}=await WebAssembly.instantiate(await response.arrayBuffer(),{});api=instance.exports;
  epoch=performance.now();api.game_init(0xC10D);loading.remove();paint();requestAnimationFrame(loop);
}catch(error){loading.textContent=`加载失败：${error.message}。请通过本地 HTTP 服务打开。`;}

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
