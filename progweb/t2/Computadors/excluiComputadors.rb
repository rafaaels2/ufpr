$:.push './'
require './Pessoas/pessoa.rb'
require './Carros/carro.rb'
require './Computadors/computador.rb'

# verifica os parametros
if ARGV.length != 1 
    abort ("# Paramêtros inconsistêntes")
end

computador = Computador.find_by_numSerie(ARGV[0])

if computador.nil?
    abort ("# Este NUM SÉRIE não existe")
end

computador.destroy
puts "# Computador Excluído"