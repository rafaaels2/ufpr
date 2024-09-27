//Gustavo do Prado Silva - 20203942 
//Rafael Gonçalves dos Santos - 20211798

Para rodar os testes basta rodar ./rodarTestes.sh
   * O caminho do Likwid foi configurado como a variavel ${LIKWID_HOME} nos Makefiles
   * É rodada a versão com pré-condicionador (p = 1)
   * Talvez seja necessario mudar as permissoes de um arquivo para o clock da cpu ser mudado,
   mas tem uma mensagem de erro correspondente
   * Ambos são compilados com as flags  -O3 -march=native -mavx

Os arquivos com os dados para plot estarão em ./saida/dados_METRICA-VERSAO.dat
Os arquivos com resultado do Likwid estarao em ./saida/METRICA_TAMANHO_VERSAO.txt
Os arquivos para plot da sua saida ao rodar estarao em ./saida/plotXXX.gp, onde XXX é a métrica desejada
Os arquivos de resultados finais, obtidos no PC h45 do Dinf estao em ./resultDinf, e tambem podem
ser plotados com ./resultDinf/plotXXX.gp, mas possuem versoes em png no mesmo diretorio
A arquitetura da maquina utilizada foi colocada no relatorio, mas tambem esta disponivel em topologia.txt
