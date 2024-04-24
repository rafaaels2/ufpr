$:.push './'
require './Disciplinas/disciplina.rb'

# verifica se todas as strings de ARGV são nulas
string_nula = !ARGV.all? { |str| !str.nil? && !str.empty? }

# inicializa a novo disciplina
if !string_nula
    disciplina = Disciplina.new ({:codigo=>ARGV[0],:nome=>ARGV[1]})

# aborta com numero errado de parametros
else 
    abort ("# Paramêtros inconsistêntes")
end

# confere se não há erros no disciplina
if disciplina.invalid?
    puts pessoa.errors[:codigo]
    puts pessoa.errors[:nome]

# salva o disciplina no banco de dados
else
    puts "# Disciplina Incluida"
    disciplina.save
end