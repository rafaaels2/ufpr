class Pessoa < ApplicationRecord
  has_one :carro
  has_many :computadors
  has_and_belongs_to_many :disciplinas
  
  validates :rg, :nome, :idade, :cidade, presence: true
end

