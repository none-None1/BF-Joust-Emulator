function detect(code){
    var u=code.indexOf('('),v=code.indexOf('{');
    if(u==-1&&v==-1){
        return {type:-1};
    }
    if(u==-1)u=1000000000000;
    if(v==-1)v=1000000000000;
    if(u<v){
        var k=code.indexOf('('),r=0,p=1;
        for(let i=k+1;i<code.length;i++){
            if(code[i]=='('){
                p++;
            }else if(code[i]==')'){
                p--;
                if(!p){
                    r=i;
                    break;
                }
            }
        }
        return {type:0,l:code.slice(0,k),r:code.slice(r+1,code.length),m:code.slice(k+1,r)};
    }else{
        var k=code.indexOf('{'),r=0,p=1;
        for(let i=k+1;i<code.length;i++){
            if(code[i]=='{'){
                p++;
            }else if(code[i]=='}'){
                p--;
                if(!p){
                    r=i;
                    break;
                }
            }
        }
        return {type:1,l:code.slice(0,k),r:code.slice(r+1,code.length),m:code.slice(k+1,r)};
    }
}
function parsetree(code){
    var tree=[];
    while(1){
        var k=detect(code);
        if(k.type==-1) break;
        tree.push(k.l);
        tree.push({type:k.type,tree:parsetree(k.m)});
        code=k.r;
    }
    tree.push(code);
    return tree;
}
function walk_(x,t,fa,fafa,pos){
    var id=0;
    for(let i of x){
        if((typeof i)!=="string"&&!i.type){
            return walk_(i.tree,x[id+1],i,x,id);
        }
        id++;
    }
    //console.log(fa);
    //console.log(fafa);
    return {tree:x,type:(t[0]=='%')+0,num:parseInt(t.slice(1)),fa:fa,fafa:fafa,pos:pos};
}
function walk(x){
    return walk_(x,' ',{},{},0);
}
function reduce(x){
    var result=[];
    if(!x.type){
        for(let i=0;i<x.num;i++){result=result.concat(JSON.parse(JSON.stringify(x.tree)))}
    }else{
        var pos=0;
        for(let i=0;i<x.tree.length;i++){
            if((typeof x.tree[i])!=="string"&&x.tree[i].type==1){
                pos=i;
                break;
            }
        }
        for(let i=0;i<x.num;i++){result=result.concat(JSON.parse(JSON.stringify(x.tree.slice(0,pos))))}
        result=result.concat(JSON.parse(JSON.stringify(x.tree[pos].tree)));
        for(let i=0;i<x.num;i++){result=result.concat(JSON.parse(JSON.stringify(x.tree.slice(pos+1))))}
    }
    var temp=[];
    while(x.fafa.length!=x.pos){temp.push(x.fafa.pop())}
    for(let i of result){x.fafa.push(i)}
    temp.pop();
    while(temp.length){x.fafa.push(temp.pop())}
}
