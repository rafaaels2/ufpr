$:.push './'
require './Computadors/computador.rb'
require './Pessoas/pessoa.rb'

# imprime todas os carros da tabela
computadors = Computador.all
computadors.each do |c|
    puts "# NUM SÉRIE: #{c.numSerie} | MARCA: #{c.marca} | RG RESPONSÀVEL: #{c.pessoa.rg}"
end