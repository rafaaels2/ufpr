class DisPe < ApplicationRecord
  belongs_to :pessoa
  belongs_to :disciplina

  validates :pessoa_id, :disciplina_id, presence: true
end

