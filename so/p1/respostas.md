### Respostas as perguntas do P1
---

1. Explique o objetivo e os parâmetros de cada uma das quatro funções acima.
* <span style="color: #FDFD96;">getcontext</span><br>
<strong style="color: pink">objetivo:</strong> salvar o contexto atual do processo, isso significa que valores dos registradores desta atual thread serão salvos na variável passada como parâmetro.<br>
<strong style="color: pink">parâmetros:</strong>
1. ponteiro para variável do tipo <span style="color: #90EE90;">ucontext_t</span> que irá receber as informações do contexto atual.<br><br>

* <span style="color: #FDFD96;">setcontext</span><br>
<strong style="color: pink">objetivo:</strong> restaurar o contexto passado no parâmetro, isso significa que o processo será redirecionado para o endereço que está contido no PC do contexto.<br>
<strong style="color: pink">parâmetros:</strong>
1. ponteiro para variável do tipo <span style="color: #90EE90;">ucontext_t</span> que é justamente o contexto que será restaurado.<br><br>

* <span style="color: #FDFD96;">swapcontext</span><br>
<strong style="color: pink">objetivo:</strong> realizar uma troca de contexto, armazenando as informações do contexto atual em uma variável e redirecionando o processo para o endereço que está contido no PC do contexto passado por uma outra variável.<br>
<strong style="color: pink">parâmetros:</strong>
1. ponteiro para variável do tipo <span style="color: #90EE90;">ucontext_t</span> que referência onde será armazenada as informações do contexto atual.
2. ponteiro para variável do tipo <span style="color: #90EE90;">ucontext_t</span> que referência o contexto para qual o processo será redirecionado.<br><br>

* <span style="color: #FDFD96;">makecontext</span><br>
<strong style="color: pink">objetivo:</strong> realizar algumas modificações no contexto passado, ajustando o PC deste contexto para uma função dentro do processo.<br>
<strong style="color: pink">parâmetros:</strong>
1. ponteiro para variável do tipo <span style="color: #90EE90;">ucontext_t</span> que referência o contexto que será modificado.
2. a função na qual o programa será redirecionado quando o contexto for acionado.
3. número de parâmetros que serão passados para a função.
4. parâmetros da função (se existirem).
---
2. Explique o significado dos campos da estrutura ucontext_t que foram utilizados no código.

**<span style="color:#FDFD96">context.uc_stack.ss_sp= stack:</span>** passa o endereço do topo da pilha para o contexto.<br>
**<span style="color:#FDFD96">context.uc_stack.ss_size = STACKSIZE:</span>** passa o tamanho da pilha para o contexto.<br>
**<span style="color:#FDFD96">context.uc_stack.ss_flags = 0:</span>** flags que controlam a pilha do contexto, nesse caso nenhuma foi passada.<br>
**<span style="color:#FDFD96">context.uc_link = 0:</span>** passa o endereço de um próximo contexto que será acionado assim que o contexto atual acabar, nesse caso não foi passado nenhum endereço.<br>

---
3. Explique cada linha do código de contexts.c que chame uma dessas funções ou que manipule estruturas do tipo ucontext_t

<span style="color: #90EE90"># salva as informações do contexto atual na váriavel ContextPing</span><br>
66. `getcontext (&ContextPing) ;`<br>

<span style="color: #90EE90"># modifica o valor do PC do contexto (ContextPing) para a função BodyPing, além de passar o parâmetro "    Ping" para a função BodyPing</span><br>
82. `makecontext (&ContextPing, (void*)(*BodyPing), 1, "    Ping") ;`
    
<span style="color: #90EE90"># salva as informações do contexto atual na váriavel ContextPong</span><br>
84. `getcontext (&ContextPong) ;`<br>

<span style="color: #90EE90"># modifica o valor do PC do contexto (ContextPong) para a função BodyPong, além de passar o parâmetro "    Pong" para a função BodyPong</span><br>
82. `makecontext (&ContextPong, (void*)(*BodyPong), 1, "        Pong") ;`

<span style="color: #90EE90"># realiza a troca de contexto, armazena as informações atuais na váriável ContextMain e restaura o contexto ContextPing, redirecionando para a função BodyPing</span><br>
102. `swapcontext (&ContextMain, &ContextPing) ;`

<span style="color: #90EE90"># realiza a troca de contexto, armazena as informações atuais na váriável ContextMain e restaura o contexto ContextPong, redirecionando para a função BodyPong</span><br>
103. `swapcontext (&ContextMain, &ContextPong) ;`

<span style="color: #90EE90"># realiza a troca de contexto, armazena as informações atuais na váriável ContextPing e restaura o contexto ContextPong, redirecionando para a função BodyPong</span><br>
33. `swapcontext (&ContextPing, &ContextPong) ;`

<span style="color: #90EE90"># realiza a troca de contexto, armazena as informações atuais na váriável ContextPing e restaura o contexto ContextMain, redirecionando para a próxima linha de execução já que não existe nenhuma função atrelada ao PC do contexto ContextMain</span><br>
37. `swapcontext (&ContextPing, &ContextMain) ;`

<span style="color: #90EE90"># realiza a troca de contexto, armazena as informações atuais na váriável ContextPong e restaura o contexto ContextPing, redirecionando para a função BodyPing</span><br>
51. `swapcontext (&ContextPong, &ContextPing) ;`

<span style="color: #90EE90"># realiza a troca de contexto, armazena as informações atuais na váriável ContextPong e restaura o contexto ContextMain, redirecionando para a próxima linha de execução já que não existe nenhuma função atrelada ao PC do contexto ContextMain</span><br>
55. `swapcontext (&ContextPong, &ContextMain) ;`


