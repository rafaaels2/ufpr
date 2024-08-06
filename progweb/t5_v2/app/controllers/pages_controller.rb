class PagesController < ApplicationController
  def user_view
    @pessoas = Pessoa.all
    @carros = Carro.all
    @computadors = Computador.all
    @disciplinas = Disciplina.all
    @dis_pes = DisPe.all
  end
  
  def admin_view
    @pessoa = Pessoa.new
    @carro = Carro.new
    @computador = Computador.new
    @disciplina = Disciplina.new
    @dis_pe = DisPe.new
  end
end

