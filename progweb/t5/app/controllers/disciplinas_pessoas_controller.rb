class DisciplinasPessoasController < ApplicationController
  before_action :set_disciplinas_pessoa, only: %i[ show edit update destroy ]

  # GET /disciplinas_pessoas or /disciplinas_pessoas.json
  def index
    @disciplinas_pessoas = DisciplinasPessoa.all
  end

  # GET /disciplinas_pessoas/1 or /disciplinas_pessoas/1.json
  def show
  end

  # GET /disciplinas_pessoas/new
  def new
    @disciplinas_pessoa = DisciplinasPessoa.new
  end

  # GET /disciplinas_pessoas/1/edit
  def edit
  end

  # POST /disciplinas_pessoas or /disciplinas_pessoas.json
  def create
    @disciplinas_pessoa = DisciplinasPessoa.new(disciplinas_pessoa_params)

    respond_to do |format|
      if link_disciplina_pessoa && @disciplinas_pessoa.save
        format.html { redirect_to disciplinas_pessoa_url(@disciplinas_pessoa), notice: "Disciplinas pessoa was successfully created." }
        format.json { render :show, status: :created, location: @disciplinas_pessoa }
      else
        format.html { render :new, status: :unprocessable_entity }
        format.json { render json: @disciplinas_pessoa.errors, status: :unprocessable_entity }
      end
    end
  end

  # PATCH/PUT /disciplinas_pessoas/1 or /disciplinas_pessoas/1.json
  def update
    respond_to do |format|
      if @disciplinas_pessoa.update(disciplinas_pessoa_params)
        format.html { redirect_to disciplinas_pessoa_url(@disciplinas_pessoa), notice: "Disciplinas pessoa was successfully updated." }
        format.json { render :show, status: :ok, location: @disciplinas_pessoa }
      else
        format.html { render :edit, status: :unprocessable_entity }
        format.json { render json: @disciplinas_pessoa.errors, status: :unprocessable_entity }
      end
    end
  end

  # DELETE /disciplinas_pessoas/1 or /disciplinas_pessoas/1.json
  def destroy
    @disciplinas_pessoa.destroy!

    respond_to do |format|
      format.html { redirect_to disciplinas_pessoas_url, notice: "Disciplinas pessoa was successfully destroyed." }
      format.json { head :no_content }
    end
  end
  
  def visualizar
    @disciplinas_pessoas = DisciplinasPessoa.all
    render :index, locals: { edit_allowed: false }
  end

  private
    # Use callbacks to share common setup or constraints between actions.
    def set_disciplinas_pessoa
      @disciplinas_pessoa = DisciplinasPessoa.find(params[:id])
    end

    # Only allow a list of trusted parameters through.
    def disciplinas_pessoa_params
      params.require(:disciplinas_pessoa).permit(:disciplina_id, :pessoa_id)
    end
    
    def link_disciplina_pessoa
      rg = params[:disciplinas_pessoa][:rg]
      codigo = params[:disciplinas_pessoa][:codigo]
      pessoa = Pessoa.find_by(rg: rg)
      disciplina = Disciplina.find_by(codigo: codigo)
      if pessoa && disciplina
        @disciplinas_pessoa.pessoa = pessoa
        @disciplinas_pessoa.disciplina = disciplina
        return true
      else
        @disciplinas_pessoa.errors.add(:base, "Pessoa ou Disciplina não existe.")
        return false
      end
    end
end
