class Admin::PessoasController < ApplicationController
  def new
    @pessoa = Pessoa.new
    @pessoa.build_carro
  end

  def create
    @pessoa = Pessoa.new(pessoa_params)
    if @pessoa.save
      redirect_to admin_view_path, notice: 'Pessoa e Carro criados com sucesso.'
    else
      render :new
    end
  end

  def edit
    @pessoa = Pessoa.find(params[:id])
    @pessoa.build_carro unless @pessoa.carro
  end

  def update
    @pessoa = Pessoa.find(params[:id])
    if @pessoa.update(pessoa_params)
      redirect_to admin_view_path, notice: 'Pessoa e Carro atualizados com sucesso.'
    else
      render :edit
    end
  end

  def destroy
    @pessoa = Pessoa.find(params[:id])
    @pessoa.destroy
    redirect_to admin_view_path, notice: 'Pessoa e Carro excluídos com sucesso.'
  end

  private

  def pessoa_params
    params.require(:pessoa).permit(:rg, :nome, :idade, :cidade, carro_attributes: [:crv, :nome, :marca])
  end
end

