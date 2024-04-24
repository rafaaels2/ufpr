$:.push './'
require './Disciplinas/disciplina.rb'

# imprime todas as disciplinas da tabela
disciplinas = Disciplina.all
disciplinas.each do |d|
    puts "# CÓDIGO: #{d.codigo} | NOME: #{d.nome}"
end