$:.push './'
require 'pessoa.rb'

# inicializa a nova pessoa
if ARGV.length == 4
    pessoa = Pessoa.new ({:rg=>ARGV[0],:nome=>ARGV[1],:idade=>ARGV[2],:cidade=>ARGV[3]})
# aborta com numero errado de parametros
else
    abort ("# Número insufisciênte de parâmetros")
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