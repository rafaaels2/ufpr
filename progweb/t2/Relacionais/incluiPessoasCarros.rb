$:.push './'
require './Pessoas/pessoa.rb'
require './Carros/carro.rb'

# verifica se todas as strings de ARGV são nulas
string_nula = !ARGV.all? { |str| !str.nil? && !str.empty? }

# inicializa a nova pessoa
if !string_nula
    pessoa = Pessoa.new ({:rg=>ARGV[0],:nome=>ARGV[1],:idade=>ARGV[2],:cidade=>ARGV[3]})
    carro = Carro.new ({:crv=>ARGV[4],:nome=>ARGV[5],:marca=>ARGV[6]})

    # relacionamento 1:1 
    carro.pessoa = pessoa
    
# aborta com numero errado de parametros
else 
    abort ("# Paramêtros inconsistêntes")
end

# confere se não há erros na pessoa
if pessoa.invalid?
    puts pessoa.errors[:rg]
    puts pessoa.errors[:nome]
    puts pessoa.errors[:idade]

# confere se não há erros no carro
elsif carro.invalid?
    puts carro.errors[:crv]
    puts carro.errors[:nome]
    puts carro.errors[:marca]

# salva a pessoa e o carro no banco de dados
else
    puts "# Pessoa e Carro incluidos"
    carro.save
    pessoa.save
end