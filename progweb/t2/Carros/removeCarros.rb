$:.push './'
require './Carros/carro.rb'

# remove um carro
if ARGV.length == 1 
    carro = Carro.find_by_crv(ARGV[0])
# aborta com numero errado de parametros
else
    abort ("# Paramêtros inconsistêntes")
end

# aborta caso CRV não exista
if carro.nil?
    abort ("# Este CRV não existe")
end

puts "# Carro excluido"
carro.destroy