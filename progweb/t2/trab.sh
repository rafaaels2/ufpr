#!/bin/bash

##########################
# SEM TRATAMENTO DE ERRO #
##########################

# inicializa as tabelas
function inicializa () {
    ruby Carros/criaCarros.rb
    ruby Pessoas/criaPessoas.rb

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
function inclui () {
    # pega o numero de parametros passados
    validarEntrada "$@"
    verificacao=$?

    # aborta o programa
    if [ $verificacao -eq 0 ]; then
        echo "# Número insufisciênte de parâmetros" >&2
        return 1
    fi

    # realiza a inclusao
    if [ "$1" = "pessoas" ]; then
        ruby Pessoas/inicializaPessoas.rb $2 $3 $4 $5
    elif ["$1" = "carros"]; then
        echo "carros"
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
        echo "# Número insufisciênte de parâmetros" >&2
        return 1
    fi

    # realiza a listagem
    if [ "$1" = "pessoas" ]; then
        ruby Pessoas/listaPessoas.rb
    elif [ "$1" = "carros" ]; then
        echo "carros"
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
        echo "# Número insufisciênte de parâmetros" >&2
        return 1
    fi

    # realiza a exclusão
    if [ "$1" = "pessoas" ]; then
        ruby Pessoas/removePessoas.rb $2
    elif [ "$1" = "carros" ]; then
        echo "carros"
    else
        echo "# Tabela Inexistente"
    fi
}