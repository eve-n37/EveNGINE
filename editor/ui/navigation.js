'use strict';
// Keep existing workspace views so inspector shortcuts and project editing stay intact.
const workspaceGroups=[
 {id:'maps',name:'Maps',views:[['map','Map editor'],['world','World overview'],['autotiles','Terrain & groups']]},
 {id:'artwork',name:'Artwork',views:[['assets','Project assets'],['library','Built-in library'],['pixels','Pixel editor'],['animation','Animation']]},
 {id:'story',name:'Story',views:[['dialogue','Dialogue'],['quests','Quests'],['items','Items'],['events','Events & flags']]},
 {id:'audio',name:'Audio',views:[['audio','Audio']]},
 {id:'game',name:'Game',views:[['appearance','Appearance'],['menus','Menus & controls']]}
];
const lastWorkspaceView=new Map(workspaceGroups.map(group=>[group.id,group.views[0][0]]));
function selectWorkspaceView(next){view=next;if(view==='pixels'&&selected?.type!=='tile')selected={type:'sprite'};render();}
function renderWorkspaceNavigation(){
 const group=workspaceGroups.find(group=>group.views.some(([id])=>id===view));
 if(!group)return;
 lastWorkspaceView.set(group.id,view);
 document.body.dataset.workspace=group.id;
 for(const button of document.querySelectorAll('#primaryNavigation [data-workspace]')){
  const active=button.dataset.workspace===group.id;
  button.classList.toggle('active',active);
  if(active)button.setAttribute('aria-current','page');else button.removeAttribute('aria-current');
 }
 const sections=$('workspaceSections');sections.hidden=group.views.length===1;
 sections.setAttribute('aria-label',group.name+' tools');
 // Preserve focused controls when edits rerender the active workspace.
 if(sections.dataset.group!==group.id){
  sections.dataset.group=group.id;
  sections.replaceChildren(...group.views.map(([id,label])=>{
   const button=document.createElement('button');button.className='sectionTab';button.dataset.section=id;button.textContent=label;
   button.onclick=()=>selectWorkspaceView(id);return button;
  }));
 }
 for(const button of sections.querySelectorAll('button')){
  const active=button.dataset.section===view;button.classList.toggle('active',active);
  if(active)button.setAttribute('aria-current','page');else button.removeAttribute('aria-current');
 }
}
for(const button of document.querySelectorAll('#primaryNavigation [data-workspace]'))button.onclick=()=>selectWorkspaceView(lastWorkspaceView.get(button.dataset.workspace));
const navigationRender=render;
render=function(){navigationRender();renderWorkspaceNavigation();};
render();
