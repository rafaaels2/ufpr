$:.push './'
require './Pessoas/pessoa.rb'
require './Disciplinas/disciplina.rb'

# verifica se todas as strings de ARGV são nulas
string_nula = !ARGV.all? { |str| !str.nil? && !str.empty? }

# inicializa a nova pessoa
if !string_nula
    disciplina = Disciplina.find_by_codigo(ARGV[0])
    pessoa = Pessoa.find_by_rg(ARGV[1])   

    if pessoa.nil?
        abort ("# Este RG não existe")
    end

    if disciplina.nil?
        abort ("# Este CÓDIGO não existe")
    end

    pessoa.disciplinas << disciplina
    
# aborta com numero errado de parametros
else 
    abort ("# Paramêtros inconsistêntes")
end

puts "# Pessoa e Disciplina Relacionados"