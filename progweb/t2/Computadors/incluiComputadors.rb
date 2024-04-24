$:.push './'
require './Computadors/computador.rb'
require './Pessoas/pessoa.rb'

# verifica se todas as strings de ARGV são nulas
string_nula = !ARGV.all? { |str| !str.nil? && !str.empty? }

# inicializa a novo computador
if !string_nula
    computador = Computador.new ({:numSerie=>ARGV[0],:marca=>ARGV[1]})

    pessoa = Pessoa.find_by_rg(ARGV[2])

    if pessoa.nil?
        abort ("# Este RG não existe")
    end

    # relacao 1:n
    computador.pessoa = pessoa

# aborta com numero errado de parametros
else 
    abort ("# Paramêtros inconsistêntes")
end

# confere se não há erros no computador
if computador.invalid?
    puts pessoa.errors[:numSerie]
    puts pessoa.errors[:marca]

# salva o computador no banco de dados
else
    puts "# Computador Incluido"
    computador.save
end