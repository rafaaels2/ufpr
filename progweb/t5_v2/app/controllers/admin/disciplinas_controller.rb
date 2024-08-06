class Admin::DisciplinasController < ApplicationController
  def new
    @disciplina = Disciplina.new
  end

  def create
    @disciplina = Disciplina.new(disciplina_params)
    if @disciplina.save
      redirect_to admin_view_path, notice: 'Disciplina criada com sucesso.'
    else
      render :new
    end
  end

  def edit
    @disciplina = Disciplina.find(params[:id])
  end

  def update
    @disciplina = Disciplina.find(params[:id])
    if @disciplina.update(disciplina_params)
      redirect_to admin_view_path, notice: 'Disciplina atualizada com sucesso.'
    else
      render :edit
    end
  end

  def destroy
    @disciplina = Disciplina.find(params[:id])
    @disciplina.destroy
    redirect_to admin_view_path, notice: 'Disciplina excluída com sucesso.'
  end

  private

  def disciplina_params
    params.require(:disciplina).permit(:codigo, :nome)
  end
end

