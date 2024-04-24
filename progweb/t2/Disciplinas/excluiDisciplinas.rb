$:.push './'
require './Disciplinas/disciplina.rb'
require './Pessoas/pessoa.rb'

# verifica os parametros
if ARGV.length != 1 
    abort ("# Paramêtros inconsistêntes")
end

disciplina = Disciplina.find_by_codigo(ARGV[0])

if disciplina.nil?
    abort ("# Este CÓDIGO não existe")
end

disciplina.destroy
puts "# Disciplina Excluída"