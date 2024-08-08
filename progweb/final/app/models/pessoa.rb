class Pessoa < ApplicationRecord
  has_one :carro
  has_many :computadors
  has_and_belongs_to_many :disciplinas
end

