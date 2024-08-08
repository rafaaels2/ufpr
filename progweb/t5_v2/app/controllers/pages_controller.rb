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
    @pessoa.build_carro
    @pessoas = Pessoa.all
  end

end

