class Carro < ApplicationRecord
  belongs_to :pessoa
  
  validates :crv, :nome, :marca, presence: true
end
