class ComputadorsController < ApplicationController
  before_action :set_computador, only: %i[ show edit update destroy ]

  # GET /computadors or /computadors.json
  def index
    @computadors = Computador.all
  end

  # GET /computadors/1 or /computadors/1.json
  def show
  end

  # GET /computadors/new
  def new
    @computador = Computador.new
  end

  # GET /computadors/1/edit
  def edit
  end

  # POST /computadors or /computadors.json
  def create
    @computador = Computador.new(computador_params)

    respond_to do |format|
      if link_pessoa_to_computador && @computador.save
        format.html { redirect_to computador_url(@computador), notice: "Computador foi criado com sucesso." }
        format.json { render :show, status: :created, location: @computador }
      else
        format.html { render :new, status: :unprocessable_entity }
        format.json { render json: @computador.errors, status: :unprocessable_entity }
      end
    end
  end

  # PATCH/PUT /computadors/1 or /computadors/1.json
  def update
    respond_to do |format|
      if @computador.update(computador_params)
        format.html { redirect_to computador_url(@computador), notice: "Computador foi atualizado com sucesso." }
        format.json { render :show, status: :ok, location: @computador }
      else
        format.html { render :edit, status: :unprocessable_entity }
        format.json { render json: @computador.errors, status: :unprocessable_entity }
      end
    end
  end

  # DELETE /computadors/1 or /computadors/1.json
  def destroy
    @computador.destroy!

    respond_to do |format|
      format.html { redirect_to computadors_url, notice: "Computador foi excluido com sucesso." }
      format.json { head :no_content }
    end
  end
  
  def visualizar
    @computadors = Computador.all
    render :index, locals: { edit_allowed: false }
  end

  private
    # Use callbacks to share common setup or constraints between actions.
    def set_computador
      @computador = Computador.find(params[:id])
    end

    # Only allow a list of trusted parameters through.
    def computador_params
      params.require(:computador).permit(:numserie, :marca, :pessoa_id)
    end
    
    def link_pessoa_to_computador
      rg = params[:computador][:rg]
      pessoa = Pessoa.find_by(rg: rg)
      if pessoa
          @computador.pessoa = pessoa
          return true
      else
        @computador.errors.add(:base, "Pessoa com o RG fornecido não encontrada")
        return false
      end
    end
end
