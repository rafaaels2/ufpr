$:.push './'
require './Pessoas/pessoa.rb'

# verifica se todas as strings de ARGV são não nulas
string_nula = ARGV.all? { |str| !str.nil? && !str.empty? }

# inicializa a nova pessoa
if string_nula
    pessoa = Pessoa.new ({:rg=>ARGV[0],:nome=>ARGV[1],:idade=>ARGV[2],:cidade=>ARGV[3]})
# aborta com numero errado de parametros
else 
    abort ("# Paramêtros inconsistêntes")
end

# confere se não há erros
if pessoa.invalid?
    puts pessoa.errors[:rg]
    puts pessoa.errors[:nome]
    puts pessoa.errors[:idade]
    puts pessoa.errors[:cidade]
# salva a pessoa no banco de dados
else
    puts "# Pessoa incluida"
    pessoa.save
end