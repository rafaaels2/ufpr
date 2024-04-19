$:.push './'
require './Carros/carro.rb'

# imprime todas os carros da tabela
carro = Carro.all
carro.each do |p|
    puts "# CRV: #{p.crv} | NOME: #{p.nome} | MARCA: #{p.marca}"
end