class DisciplinasPessoa < ApplicationRecord
  belongs_to :disciplina
  belongs_to :pessoa
end
