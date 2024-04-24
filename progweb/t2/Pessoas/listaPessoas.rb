$:.push './'
require './Pessoas/pessoa.rb'
require './Carros/carro.rb'
require './Computadors/computador.rb'
require './Disciplinas/disciplina.rb'

# imprime todas as pessoas da tabela
pessoas = Pessoa.all
pessoas.each do |p|
    puts "# RG: #{p.rg} | NOME: #{p.nome} | IDADE: #{p.idade} | CIDADE: #{p.cidade} | CRV POSSUÍDO: #{p.carro.crv}"

    computadors = p.computadors

    # imprime os computadores
    print "# COMPUTADORES POSSUÍDOS: "
    computadors.each do |c|
        print "#{c.numSerie} | "
    end

    # quebra de linha
    puts

    disciplinas = p.disciplinas
    # imprime as disciplinas
    print "# DISCIPLINAS MATRICULADAS: "
    disciplinas.each do |d|
        print "#{d.nome} | "
    end

    # quebra de linha
    puts
end