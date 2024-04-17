#!/bin/bash

function validarEntrada {
    if [ $# -lt 2 ]; then
        return 0
    fi

    return 1
}

function inicializa {
    ruby criaEstados.rb
}

function exclui {
    rm -rf *.sqlite3*
}

function inclui {
    validarEntrada "$@"
    verificacao=$?

    if [ $verificacao -eq 1 ]; then
        echo boa
    fi
}
