import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';
const html=fs.readFileSync('web/installer.html','utf8');
const script=html.match(/<script type="module">([\s\S]*?)<\/script>/)[1];
new vm.Script('(async()=>{'+script+'})()');
const monitor=script.slice(script.indexOf('const byId='),script.indexOf('try{\n if(!navigator.serial'));
const nodes=new Map();
const element=id=>{if(!nodes.has(id))nodes.set(id,{checked:true,disabled:false,textContent:'',click(){}});return nodes.get(id);};
let active=false, released=false, closed=false, cancel;
const parts=[new TextEncoder().encode('MNP: zażółć\n')];
const reader={
 async read(){if(parts.length){const value=parts.shift();parts.push(value.slice(7));return {value:value.slice(0,7),done:false};}return new Promise(resolve=>{cancel=()=>resolve({done:true});});},
 async cancel(){cancel?.();},
 releaseLock(){released=true;}
};
// Use a finite stream with a split UTF-8 character.
const bytes=new TextEncoder().encode('MNP: zażółć\n');
const split=bytes.indexOf(0xc5)+1;
let i=0;reader.read=async()=> i++===0?{value:bytes.slice(0,split)}:i===2?{value:bytes.slice(split)}:new Promise(resolve=>{cancel=()=>resolve({done:true});});
const port={async open(opts){assert.equal(opts.baudRate,115200);active=true;},readable:{getReader:()=>reader},async close(){closed=true;active=false;}};
const ctx=vm.createContext({document:{getElementById:element,createElement:()=>element('download')},navigator:{serial:{requestPort:async()=>port}},window:{isSecureContext:true},go:element('go'),ack:element('ack'),running:false,firmwareReady:true,TextDecoder,Date,Blob,URL,setTimeout,assert});
vm.runInContext(monitor+'\nglobalThis.capture={record,renderLog,getState:()=>({logSize,discarded,monitorState,chunks}),buttons};',ctx);
const promise=element('monitorStart').onclick();
await new Promise(resolve=>setTimeout(resolve,10));
assert(active);assert(element('go').disabled);
await element('monitorStop').onclick();await promise;
assert(released&&closed&&!active);
assert(ctx.capture.getState().chunks.join('').includes('zażółć'));
assert.equal(ctx.capture.getState().monitorState,'idle');
assert.equal(element('go').disabled,false);
ctx.capture.record('x'.repeat(5*1024*1024));
assert.equal(ctx.capture.getState().logSize,4*1024*1024);
assert(ctx.capture.getState().discarded>0);
ctx.capture.renderLog();assert(element('monitorOutput').textContent.includes('Podgląd końca'));
element('monitorClear').onclick();assert.equal(ctx.capture.getState().logSize,0);
console.log('Web monitor: script syntax, UTF-8, stop/close, flash exclusion, bounded buffer passed');
