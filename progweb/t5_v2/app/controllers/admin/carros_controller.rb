class Admin::CarrosController < ApplicationController
  def index
    @carros = Carro.all
  end

  def new
    @carro = Carro.new
  end

  def create
    @carro = Carro.new(carro_params)
    if @carro.save
      redirect_to admin_carros_path, notice: 'Carro foi criado com sucesso.'
    else
      render :new
    end
  end

  def edit
    @carro = Carro.find(params[:id])
  end

  def update
    @carro = Carro.find(params[:id])
    if @carro.update(carro_params)
      redirect_to admin_carros_path, notice: 'Carro foi atualizado com sucesso.'
    else
      render :edit
    end
  end

  def destroy
    @carro = Carro.find(params[:id])
    @carro.destroy
    redirect_to admin_carros_path, notice: 'Carro foi excluído com sucesso.'
  end

  private

  def carro_params
    params.require(:carro).permit(:crv, :nome, :marca, :pessoa_id)
  end
end

