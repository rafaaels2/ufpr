class Admin::PessoasController < ApplicationController
  before_action :set_pessoa, only: [:edit, :update, :destroy]

  def show
    # show
  end
  
  def index
    @pessoas = Pessoa.all
  end

  def new
    @pessoa = Pessoa.new
    @pessoa.build_carro
  end

  def create
    @pessoa = Pessoa.new(pessoa_params)
    if @pessoa.save
      redirect_to admin_view_path, notice: 'Pessoa e Carro foram criados com sucesso.'
    else
      render :new
    end
  end

  def edit
  end

  def update
    if @pessoa.update(pessoa_params)
      redirect_to admin_view_path, notice: 'Pessoa e Carro foram atualizados com sucesso.'
    else
      render :edit
    end
  end

  def destroy
    @pessoa.destroy
    redirect_to admin_view_path, notice: 'Pessoa e Carro foram excluídos com sucesso.'
  end

  private

  def set_pessoa
    @pessoa = Pessoa.find(params[:id])
  end

  def pessoa_params
    params.require(:pessoa).permit(:rg, :nome, :idade, :cidade, carro_attributes: [:id, :crv, :nome, :marca, :_destroy])
  end
end

