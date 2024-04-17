$:.push './'
require 'estado.rb'

estados = Estado.all
estados.each do |e|
    puts "#{e.id} | #{e.sigla} = #{e.nome}"
end