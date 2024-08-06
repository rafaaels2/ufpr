class Admin::ComputadorsController < ApplicationController
  def new
    @computador = Computador.new
  end

  def create
    @computador = Computador.new(computador_params)
    if @computador.save
      redirect_to admin_view_path, notice: 'Computador criado com sucesso.'
    else
      render :new
    end
  end

  def edit
    @computador = Computador.find(params[:id])
  end

  def update
    @computador = Computador.find(params[:id])
    if @computador.update(computador_params)
      redirect_to admin_view_path, notice: 'Computador atualizado com sucesso.'
    else
      render :edit
    end
  end

  def destroy
    @computador = Computador.find(params[:id])
    @computador.destroy
    redirect_to admin_view_path, notice: 'Computador excluído com sucesso.'
  end

  private

  def computador_params
    params.require(:computador).permit(:numserie, :marca, :pessoa_id)
  end
end

