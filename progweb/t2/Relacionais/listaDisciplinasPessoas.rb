$:.push './'
require 'rubygems'
require 'active_record'
require 'Disciplinas/disciplina.rb'
require 'Pessoas/pessoa.rb'

pessoas = Pessoa.all
pessoas.each do |pessoa|
    disciplinas = pessoa.disciplinas
    print "# #{pessoa.nome} | "

    disciplinas.each do |disciplina|
        print "#{disciplina.nome} "
    end

    puts
end