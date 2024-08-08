class DisciplinasPessoa < ApplicationRecord
  belongs_to :disciplina
  belongs_to :pessoa
  
  validates :disciplina_id, :pessoa_id, presence: true
end
