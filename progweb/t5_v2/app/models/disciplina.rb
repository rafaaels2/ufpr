class Disciplina < ApplicationRecord
  has_and_belongs_to_many :pessoas

  validates :codigo, :nome, presence: true
end

