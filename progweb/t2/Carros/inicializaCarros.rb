$:.push './'
require './Carros/carro.rb'

# verifica se todas as strings de ARGV são não nulas
string_nula = ARGV.all? { |str| !str.nil? && !str.empty? }

# inicializa a nova carro
if string_nula
    carro = Carro.new ({:crv=>ARGV[0],:nome=>ARGV[1],:marca=>ARGV[2]})
# aborta com numero errado de parametros
else
    abort ("# Paramêtros inconsistêntes")
end

# confere se não há erros
if carro.invalid?
    puts carro.errors[:crv]
    puts carro.errors[:nome]
    puts carro.errors[:marca]
# salva a carro no banco de dados
else
    puts "# Carro incluido"
    carro.save
end