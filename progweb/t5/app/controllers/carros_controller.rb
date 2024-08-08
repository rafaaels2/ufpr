class CarrosController < ApplicationController
  before_action :set_carro, only: %i[ show edit update destroy ]

  # GET /carros or /carros.json
  def index
    @carros = Carro.all
  end

  # GET /carros/1 or /carros/1.json
  def show
  end

  # GET /carros/new
  def new
    @carro = Carro.new
  end

  # GET /carros/1/edit
  def edit
  end

  # POST /carros or /carros.json
  def create
    @carro = Carro.new(carro_params)

    respond_to do |format|
      if link_pessoa_to_carro && @carro.save
        format.html { redirect_to carro_url(@carro), notice: "Carro foi criado com sucesso." }
        format.json { render :show, status: :created, location: @carro }
      else
        format.html { render :new, status: :unprocessable_entity }
        format.json { render json: @carro.errors, status: :unprocessable_entity }
      end
    end
  end

  # PATCH/PUT /carros/1 or /carros/1.json
  def update
    respond_to do |format|
      if link_pessoa_to_carro && @carro.update(carro_params)
        format.html { redirect_to carro_url(@carro), notice: "Carro foi atualizado com sucesso." }
        format.json { render :show, status: :ok, location: @carro }
      else
        format.html { render :edit, status: :unprocessable_entity }
        format.json { render json: @carro.errors, status: :unprocessable_entity }
      end
    end
  end

  # DELETE /carros/1 or /carros/1.json
  def destroy
    @carro.destroy!

    respond_to do |format|
      format.html { redirect_to carros_url, notice: "Carro foi destruido com sucesso." }
      format.json { head :no_content }
    end
  end
  
  def visualizar
    @carros = Carro.all
    render :index, locals: { edit_allowed: false }
  end

  private
    # Use callbacks to share common setup or constraints between actions.
    def set_carro
      @carro = Carro.find(params[:id])
    end

    # Only allow a list of trusted parameters through.
    def carro_params
      params.require(:carro).permit(:crv, :nome, :marca, :pessoa_id)
    end

    def link_pessoa_to_carro
      rg = params[:carro][:rg]
      pessoa = Pessoa.find_by(rg: rg)
      if pessoa
        if pessoa.carro
          @carro.errors.add(:base, "Pessoa já possui um carro")
          return false
        else
          @carro.pessoa = pessoa
          return true
        end
      else
        @carro.errors.add(:base, "Pessoa com o RG fornecido não encontrada")
        return false
      end
    end
end

