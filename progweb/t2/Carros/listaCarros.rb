$:.push './'
require './Carros/carro.rb'
require './Pessoas/pessoa.rb'

# imprime todas os carros da tabela
carros = Carro.all
carros.each do |c|
    puts "# CRV: #{c.crv} | NOME: #{c.nome} | MARCA: #{c.marca} | RG RESPONSÀVEL: #{c.pessoa.rg}"
end