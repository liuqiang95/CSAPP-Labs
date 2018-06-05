  r13 = rsi = rsp;
  read_six_numbers();// rsp + 4*i, int a[6]
  rsp = &a[0]
  r14 = rsp; // r13 = rsi = r14 = &a[0]
  r12d = 0;

  do{
    rbp = r13;
    eax = (*r13) - 1;
    if(eax>5)bomb();
    r12d ++;
    if(6 == r12d) break;
    ebx = r12d
    do{
      rax = ebx
      eax = *(rsp + 4*rax)
      eax != *rbp(else bomb)
      ebx ++      
    }while(ebx<=5)
    r13 += 4
  }while(true)
// 1 <= a[i] <= 6, a[i] != *rbp

  rsi = rsp + 24
  rax = r14
  ecx = 7
  do{
    edx = ecx
    edx -= *rax
    *rax = edx
    rax += 4
  }while(rsi != rax)
// a[i] = 7 - a[i], a： rsp ~ rsp+24
  
// back a[6]: 3 4 5 6 1 2
// so: 4 3 2 1 6 5

  esi = 0
  do{
    ecx = *(rsp + rsi)
    if(ecx<=1){
      edx = $0x6032d0
    }else{
      eax = 1
      edx = $0x6032d0
      do{
        rdx = *(rdx + 8)
        eax ++
      }while(eax != ecx)
    }
    *(rsp + 2*rsi + 32) = rdx
    rsi += 4
  }while(24 != rsi)
//ecx = a[i], rsp + 32 + 8*i = node[a[i]]
/* node 
(gdb) x/12 0x6032d0
0x6032d0 <node1>: 0x10000014c 0x6032e0 <node2>
0x6032e0 <node2>: 0x2000000a8 0x6032f0 <node3>
0x6032f0 <node3>: 0x30000039c 0x603300 <node4>
0x603300 <node4>: 0x4000002b3 0x603310 <node5>
0x603310 <node5>: 0x5000001dd 0x603320 <node6>
0x603320 <node6>: 0x6000001bb 0x0

(gdb) x/24wd 0x6032d0
0x6032d0 <node1>: 332 1 6304480 0
0x6032e0 <node2>: 168 2 6304496 0
0x6032f0 <node3>: 924 3 6304512 0
0x603300 <node4>: 691 4 6304528 0
0x603310 <node5>: 477 5 6304544 0
0x603320 <node6>: 443 6 0 0

node: int, int, node *
node[6]

*/
// sort node[6] accordding to val1 by a[6]
// so a[6]: 3 4 5 6 1 2



  rbx = *(rsp + 32) // node:0
  rax = rsp + 40 //&node:1
  rsi = rsp + 80 //boundary
  rcx = rbx
  do{
    rdx = *rax //node:1
    *(rcx + 8) = rdx // node:0 -> next = node:1
    rax += 8 //&node:2
    if(rsi == rax) break;
    rcx = rdx //node:1
  }while(true)
  // node:0 -> next = node:1 ,...node:4 -> next = node:5
  *(rdx + 8) = 0 //node:5 -> next = NULL
  ebp = 5 
  do{
    rax = *(rbx + 8)// node:1
    eax = *rax // val1:1
    *rbx >= eax (else bomb) // val1:0 >= val1:1
    rbx = *(rbx + 8) // node:2
    ebp --;
  }while(ebp != 0) //此处返回 eax

// sort node[6] accordding to val1 by a[6]
// 

  





    
