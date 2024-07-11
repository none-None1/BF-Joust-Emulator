importScripts('./run.js')
self.onmessage=function(event){
    d=event.data;
    z={i:d.i,j:d.j};
    score=0;
    for(let i=10;i<=30;i++){
        z[i]=bfjoust(d.x,d.y,i,0);
        if(z[i].state=='X') score--;
        if(z[i].state=='Y') score++;
    }
    for(let i=10;i<=30;i++){
        z[-i]=bfjoust(d.x,d.y,i,1);
        if(z[-i].state=='X') score--;
        if(z[-i].state=='Y') score++;
    }
    z.score=score;
    self.postMessage(z);
}