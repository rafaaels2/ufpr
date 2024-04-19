$:.push './'
require 'pessoa.rb'

# imprime todas as pessoas da tabela
pessoas = Pessoa.all
pessoas.each do |p|
    puts "# RG: #{p.rg} | NOME: #{p.nome} | IDADE: #{p.idade} | CIDADE: #{p.cidade}"
end