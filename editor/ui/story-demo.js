(function(root){'use strict';
const M=root.LotaModel,B=root.EveBuiltins;
B.createStoryDemo=function(name="The Keeper's Errand"){
 let p=B.createDemo(name);p.id='keepers-errand-'+Math.random().toString(36).slice(2,9);
 p.quests=[{id:'keepers-errand',name:"The Keeper's Errand",description:'Open the garden, visit the grove, speak to the trader, and bring back two trail breads.',objectives:[{type:'activate',target:'meadow:garden-gate',quantity:1},{type:'visit',target:'grove',quantity:1},{type:'talk',target:'grove:merchant',quantity:1},{type:'item',target:'trail-bread',quantity:2}],rewardItem:'gold-coin',rewardQuantity:5}];
 const choice=(text,next,more={})=>({text,next,...more});
 let keeper=p.scenes[0].objects.find(o=>o.id==='keeper');keeper.dialogue={start:'greeting',nodes:[
  {id:'greeting',text:'Welcome, traveler. Would you help me gather provisions from the grove?',choices:[
   choice("I'll help.",'accepted',{action:'accept',quest:'keepers-errand',conditionQuest:'keepers-errand',conditionState:'available'}),
   choice('About the errand...','progress',{conditionQuest:'keepers-errand',conditionState:'active'}),
   choice('Thank you for the reward.','thanks',{conditionQuest:'keepers-errand',conditionState:'completed'})]},
  {id:'accepted',text:'Take the key from the chest. Open the timber door and use the lever for the garden gate. The trader and bread are in the grove. J opens your journal.',choices:[choice("I'll be back.",'')]},
  {id:'progress',text:'Have you opened the gate, visited the grove, spoken to the trader, and brought two trail breads?',choices:[choice('Here are your provisions.','reward',{action:'complete',quest:'keepers-errand',conditionQuest:'keepers-errand',conditionState:'ready'}),choice('Remind me of the route.','accepted'),choice("I'm still working on it.",'')]},
  {id:'reward',text:'Perfect! Here are five gold coins for your trouble. The grove is open for everyone now.',choices:[choice('Glad to help.','')]},
  {id:'thanks',text:'You have already earned your reward. Enjoy the garden, traveler.',choices:[choice('Goodbye.','')]}
 ]};
 let trader=p.scenes[1].objects.find(o=>o.id==='merchant');trader.dialogue={start:'hello',nodes:[{id:'hello',text:'Good to see someone from the courtyard. What brings you to the grove?',choices:[choice('Where can I find trail bread?','bread'),choice('Tell me about this place.','grove'),choice('Just passing through.','')]},{id:'bread',text:'The oak chest west of me holds two trail breads. Bring them back to the keeper to finish the errand.',choices:[choice('Thanks.','')]},{id:'grove',text:'This quiet garden belongs to everyone. The keeper keeps the gates in working order.',choices:[choice('Goodbye.','')]}]};
 let errors=M.validate(p);if(errors.length)throw Error(errors.join('\n'));return p;
};
})(typeof window!=='undefined'?window:globalThis);
