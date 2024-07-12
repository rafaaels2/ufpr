const grade = [
    "CI068", "CI210", "CI212", "CI215", "CI162", "CI163", "CI221", "SE045",
    "CI055", "CI056", "CI057", "CI062", "CI065", "CI165", "CI211", "SC003",
    "CM046", "CI067", "CI064", "CE003", "CI059", "CI209", "CI205", "CI340",
    "CM045", "CM005", "CI237", "CI058", "CI061", "CI218", "CI351", "CI310",
    "CM201", "CM202", "CI166", "CI164", "SA214", "CI220", "CI206", "SC202"
]

let flag = 0
let filtredGrr
/* -------------------------------------------------------------------------- */

function inicializarTabela () {
    for (let i = 0; i < grade.length;i ++) {
        const elemento = document.getElementById(grade[i])
        elemento.style.backgroundColor = "white"
    }
}

function atualizarEstilo (materia, situacao, nome) {
    const elemento = document.getElementById(materia);
    if (elemento) {
        if (situacao == 1)
            elemento.style.backgroundColor = "green"
        else if (situacao == 2 || situacao == 3 || situacao == 9)
            elemento.style.backgroundColor = "red"
        else if (situacao == 5 || situacao == 12 || situacao == 15)
            elemento.style.backgroundColor = "white"
        else if (situacao == 4 || situacao == 11)
            elemento.style.backgroundColor = "yellow"
        else if (situacao == 10)
            elemento.style.backgroundColor = "blue"
        else 
            elemento.style.backgroundColor = "white"
    }

    flag = 1
}

document.addEventListener('DOMContentLoaded', () => {
    const xhr = new XMLHttpRequest()
    xhr.open('GET', 'alunos.xml', true)
    xhr.onreadystatechange = function() {
        if (xhr.readyState === 4 && xhr.status === 200) {
            const xmlDoc = xhr.responseXML
            const alunos = xmlDoc.getElementsByTagName('ALUNO');
            
            // Função para filtrar frutas por nome
            function filtrarGrr (selectedGrr) {
                const grrFiltrado = [];
                for (let i = 0; i < alunos.length; i++) {
                    const matricula = alunos[i].getElementsByTagName('MATR_ALUNO')[0].textContent
                    if (matricula === selectedGrr) {
                        const materia = alunos[i].getElementsByTagName('COD_ATIV_CURRIC')[0].textContent
                        const ano = alunos[i].getElementsByTagName('ANO')[0].textContent
                        const situacao = alunos[i].getElementsByTagName('SITUACAO_ITEM')[0].textContent
                        const nome = alunos[i].getElementsByTagName('NOME_ATIV_CURRIC')[0].textContent
                        const semestre = alunos[i].getElementsByTagName('PERIODO')[0].textContent
                        const nota = alunos[i].getElementsByTagName('MEDIA_FINAL')[0].textContent
                        const frequencia = alunos[i].getElementsByTagName('FREQUENCIA')[0].textContent
                        
                        const encontrada = grrFiltrado.find (grr => grr.materia === materia)
                        if (encontrada) {
                            encontrada.detalhes[encontrada.detalhes.length] = {ano, semestre, nota, frequencia, situacao}
                        }
                        else {
                            grrFiltrado.push({
                                materia, 
                                nome, 
                                detalhes: [{ano, semestre, nota, frequencia, situacao}]
                            })
                        }
                        
                    }
                }
                return grrFiltrado;
            }

            function atualizaGrade (selectedGrr) {
                inicializarTabela ()
                // Adiciona as linhas da tabela
                selectedGrr.forEach(grr => {
                    let maior = grr.detalhes[0]
                    for (let i = 1; i < grr.detalhes.length; i++) {
                        if (grr.detalhes[i].ano > maior.ano) {
                            maior = grr.detalhes[i]
                        }
                        else  {
                            const numC = parseInt(grr.detalhes[i].semestre[0], 10);
                            const numM = parseInt(maior.semestre[0], 10);

                            if (grr.detalhes[i].ano === maior.ano && numC > numM) {
                                maior = grr.detalhes[i]
                            }
                        }
                    }

                    atualizarEstilo (grr.materia, maior.situacao, grr.nome) 
                });
    
            }

            // Evento para filtrar e exibir frutas quando o botão é clicado
            document.getElementById('grr-btn').addEventListener('click', () => {
                const selectedGrr = document.getElementById('grr-input').value
                filtredGrr = filtrarGrr (selectedGrr)
                atualizaGrade (filtredGrr)
            });

        }
    };
    xhr.send();
});


function mostrarModal (materia) {
    if (flag === 1) {
        const encontrada = filtredGrr.find (grr => grr.materia === materia)
        if (encontrada) {
            let maior = []
            document.getElementById("tituloModal").innerText = `${encontrada.materia} \ ${encontrada.nome}`;
            maior = encontrada.detalhes[0]
            for (let i = 1; i < encontrada.detalhes.length; i++) {
                if (encontrada.detalhes[i].ano > maior.ano) {
                    maior = encontrada.detalhes[i]
                }
                else  {
                    const numC = parseInt(encontrada.detalhes[i].semestre[0], 10);
                    const numM = parseInt(maior.semestre[0], 10);

                    if (encontrada.detalhes[i].ano === maior.ano && numC > numM) {
                        maior = encontrada.detalhes[i]
                    }
                }
            }
            document.getElementById("textoModal").innerText = `cursado pela última vez no ${maior.semestre} de ${maior.ano}. Nota: ${maior.nota} Frequência: ${maior.frequencia}`
            document.getElementById("meuModal").style.display = "block"
        }
    }
}

function fecharModal() {
    document.getElementById("meuModal").style.display = "none"
}

// Fecha o modal se o usuário clicar fora dele
window.onclick = function(event) {
    const modal = document.getElementById("meuModal")
    const modal2 = document.getElementById("meuModal2")
    if (event.target === modal || event.target === modal2) {
        modal.style.display = "none"
        modal2.style.display = "none"
        document.getElementById("textoModal2").innerText = ''
    }
}

function mostrarModal2 (materia) {
    if (flag === 1) {
        const encontrada = filtredGrr.find (grr => grr.materia === materia)
        if (encontrada) {
            document.getElementById("tituloModal2").innerText = `${encontrada.materia} \ ${encontrada.nome}`;
            for (let i = 0; i < encontrada.detalhes.length; i++) {
                document.getElementById("textoModal2").innerText += `\ncursado no ${encontrada.detalhes[i].semestre} de ${encontrada.detalhes[i].ano}. Nota: ${encontrada.detalhes[i].nota} Frequência: ${encontrada.detalhes[i].frequencia}`
            }
            document.getElementById("meuModal2").style.display = "block"
        }
    }
}

function fecharModal2() {
    document.getElementById("meuModal2").style.display = "none"
    document.getElementById("textoModal2").innerText = ''
}

inicializarTabela ()
