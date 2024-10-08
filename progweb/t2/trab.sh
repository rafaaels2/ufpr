#!/bin/bash

##########################
# SEM TRATAMENTO DE ERRO #
##########################

# inicializa as tabelas
function inicializa () {
    ruby Carros/criaCarros.rb
    ruby Pessoas/criaPessoas.rb
    ruby Computadors/criaComputadors.rb
    ruby Disciplinas/criaDisciplinas.rb
    ruby Relacionais/criaDisciplinasPessoas.rb

    echo "# Tabelas Inicializadas !"
}

# exclui as tabelas
function remove () {
    rm -rf *.sqlite3*

    echo "# Tabelas Excluídas !"
}

# verifica se foram passados argumentos suficientes
function validarEntrada () {
    if [ $# -lt 1 ]; then
        return 0
    fi

    return 1
}

# inclui um elemento na tabela
# metodos para inserção -> rafael, "rafael", nome=rafael, nome="rafael"
# se passar mais parametres que o necessario, não serao contabilzados os demais
function inclui () {
    # pega o numero de parametros passados
    validarEntrada "$@"
    verificacao=$?

    # aborta o programa
    if [ $verificacao -eq 0 ]; then
        echo "# Paramêtros inconsistêntes" >&2
        return 1
    fi

    # realiza a inclusao
    if [ "$1" = "pessoas" ]; then
        rg=$(echo "$2" | cut -d'=' -f2)
        nomePessoa=$(echo "$3" | cut -d'=' -f2)
        idade=$(echo "$4" | cut -d'=' -f2)
        cidade=$(echo "$5" | cut -d'=' -f2)
        crv=$(echo "$6" | cut -d'=' -f2)
        nomeCarro=$(echo "$7" | cut -d'=' -f2)
        marca=$(echo "$8" | cut -d'=' -f2)

        ruby Relacionais/incluiPessoasCarros.rb "$rg" "$nomePessoa" "$idade" "$cidade" "$crv" "$nomeCarro" "$marca"

    elif [ "$1" = "carros" ]; then
        crv=$(echo "$2" | cut -d'=' -f2)
        nomeCarro=$(echo "$3" | cut -d'=' -f2)
        marca=$(echo "$4" | cut -d'=' -f2)
        rg=$(echo "$5" | cut -d'=' -f2)
        nomePessoa=$(echo "$6" | cut -d'=' -f2)
        idade=$(echo "$7" | cut -d'=' -f2)
        cidade=$(echo "$8" | cut -d'=' -f2)

        ruby Relacionais/incluiPessoasCarros.rb "$rg" "$nomePessoa" "$idade" "$cidade" "$crv" "$nomeCarro" "$marca"
    
    elif [ "$1" = "computadors" ]; then
        numSerie=$(echo "$2" | cut -d'=' -f2)
        marca=$(echo "$3" | cut -d'=' -f2)
        rg=$(echo "$4" | cut -d'=' -f2)

        ruby Computadors/incluiComputadors.rb "$numSerie" "$marca" "$rg"

    elif [ "$1" = "disciplinas" ]; then
        codigo=$(echo "$2" | cut -d'=' -f2)
        nome=$(echo "$3" | cut -d'=' -f2)

        ruby Disciplinas/incluiDisciplinas.rb "$codigo" "$nome" 

    elif [ "$1" = "disPes" ]; then
        codigo=$(echo "$2" | cut -d'=' -f2)
        rg=$(echo "$3" | cut -d'=' -f2)

        ruby Relacionais/incluiDisciplinasPessoas.rb "$codigo" "$rg" 

    else
        echo "# Tabela Inexistente"
    fi
}

# lista todos os elementos da
function lista () {
    # pega o numero de parametros passados
    validarEntrada "$@"
    verificacao=$?

    # aborta o programa
    if [ $verificacao -eq 0 ]; then
        echo "# Paramêtros inconsistêntes" >&2
        return 1
    fi

    # realiza a listagem
    if [ "$1" = "pessoas" ]; then
        ruby Pessoas/listaPessoas.rb
    elif [ "$1" = "carros" ]; then
        ruby Carros/listaCarros.rb
    elif [ "$1" = "computadors" ]; then
        ruby Computadors/listaComputadors.rb
    elif [ "$1" = "disciplinas" ]; then
        ruby Disciplinas/listaDisciplinas.rb
    elif [ "$1" = "disPes" ]; then
        ruby Relacionais/listaDisciplinasPessoas.rb
    else
        echo "# Tabela Inexistente"
    fi
}

# exclui um elemento da tabela
function exclui () {
    # pega o numero de parametros passados
    validarEntrada "$@"
    verificacao=$?

    # aborta o programa
    if [ $verificacao -eq 0 ]; then
        echo "# Paramêtros inconsistêntes" >&2
        return 1
    fi

    # realiza a exclusão
    if [ "$1" = "pessoas" ]; then
        ruby Relacionais/excluiPessoasCarros.rb $1 $2
    elif [ "$1" = "carros" ]; then
        ruby Relacionais/excluiPessoasCarros.rb $1 $2
    elif [ "$1" = "computadors" ]; then
        ruby Computadors/excluiComputadors.rb $2
    elif [ "$1" = "disciplinas" ]; then
        ruby Disciplinas/excluiDisciplinas.rb $2
    else
        echo "# Tabela Inexistente"
    fi
}

function registros () {
    # 5 registros de pessoas e 5 carros
    ruby Relacionais/incluiPessoasCarros.rb 2004 lari 20 bastos 0001 hb20 hyundai
    ruby Relacionais/incluiPessoasCarros.rb 2003 rafael 20 curitiba 0002 santa hyundai
    ruby Relacionais/incluiPessoasCarros.rb 2005 henrique 21 curitiba 0003 lancer mitsubishi
    ruby Relacionais/incluiPessoasCarros.rb 2006 leonardo 20 campinas 0004 astra chevrolet
    ruby Relacionais/incluiPessoasCarros.rb 2007 milena 18 jose 0005 a3 audi

    # 4 resgistros de Computadores
    ruby Computadors/incluiComputadors.rb 1000 microsoft 2004
    ruby Computadors/incluiComputadors.rb 1100 apple 2004
    ruby Computadors/incluiComputadors.rb 1110 dell 2003
    ruby Computadors/incluiComputadors.rb 1111 positivo 2005

    # 4 registros de Disiplinas
    ruby Disciplinas/incluiDisciplinas.rb 1055 alg1
    ruby Disciplinas/incluiDisciplinas.rb 1010 progweb
    ruby Disciplinas/incluiDisciplinas.rb 1211 compiladores
    ruby Disciplinas/incluiDisciplinas.rb 1064 so

    # Registros de 10 relacionamentos entre disciplinas e pessoas
    ruby Relacionais/incluiDisciplinasPessoas.rb 1055 2004
    ruby Relacionais/incluiDisciplinasPessoas.rb 1010 2003
    ruby Relacionais/incluiDisciplinasPessoas.rb 1064 2003
    ruby Relacionais/incluiDisciplinasPessoas.rb 1211 2005
    ruby Relacionais/incluiDisciplinasPessoas.rb 1055 2006
    ruby Relacionais/incluiDisciplinasPessoas.rb 1010 2006
    ruby Relacionais/incluiDisciplinasPessoas.rb 1211 2007
    ruby Relacionais/incluiDisciplinasPessoas.rb 1211 2003
    ruby Relacionais/incluiDisciplinasPessoas.rb 1064 2004
    ruby Relacionais/incluiDisciplinasPessoas.rb 1010 2004
}