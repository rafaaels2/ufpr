$:.push './'
require './Pessoas/pessoa.rb'

# remove uma pessoa
if ARGV.length == 1 
    pessoa = Pessoa.find_by_rg(ARGV[0])
# aborta com numero errado de parametros
else
    abort ("# Paramêtros inconsistêntes")
end

# aborta caso RG não exista
if pessoa.nil?
    abort ("# Este RG não existe")
end

puts "# Pessoa excluida"
pessoa.destroy