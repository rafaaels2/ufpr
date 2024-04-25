### TABELAS
* pessoas
  * | RG | NOME | IDADE | CIDADE |
* carros
  * | CRV | NOME | MARCA | PESSOA |
* computadors
  * | NUMSERIE | MARCA | PESSOA |
* disciplinas 
  * | CODIGO | NOME |
* disciplinas_pessoas
  * | DISCIPLINA | PESSOAS |

pessoas 1: 1 carros<br>
pessoas 1: n computadors<br>
pessoas n: n disciplinas<br>

---
### COMANDOS
  os comandos são feitos atraves de alias que estão localizados no arquivo <span style="color: green;">trab.sh</span><br>
* <span style="color: pink;">**inicializa**</span>: inicializa todas as tabelas
  * COMO USAR: ```$ inicializa```
* <span style="color: pink;">**inclui**</span>: inclui um elemento na tabela
  * COMO USAR: ```$ inclui <tabela> <argumentos na ordem da tabela>```
  * EXEMPLOS:
    * ```$ inclui pessoas 2004 lari 20 bastos 0001 hb20 hyundai```
    * ```$ inclui pessoas rg="2003" nome="rafael" idade="20" cidade="curitiba" crv="0002" nome="santa fe" marca="hyundai"```
    * ```$ inclui carros 0003 lancer mitsubishi 2005 henrique 21 curitiba```
    * ```$ inclui disciplinas 1010 progweb```
    * ```$ inclui computadors 1000 microsoft 2004```
    * ```$ inclui disPes 1010 2003```
* <span style="color: pink;">**exclui**</span>: exclui um elemento da tabela
  * COMO USAR: ```$ exclui <tabela> <chave>```
  * EXEMPLOS:
    * ```$ exclui pessoas 2004```
    * ```$ exclui carros 0001```
    * ```$ exclui disciplinas 1010```
    * ```$ exclui computadors 1000```
* <span style="color: pink;">**lista**</span>: lista todos os elementos da tabela
  * COMO USAR: ```$ lista <tabela>```
  * EXEMPLOS:
    * ```$ lista pessoas```
    * ```$ lista carros```
    * ```$ lista disciplinas```
    * ```$ lista computadors```
    * ```$ lista disPes```
* <span style="color: pink;">**remove**</span>: remove as tabelas
  * COMO USAR: ```$ remove```


