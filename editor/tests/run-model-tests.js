const fs=require('node:fs'),os=require('node:os'),path=require('node:path'),{spawnSync}=require('node:child_process');
const folder=fs.mkdtempSync(path.join(os.tmpdir(),'evengine-model-tests-'));
try{
 const fixture=path.join(folder,'audio.json');fs.writeFileSync(fixture,JSON.stringify(JSON.parse(fs.readFileSync(path.join(__dirname,'../examples/Mosslight.eve'))).audio));
 for(const [script,args] of [['model.test.js',[]],['features.test.js',[fixture]],['builtins.test.js',[]],['story.test.js',[]],['appearance.test.js',[]],['rc.test.js',[]],['adventure.test.js',[]]]){
  const result=spawnSync(process.execPath,[path.join(__dirname,script),...args],{encoding:'utf8'});process.stdout.write(result.stdout||'');process.stderr.write(result.stderr||'');if(result.error)throw result.error;if(result.status!==0){process.exitCode=1;break;}
 }
}finally{fs.rmSync(folder,{recursive:true,force:true});}
