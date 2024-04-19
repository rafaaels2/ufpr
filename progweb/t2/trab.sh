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
        nome=$(echo "$3" | cut -d'=' -f2)
        idade=$(echo "$4" | cut -d'=' -f2)
        cidade=$(echo "$5" | cut -d'=' -f2)

        ruby Pessoas/inicializaPessoas.rb "$rg" "$nome" "$idade" "$cidade"
    elif [ "$1" = "carros" ]; then
        crv=$(echo "$2" | cut -d'=' -f2)
        nome=$(echo "$3" | cut -d'=' -f2)
        marca=$(echo "$4" | cut -d'=' -f2)

        ruby Carros/inicializaCarros.rb "$crv" "$nome" "$marca"
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
        ruby Pessoas/removePessoas.rb $2
    elif [ "$1" = "carros" ]; then
        ruby Carros/removeCarros.rb $2
    else
        echo "# Tabela Inexistente"
    fi
}

function teste () {
    echo $# 
    echo $5

    parte=$(echo "$5" | cut -d'=' -f2)
    echo "$parte"
}