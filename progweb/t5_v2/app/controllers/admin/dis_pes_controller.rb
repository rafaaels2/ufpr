class Admin::DisPesController < ApplicationController
  def new
    @dis_pe = DisPe.new
  end

  def create
    @dis_pe = DisPe.new(dis_pe_params)
    if @dis_pe.save
      redirect_to admin_view_path, notice: 'Registro DisPes criado com sucesso.'
    else
      render :new
    end
  end

  def edit
    @dis_pe = DisPe.find(params[:id])
  end

  def update
    @dis_pe = DisPe.find(params[:id])
    if @dis_pe.update(dis_pe_params)
      redirect_to admin_view_path, notice: 'Registro DisPes atualizado com sucesso.'
    else
      render :edit
    end
  end

  def destroy
    @dis_pe = DisPe.find(params[:id])
    @dis_pe.destroy
    redirect_to admin_view_path, notice: 'Registro DisPes excluído com sucesso.'
  end

  private

  def dis_pe_params
    params.require(:dis_pe).permit(:disciplina_id, :pessoa_id)
  end
end

