$:.push './'
require 'estado.rb'

estado = Estado.find_by_sigla("SP")
estado.update({:nome => "São Paulo"})