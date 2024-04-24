$:.push './'
require './Pessoas/pessoa.rb'
require './Carros/carro.rb'
require './Computadors/computador.rb'
require './Disciplinas/disciplina.rb'

# verifica os parametros
if ARGV.length != 2 
    abort ("# Paramêtros inconsistêntes")
end

# remove pela pessoa
if ARGV[0] == "pessoas"
    pessoa = Pessoa.find_by_rg(ARGV[1])

    if pessoa.nil?
        abort ("# Este RG não existe")
    end

    # destroi computadores
    if pessoa.computadors.any?
        pessoa.computadors.destroy_all
    end

    carro = pessoa.carro

    # destroi carro e pessoa
    carro.destroy
    pessoa.destroy
    puts "# Pessoa excluída"

# remove pelo carro
elsif ARGV[0] == "carros"
    carro = Carro.find_by_crv(ARGV[1])

    if carro.nil?
        abort (" Este CRV não existe")
    end

    pessoa = carro.pessoa

    # destroi computadores
    if pessoa.computadors.any?
        pessoa.computadors.destroy_all
    end
    
    carro.destroy
    pessoa.destroy   
    puts "# Carro excluído"

# tabela passada como parametro nao existe
else 
    puts "# Tabela Inexistente"
end